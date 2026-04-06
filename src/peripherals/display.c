/*
 * display.c
 * LCD display driver for DE10-Standard 128x64 LCM module
 * Uses DIRECT REGISTER ACCESS via /dev/mem
 */

#include "peripherals/display.h"
#include "../includes/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#define SPIM0_BASE        0xFFF00000
#define SPIM0_SPAN        0x1000
#define SPIM_CTRLR0       0x00
#define SPIM_CTRLR1       0x04
#define SPIM_SPIENR       0x08
#define SPIM_SER          0x10
#define SPIM_BAUDR        0x14
#define SPIM_TXFTLR       0x18
#define SPIM_RXFTLR       0x1C
#define SPIM_TXFLR        0x20
#define SPIM_RXFLR        0x24
#define SPIM_SR           0x28
#define SPIM_IMR          0x2C
#define SPIM_DR           0x60

#define SR_TFE            (1 << 2)
#define SR_TFNF           (1 << 1)
#define SR_BUSY           (1 << 0)

#define GPIO1_BASE        0xFF709000
#define GPIO1_SPAN        0x1000
#define GPIO1_SWPORTA_DR  0x00
#define GPIO1_SWPORTA_DDR 0x04

#define BIT_BK            (1 << 8)
#define BIT_DC            (1 << 12)
#define BIT_RST           (1 << 15)

static int mem_fd = -1;
static int hw_available = 0;
static void *spi_base = NULL;
static void *gpio1_base = NULL;
static uint8_t framebuffer[1024];

static inline uint32_t spi_read(uint32_t offset) {
    return *((volatile uint32_t *)((uint8_t *)spi_base + offset));
}
static inline void spi_write(uint32_t offset, uint32_t value) {
    *((volatile uint32_t *)((uint8_t *)spi_base + offset)) = value;
}
static inline uint32_t gpio1_read(uint32_t offset) {
    return *((volatile uint32_t *)((uint8_t *)gpio1_base + offset));
}
static inline void gpio1_write(uint32_t offset, uint32_t value) {
    *((volatile uint32_t *)((uint8_t *)gpio1_base + offset)) = value;
}

static void gpio_set(uint32_t bits) {
    gpio1_write(GPIO1_SWPORTA_DR, gpio1_read(GPIO1_SWPORTA_DR) | bits);
}
static void gpio_clear(uint32_t bits) {
    gpio1_write(GPIO1_SWPORTA_DR, gpio1_read(GPIO1_SWPORTA_DR) & ~bits);
}
static void lcd_dc_command(void) { gpio_clear(BIT_DC); }
static void lcd_dc_data(void)    { gpio_set(BIT_DC); }
static void lcd_reset_on(void)   { gpio_clear(BIT_RST); }
static void lcd_reset_off(void)  { gpio_set(BIT_RST); }
static void lcd_backlight_on(void)  { gpio_set(BIT_BK); }
static void lcd_backlight_off(void) { gpio_clear(BIT_BK); }

static void spi_wait_idle(void) {
    int timeout = 100000;
    while ((spi_read(SPIM_SR) & SR_BUSY) && --timeout > 0);
}
static void spi_send_byte(uint8_t byte) {
    int timeout = 100000;
    while (!(spi_read(SPIM_SR) & SR_TFNF) && --timeout > 0);
    spi_write(SPIM_DR, (uint32_t)byte);
}
static void spi_send_bytes(uint8_t *data, int len) {
    for (int i = 0; i < len; i++) spi_send_byte(data[i]);
    spi_wait_idle();
}
static void lcd_send_command(uint8_t cmd) {
    lcd_dc_command();
    usleep(1);
    spi_send_byte(cmd);
    spi_wait_idle();
}
static void lcd_send_data_buf(uint8_t *data, int len) {
    lcd_dc_data();
    usleep(1);
    spi_send_bytes(data, len);
}

static void spi_init_hw(void) {
    spi_write(SPIM_SPIENR, 0);
    spi_write(SPIM_CTRLR0, 0x0007);
    spi_write(SPIM_BAUDR, 200);
    spi_write(SPIM_IMR, 0x00);
    spi_write(SPIM_TXFTLR, 0);
    spi_write(SPIM_SER, 0x01);
    spi_write(SPIM_SPIENR, 1);
    printf("[DISPLAY] SPI0 configured\n");
}

static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x08,0x2A,0x1C,0x2A,0x08}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x00,0x08,0x14,0x22,0x41}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x41,0x22,0x14,0x08,0x00}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x01,0x01}, // F
    {0x3E,0x41,0x41,0x51,0x32}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x04,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x20,0x18,0x20,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x03,0x04,0x78,0x04,0x03}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
};

