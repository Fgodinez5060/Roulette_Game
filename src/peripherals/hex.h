/*
 * hex.h
 * 7-segment HEX display control
 */

#ifndef HEX_H
#define HEX_H

#include <stdint.h>

// Initialize HEX displays
void hex_init(void);

// Display a number on HEX (up to 6 digits)
void hex_display_number(int number);

// Clear all HEX displays
void hex_clear(void);

// Cleanup HEX resources
void hex_cleanup(void);

#endif // HEX_H
