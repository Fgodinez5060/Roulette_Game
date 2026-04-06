/*
 * display.h
 * LCD display interface for DE10-Standard 128x64 LCM module
 */

#ifndef DISPLAY_H
#define DISPLAY_H

void display_init(void);
void display_balance(int balance);
void display_message(const char *message);
void display_error(const char *error);
void display_clear(void);
void display_welcome(void);
void display_bet_info(int bet_type, int bet_amount, int bet_position);
void display_result(int won, int amount, int landed, int balance);
void display_game_over(int final_balance);
void display_message_lines(const char *line1, const char *line2, const char *line3, const char *line4);
void display_cleanup(void);

#endif // DISPLAY_H