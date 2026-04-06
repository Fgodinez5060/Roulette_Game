/*
 * config.h
 * Configuration constants for the roulette game
 * 
 * Updated for Milestone 5: multiple bet types, LFSR, LCD
 */

#ifndef CONFIG_H
#define CONFIG_H

// Game configuration constants
#define INITIAL_BALANCE       100    // Starting player balance
#define NUM_LEDS              10     // Number of LEDs on the roulette wheel
#define LED_SPIN_DELAY_MS     100    // Delay between LED transitions (milliseconds)
#define TOTAL_SPIN_TIME_MS    3000   // Total duration of LED animation (3 seconds)
#define DEBOUNCE_DELAY_MS     50     // Switch debounce delay

// Bet amounts based on switch positions (SW0-SW3)
#define BET_AMOUNT_SW0        10     // Bet for switch 0
#define BET_AMOUNT_SW1        20     // Bet for switch 1
#define BET_AMOUNT_SW2        50     // Bet for switch 2
#define BET_AMOUNT_SW3        100    // Bet for switch 3

// Bet types - selected via SW8-SW9
#define BET_TYPE_NUMBER       0      // Specific number (1 in 10 odds, 10x payout)
#define BET_TYPE_ODD_EVEN     1      // Odd/Even (1 in 2 odds, 2x payout)
#define BET_TYPE_HIGH_LOW     2      // High(5-9)/Low(0-4) (1 in 2 odds, 2x payout)

// Payout multipliers per bet type
#define PAYOUT_NUMBER         10     // Specific number: 10x payout
#define PAYOUT_ODD_EVEN       2      // Odd/Even: 2x payout
#define PAYOUT_HIGH_LOW       2      // High/Low: 2x payout

// Button assignments
#define BUTTON_SPIN           0      // KEY0 - Start spin / confirm
#define BUTTON_RESET          1      // KEY1 - Reset game
#define BUTTON_CHECK          2      // KEY2 - Check balance
#define BUTTON_QUIT           3      // KEY3 - Quit

#define LFSR_PIO_BASE         0x6000 // LFSR random number generator

// LCD SPI configuration
#define LCD_SPI_DEVICE        "/dev/spidev0.0"
#define LCD_SPI_SPEED         1000000  // 1 MHz SPI clock
#define LCD_SPI_MODE          0        // SPI Mode 0
#define LCD_WIDTH             128
#define LCD_HEIGHT            64

#define HPS_GPIO1_BASE        0xFF709000
#define HPS_GPIO1_SPAN        0x1000

#define LCD_BK_BIT            8    // GPIO37 = backlight
#define LCD_DC_BIT            12   // GPIO41 = data/command
#define LCD_RST_BIT           15   // GPIO44 = reset (active low)

#endif // CONFIG_H
