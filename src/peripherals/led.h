/*
 * led.h
 * LED control interface for roulette wheel visualization
 */

#ifndef LED_H
#define LED_H

/*
 * Initialize LED hardware
 * Sets up memory mapping for LED control registers
 */
void led_init(void);

/*
 * Light up a specific LED position on the roulette wheel
 */
int led_set_position(int position);

/*
 * Turn off all LEDs
 */
void led_clear_all(void);

/*
 * Animate the roulette wheel spin
 * LEDs light up sequentially until landing on final position
 */
int led_spin_animation(int final_position);

/*
 * Clean up LED resources
 */
void led_cleanup(void);

#endif // LED_H
