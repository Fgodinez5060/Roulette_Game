/*
 * switch.h
 * Switch input interface for bet selection
 */

#ifndef SWITCH_H
#define SWITCH_H

/*
 * Initialize switch hardware
 * Sets up memory mapping for switch input registers
 */
void switch_init(void);

/*
 * Read the current state of all switches
 */
int switch_read(void);

/*
 * Get the selected bet amount based on switch position
 * Only one switch should be active at a time
 */
int switch_get_bet_amount(void);

/*
 * Check if exactly one switch is active (valid bet selection)
 */
int switch_is_valid_selection(void);

// Read button states (active-low)
int button_read(void);

// Check if spin button (KEY0) pressed
int button_is_spin_pressed(void);

// Check if reset button (KEY3) pressed
int button_is_reset_pressed(void);

// Check if specific key pressed (0-3)
int button_is_key_pressed(int key_number);

// Wait for button press
void button_wait_for_press(int button_mask);

/*
 * Clean up switch resources
 */
void switch_cleanup(void);

#endif // SWITCH_H
