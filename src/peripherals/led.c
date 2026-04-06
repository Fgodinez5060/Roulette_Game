/*
 * led.c
 * LED control using memory-mapped I/O
 */

#include "peripherals/led.h"
#include "../lib/address_map_arm.h"
#include "../includes/config.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static void *virtual_base = NULL;
static volatile uint32_t *led_ptr = NULL;
static int fd = -1;

void led_init(void) {
    // Open /dev/mem
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("[LED] ERROR: could not open /dev/mem\n");
        return;
    }
    
    // Map memory
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE), 
                       MAP_SHARED, fd, HW_REGS_BASE);
    
    if (virtual_base == MAP_FAILED) {
        printf("[LED] ERROR: mmap() failed\n");
        close(fd);
        fd = -1;
        return;
    }
    
    // Calculate LED address
    led_ptr = (uint32_t *)(virtual_base + 
              ((unsigned long)(ALT_LWFPGASLVS_OFST + LED_PIO_BASE) & 
               (unsigned long)(HW_REGS_MASK)));
    
    // Turn off all LEDs
    *led_ptr = 0x3FF;
    
    printf("[LED] Initialization complete\n");
}

int led_set_position(int position) {
    if (led_ptr == NULL) {
        printf("[LED] ERROR: LED not initialized\n");
        return -1;
    }
    
    if (position < 0 || position >= NUM_LEDS) {
        printf("[LED] ERROR: Invalid position %d\n", position);
        return -1;
    }
    
    // Inverted logic: clear bit to turn LED on
    uint32_t led_mask = ~(1 << position);
    *led_ptr = led_mask;
    
    return 0;
}

void led_clear_all(void) {
    if (led_ptr == NULL) {
        printf("[LED] ERROR: LED not initialized\n");
        return;
    }
    
    *led_ptr = 0x3FF;
}

int led_spin_animation(int final_position) {
    if (led_ptr == NULL) {
        printf("[LED] ERROR: LED not initialized\n");
        return -1;
    }
    
    if (final_position < 0 || final_position >= NUM_LEDS) {
        printf("[LED] ERROR: Invalid final position %d\n", final_position);
        return -1;
    }
    
    int num_transitions = TOTAL_SPIN_TIME_MS / LED_SPIN_DELAY_MS;
    int current_position = 0;
    
    printf("[LED] Starting spin animation...\n");
    
    // Spin through LEDs
    for (int i = 0; i < num_transitions; i++) {
        led_set_position(current_position);
        usleep(LED_SPIN_DELAY_MS * 1000);
        current_position = (current_position + 1) % NUM_LEDS;
    }
    
    // Stop on final position
    led_set_position(final_position);
    
    printf("[LED] Spin complete at position %d\n", final_position);
    return 0;
}

void led_cleanup(void) {
    printf("[LED] Cleaning up...\n");
    
    if (led_ptr != NULL) {
        *led_ptr = 0x3FF;
    }
    
    if (virtual_base != NULL) {
        munmap(virtual_base, HW_REGS_SPAN);
        virtual_base = NULL;
        led_ptr = NULL;
    }
    
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    
    printf("[LED] Cleanup complete\n");
}