static void fb_clear(void) { memset(framebuffer, 0x00, sizeof(framebuffer)); }
static void fb_set_pixel(int x, int y) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    framebuffer[(y / 8) * 128 + x] |= (1 << (y % 8));
}
static void fb_draw_char(int x, int y, char c) {
    if (c >= 'a' && c <= 'z') c = c - 32;
    if (c < ' ' || c > 'Z') c = ' ';
    int idx = c - ' ';
    if (idx < 0 || idx >= (int)(sizeof(font5x7) / sizeof(font5x7[0]))) idx = 0;
    for (int col = 0; col < 5; col++) {
        uint8_t coldata = font5x7[idx][col];
        for (int row = 0; row < 7; row++) {
            if (coldata & (1 << row)) fb_set_pixel(x + col, y + row);
        }
    }
}
static void fb_draw_string(int x, int y, const char *str) {
    while (*str) {
        if (x + 5 > 128) { x = 0; y += 9; }
        fb_draw_char(x, y, *str);
        x += 6; str++;
    }
}
static void fb_draw_string_centered(int y, const char *str) {
    int len = strlen(str);
    int x = (128 - len * 6) / 2;
    if (x < 0) x = 0;
    fb_draw_string(x, y, str);
}
static void fb_draw_hline(int x1, int x2, int y) {
    for (int x = x1; x <= x2; x++) fb_set_pixel(x, y);
}
static void fb_flush(void) {
    if (!hw_available) return;
    for (int page = 0; page < 8; page++) {
        lcd_send_command(0xB0 | page);
        lcd_send_command(0x10);
        lcd_send_command(0x00);
        lcd_send_data_buf(&framebuffer[page * 128], 128);
    }
}

// ---- Public API ----

void display_init(void) {
    printf("[DISPLAY] Initializing LCD...\n");
    
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) { printf("[DISPLAY] WARNING: Cannot open /dev/mem\n"); hw_available = 0; return; }
    
    spi_base = mmap(NULL, SPIM0_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, SPIM0_BASE);
    if (spi_base == MAP_FAILED) { hw_available = 0; close(mem_fd); mem_fd = -1; return; }
    
    gpio1_base = mmap(NULL, GPIO1_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO1_BASE);
    if (gpio1_base == MAP_FAILED) { munmap(spi_base, SPIM0_SPAN); spi_base = NULL; hw_available = 0; close(mem_fd); mem_fd = -1; return; }
    
    hw_available = 1;
    
    uint32_t ddr = gpio1_read(GPIO1_SWPORTA_DDR);
    ddr |= BIT_BK | BIT_DC | BIT_RST;
    gpio1_write(GPIO1_SWPORTA_DDR, ddr);
    
    lcd_backlight_on();
    lcd_reset_on();  usleep(50000);
    lcd_reset_off(); usleep(100000);
    
    spi_init_hw();
    
    // LCD init - try multiple scroll line values to fix split text
    lcd_send_command(0xE2); usleep(20000);  // system reset
    lcd_send_command(0xA0);  // SEG normal
    lcd_send_command(0xC8);  // COM reverse
    lcd_send_command(0xA2);  // bias 1/9
    lcd_send_command(0x2C); usleep(20000);
    lcd_send_command(0x2E); usleep(20000);
    lcd_send_command(0x2F); usleep(20000);
    lcd_send_command(0xF8);
    lcd_send_command(0x00);
    lcd_send_command(0x27);
    lcd_send_command(0x81);
    lcd_send_command(0x08);
    lcd_send_command(0xAC);
    lcd_send_command(0x00);
    lcd_send_command(0xA6);
    lcd_send_command(0x40);  // scroll line = 0 (try 0xC0 if still split)
    lcd_send_command(0xAF);
    
    fb_clear(); fb_flush();
    printf("[DISPLAY] LCD initialized\n");
}

void display_balance(int balance) {
    char buf[24];
    snprintf(buf, sizeof(buf), "BALANCE: $%d", balance);
    printf("[DISPLAY] %s\n", buf);
    if (hw_available) {
        fb_clear();
        fb_draw_hline(0, 127, 0);
        fb_draw_string_centered(4, "ROULETTE");
        fb_draw_hline(0, 127, 12);
        fb_draw_string_centered(20, buf);
        fb_draw_hline(0, 127, 30);
        fb_draw_string(2, 35, "K0:PLAY K1:RESET");
        fb_draw_string(2, 45, "K2:BAL  K3:QUIT");
        fb_flush();
    }
}

void display_message(const char *message) {
    printf("[DISPLAY] %s\n", message);
    if (hw_available) {
        fb_clear();
        fb_draw_hline(0, 127, 20);
        fb_draw_string_centered(28, message);
        fb_draw_hline(0, 127, 38);
        fb_flush();
    }
}

void display_error(const char *error) {
    printf("[DISPLAY] ERROR: %s\n", error);
    if (hw_available) {
        fb_clear();
        fb_draw_string_centered(10, "!! ERROR !!");
        fb_draw_hline(0, 127, 20);
        fb_draw_string_centered(28, error);
        fb_draw_hline(0, 127, 38);
        fb_flush();
    }
}

void display_clear(void) { if (hw_available) { fb_clear(); fb_flush(); } }

void display_welcome(void) {
    printf("[DISPLAY] Welcome screen\n");
    if (hw_available) {
        fb_clear();
        fb_draw_hline(0, 127, 0);
        fb_draw_string_centered(6, "DE-10 ROULETTE");
        fb_draw_hline(0, 127, 15);
        fb_draw_string_centered(22, "SWE-450");
        fb_draw_string_centered(34, "PRESS KEY0");
        fb_draw_string_centered(44, "TO START");
        fb_draw_hline(0, 127, 54);
        fb_flush();
    }
}

void display_bet_info(int bet_type, int bet_amount, int bet_position) {
    char line1[24], line2[24], line3[24];
    snprintf(line1, sizeof(line1), "BET: $%d", bet_amount);
    switch (bet_type) {
        case BET_TYPE_NUMBER:
            snprintf(line2, sizeof(line2), "TYPE: NUMBER %d", bet_position);
            snprintf(line3, sizeof(line3), "PAYS: %dX", PAYOUT_NUMBER); break;
        case BET_TYPE_ODD_EVEN:
            snprintf(line2, sizeof(line2), "TYPE: %s", (bet_position % 2 == 1) ? "ODD" : "EVEN");
            snprintf(line3, sizeof(line3), "PAYS: %dX", PAYOUT_ODD_EVEN); break;
        case BET_TYPE_HIGH_LOW:
            snprintf(line2, sizeof(line2), "TYPE: %s", (bet_position >= 5) ? "HIGH 5-9" : "LOW 0-4");
            snprintf(line3, sizeof(line3), "PAYS: %dX", PAYOUT_HIGH_LOW); break;
        default:
            snprintf(line2, sizeof(line2), "TYPE: ???");
            snprintf(line3, sizeof(line3), "PAYS: ???"); break;
    }
    printf("[DISPLAY] %s | %s | %s\n", line1, line2, line3);
    if (hw_available) {
        fb_clear();
        fb_draw_hline(0, 127, 0);
        fb_draw_string_centered(4, "YOUR BET");
        fb_draw_hline(0, 127, 13);
        fb_draw_string_centered(18, line1);
        fb_draw_string_centered(28, line2);
        fb_draw_string_centered(38, line3);
        fb_draw_hline(0, 127, 48);
        fb_draw_string_centered(52, "SPINNING...");
        fb_flush();
    }
}

void display_result(int won, int amount, int landed, int balance) {
    char line1[24], line2[24], line3[24];
    snprintf(line1, sizeof(line1), "LANDED: %d", landed);
    if (won) snprintf(line2, sizeof(line2), "YOU WON $%d!", amount);
    else snprintf(line2, sizeof(line2), "YOU LOST $%d", amount);
    snprintf(line3, sizeof(line3), "BAL: $%d", balance);
    printf("[DISPLAY] %s | %s | %s\n", line1, line2, line3);
    if (hw_available) {
        fb_clear();
        fb_draw_hline(0, 127, 0);
        fb_draw_string_centered(4, won ? "*** WIN ***" : "--- LOSS ---");
        fb_draw_hline(0, 127, 13);
        fb_draw_string_centered(20, line1);
        fb_draw_string_centered(32, line2);
        fb_draw_hline(0, 127, 42);
        fb_draw_string_centered(48, line3);
        fb_flush();
    }
}

void display_game_over(int final_balance) {
    char buf[24];
    snprintf(buf, sizeof(buf), "FINAL: $%d", final_balance);
    printf("[DISPLAY] GAME OVER - %s\n", buf);
    if (hw_available) {
        fb_clear();
        fb_draw_hline(0, 127, 10);
        fb_draw_string_centered(16, "GAME OVER");
        fb_draw_hline(0, 127, 26);
        fb_draw_string_centered(34, buf);
        fb_draw_string_centered(48, "K1 TO RESTART");
        fb_flush();
    }
}

void display_message_lines(const char *line1, const char *line2, const char *line3, const char *line4) {
    printf("[DISPLAY] %s | %s | %s | %s\n", line1, line2, line3, line4);
    if (hw_available) {
        fb_clear();
        fb_draw_hline(0, 127, 0);
        fb_draw_string_centered(4, line1);
        fb_draw_hline(0, 127, 13);
        fb_draw_string_centered(20, line2);
        fb_draw_string_centered(32, line3);
        fb_draw_string_centered(44, line4);
        fb_draw_hline(0, 127, 54);
        fb_flush();
    }
}

void display_cleanup(void) {
    printf("[DISPLAY] Cleaning up...\n");
    if (hw_available) {
        fb_clear(); fb_draw_string_centered(28, "GOODBYE!"); fb_flush();
        usleep(500000);
        lcd_backlight_off(); lcd_send_command(0xAE);
        spi_write(SPIM_SPIENR, 0);
    }
    if (spi_base && spi_base != MAP_FAILED) { munmap(spi_base, SPIM0_SPAN); spi_base = NULL; }
    if (gpio1_base && gpio1_base != MAP_FAILED) { munmap(gpio1_base, GPIO1_SPAN); gpio1_base = NULL; }
    if (mem_fd >= 0) { close(mem_fd); mem_fd = -1; }
    hw_available = 0;
}