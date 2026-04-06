/*
 * main.c
 * Roulette game main program - DE10-Standard
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "peripherals/led.h"
#include "peripherals/switch.h"
#include "peripherals/display.h"
#include "peripherals/hex.h"
#include "game-logic/game-logic.h"
#include "config.h"

void run_game(void);
void show_menu(void);
int  get_menu_choice(void);
int  get_bet_type(void);
int  get_bet_amount(void);
int  get_bet_position(int bet_type);
void flash_winning_led(int position);

int main(void) {
    printf("=== DE-10 Roulette Game ===\n");
    printf("SWE-450 Embedded Systems\n");
    printf("Initializing hardware...\n\n");
    run_game();
    printf("\nGame terminated\n");
    return 0;
}

void show_menu(void) {
    printf("\n========================================\n");
    printf("         MAIN MENU\n");
    printf("========================================\n");
    printf("KEY0 = Play\n");
    printf("KEY1 = Reset\n");
    printf("KEY2 = Check balance\n");
    printf("KEY3 = Quit\n");
    printf("========================================\n");
}

int get_menu_choice(void) {
    while (1) {
        if (button_is_key_pressed(0)) return 0;
        if (button_is_key_pressed(1)) return 1;
        if (button_is_key_pressed(2)) return 2;
        if (button_is_key_pressed(3)) return 3;
        usleep(50000);
    }
}

int get_bet_type(void) {
    printf("\n>>> SELECT BET TYPE <<<\n");
    printf("SW8+SW9 off = NUMBER (10x payout)\n");
    printf("SW8 on      = ODD/EVEN (2x payout)\n");
    printf("SW9 on      = HIGH/LOW (2x payout)\n");
    printf("Press KEY0 to confirm\n\n");
    
    int bet_type = BET_TYPE_NUMBER;
    int last_displayed = -1;  // track what's on LCD to avoid flicker
    
    while (1) {
        int switches = switch_read();
        int sw8 = (switches >> 8) & 1;
        int sw9 = (switches >> 9) & 1;
        
        if (sw9) {
            bet_type = BET_TYPE_HIGH_LOW;
            printf("\rType: HIGH/LOW (2x) | KEY0 to confirm  ");
            if (last_displayed != 2) {
                display_message_lines("SELECT BET TYPE",
                                      "SW8+9 OFF: NUMBER",
                                      "SW9 ON: HIGH/LOW",
                                      "> HIGH/LOW SELECTED");
                last_displayed = 2;
            }
        } else if (sw8) {
            bet_type = BET_TYPE_ODD_EVEN;
            printf("\rType: ODD/EVEN (2x) | KEY0 to confirm  ");
            if (last_displayed != 1) {
                display_message_lines("SELECT BET TYPE",
                                      "SW8+9 OFF: NUMBER",
                                      "SW8 ON: ODD/EVEN",
                                      "> ODD/EVEN SELECTED");
                last_displayed = 1;
            }
        } else {
            bet_type = BET_TYPE_NUMBER;
            printf("\rType: NUMBER (10x)  | KEY0 to confirm  ");
            if (last_displayed != 0) {
                display_message_lines("SELECT BET TYPE",
                                      "SW8+9 OFF: NUMBER",
                                      "SW8: ODD  SW9: HIGH",
                                      "> NUMBER SELECTED");
                last_displayed = 0;
            }
        }
        fflush(stdout);
        
        if (button_is_key_pressed(0)) {
            printf("\n[BET] Type confirmed: %d\n", bet_type);
            usleep(300000);
            return bet_type;
        }
        
        usleep(100000);
    }
}

int get_bet_amount(void) {
    printf("\n>>> SELECT BET AMOUNT <<<\n");
    printf("SW0=$10, SW1=$20, SW2=$50, SW3=$100\n");
    printf("Press KEY0 to confirm\n\n");
    
    int bet_amount = -1;
    int selected = -1;
    int last_displayed = -1;
    
    // Show initial instructions
    display_message_lines("SELECT BET AMOUNT",
                          "SW0=$10  SW1=$20",
                          "SW2=$50  SW3=$100",
                          "FLIP A SWITCH...");
    
    while (bet_amount == -1) {
        int switches = switch_read() & 0x0F;
        
        if (switches != 0 && (switches & (switches - 1)) == 0) {
            if (switches & 0x01)      selected = BET_AMOUNT_SW0;
            else if (switches & 0x02) selected = BET_AMOUNT_SW1;
            else if (switches & 0x04) selected = BET_AMOUNT_SW2;
            else if (switches & 0x08) selected = BET_AMOUNT_SW3;
            
            printf("\rBet: $%d | Press KEY0...  ", selected);
            fflush(stdout);
            
            if (selected != last_displayed) {
                char sel_buf[24];
                snprintf(sel_buf, sizeof(sel_buf), "> $%d SELECTED", selected);
                display_message_lines("SELECT BET AMOUNT",
                                      "SW0=$10  SW1=$20",
                                      "SW2=$50  SW3=$100",
                                      sel_buf);
                last_displayed = selected;
            }
            
            if (button_is_key_pressed(0)) {
                bet_amount = selected;
                printf("\n[BET] $%d confirmed\n", bet_amount);
            }
        } else {
            printf("\rSelect ONE switch (SW0-SW3)  ");
            fflush(stdout);
            if (last_displayed != 0) {
                display_message_lines("SELECT BET AMOUNT",
                                      "SW0=$10  SW1=$20",
                                      "SW2=$50  SW3=$100",
                                      "FLIP A SWITCH...");
                last_displayed = 0;
            }
            selected = -1;
        }
        usleep(100000);
    }
    
    usleep(300000);
    return bet_amount;
}

int get_bet_position(int bet_type) {
    int position = -1;
    
    if (bet_type == BET_TYPE_NUMBER) {
        printf("\n>>> SELECT NUMBER (0-9) <<<\n");
        printf("Flip ONE switch (SW0-SW9) and press KEY0\n\n");
        
        int last_displayed = -1;
        
        display_message_lines("SELECT NUMBER 0-9",
                              "FLIP ONE SWITCH",
                              "SW0 - SW9",
                              "FLIP A SWITCH...");
        
        while (1) {
            int switches = switch_read();
            
            if (switches != 0 && (switches & (switches - 1)) == 0) {
                for (int i = 0; i < NUM_LEDS; i++) {
                    if (switches & (1 << i)) {
                        position = i;
                        break;
                    }
                }
                
                led_set_position(position);
                printf("\rNumber: %d | Press KEY0...  ", position);
                fflush(stdout);
                
                if (position != last_displayed) {
                    char sel_buf[24];
                    snprintf(sel_buf, sizeof(sel_buf), "> NUMBER %d SELECTED", position);
                    display_message_lines("SELECT NUMBER 0-9",
                                          "FLIP ONE SWITCH",
                                          "SW0 - SW9",
                                          sel_buf);
                    last_displayed = position;
                }
                
                if (button_is_key_pressed(0)) {
                    printf("\n[BET] Number %d confirmed\n", position);
                    led_clear_all();
                    usleep(300000);
                    return position;
                }
            } else if (switches == 0) {
                printf("\rSelect a switch (SW0-SW9)   ");
                fflush(stdout);
                led_clear_all();
                if (last_displayed != -1) {
                    display_message_lines("SELECT NUMBER 0-9",
                                          "FLIP ONE SWITCH",
                                          "SW0 - SW9",
                                          "FLIP A SWITCH...");
                    last_displayed = -1;
                }
            } else {
                printf("\rSelect only ONE switch      ");
                fflush(stdout);
                led_clear_all();
            }
            usleep(100000);
        }
        
    } else if (bet_type == BET_TYPE_ODD_EVEN) {
        printf("\n>>> SELECT ODD OR EVEN <<<\n");
        printf("SW0 = EVEN, SW1 = ODD\n");
        printf("Press KEY0 to confirm\n\n");
        
        int last_displayed = -1;
        
        display_message_lines("ODD OR EVEN?",
                              "SW0 = EVEN",
                              "SW1 = ODD",
                              "FLIP A SWITCH...");
        
        while (1) {
            int switches = switch_read() & 0x03;
            
            if (switches == 0x01) {
                position = 0;
                printf("\rChoice: EVEN | Press KEY0...  ");
                fflush(stdout);
                if (last_displayed != 0) {
                    display_message_lines("ODD OR EVEN?",
                                          "SW0 = EVEN",
                                          "SW1 = ODD",
                                          "> EVEN SELECTED");
                    last_displayed = 0;
                }
            } else if (switches == 0x02) {
                position = 1;
                printf("\rChoice: ODD  | Press KEY0...  ");
                fflush(stdout);
                if (last_displayed != 1) {
                    display_message_lines("ODD OR EVEN?",
                                          "SW0 = EVEN",
                                          "SW1 = ODD",
                                          "> ODD SELECTED");
                    last_displayed = 1;
                }
            } else {
                printf("\rFlip SW0 (EVEN) or SW1 (ODD)  ");
                fflush(stdout);
                position = -1;
                if (last_displayed != -1) {
                    display_message_lines("ODD OR EVEN?",
                                          "SW0 = EVEN",
                                          "SW1 = ODD",
                                          "FLIP A SWITCH...");
                    last_displayed = -1;
                }
            }
            
            if (position >= 0 && button_is_key_pressed(0)) {
                printf("\n[BET] %s confirmed\n", position ? "ODD" : "EVEN");
                usleep(300000);
                return position;
            }
            usleep(100000);
        }
        
    } else if (bet_type == BET_TYPE_HIGH_LOW) {
        printf("\n>>> SELECT HIGH OR LOW <<<\n");
        printf("SW0 = LOW (0-4), SW1 = HIGH (5-9)\n");
        printf("Press KEY0 to confirm\n\n");
        
        int last_displayed = -1;
        
        display_message_lines("HIGH OR LOW?",
                              "SW0 = LOW 0-4",
                              "SW1 = HIGH 5-9",
                              "FLIP A SWITCH...");
        
        while (1) {
            int switches = switch_read() & 0x03;
            
            if (switches == 0x01) {
                position = 0;
                printf("\rChoice: LOW (0-4) | Press KEY0...  ");
                fflush(stdout);
                if (last_displayed != 0) {
                    display_message_lines("HIGH OR LOW?",
                                          "SW0 = LOW 0-4",
                                          "SW1 = HIGH 5-9",
                                          "> LOW SELECTED");
                    last_displayed = 0;
                }
            } else if (switches == 0x02) {
                position = 1;
                printf("\rChoice: HIGH (5-9) | Press KEY0... ");
                fflush(stdout);
                if (last_displayed != 1) {
                    display_message_lines("HIGH OR LOW?",
                                          "SW0 = LOW 0-4",
                                          "SW1 = HIGH 5-9",
                                          "> HIGH SELECTED");
                    last_displayed = 1;
                }
            } else {
                printf("\rFlip SW0 (LOW) or SW1 (HIGH)      ");
                fflush(stdout);
                position = -1;
                if (last_displayed != -1) {
                    display_message_lines("HIGH OR LOW?",
                                          "SW0 = LOW 0-4",
                                          "SW1 = HIGH 5-9",
                                          "FLIP A SWITCH...");
                    last_displayed = -1;
                }
            }
            
            if (position >= 0 && button_is_key_pressed(0)) {
                printf("\n[BET] %s confirmed\n", position ? "HIGH (5-9)" : "LOW (0-4)");
                usleep(300000);
                return position;
            }
            usleep(100000);
        }
    }
    
    return 0;
}

void flash_winning_led(int position) {
    printf("[LED] Flashing position %d\n", position);
    for (int i = 0; i < 6; i++) {
        led_set_position(position);
        usleep(250000);
        led_clear_all();
        usleep(250000);
    }
    led_set_position(position);
    usleep(500000);
}

void run_game(void) {
    printf("[MAIN] Starting initialization...\n");
    
    led_init();
    switch_init();
    display_init();
    hex_init();
    
    GameState game = game_init();
    
    display_welcome();
    hex_display_number(game.balance);
    
    printf("\n[MAIN] ========================================\n");
    printf("[MAIN] DE-10 Roulette Game - READY!\n");
    printf("[MAIN] Starting Balance: $%d\n", game.balance);
    printf("[MAIN] ========================================\n\n");
    
    int running = 1;
    
    while (running && game.is_game_active) {
        show_menu();
        display_balance(game.balance);
        
        int choice = get_menu_choice();
        
        switch (choice) {
            case 0: {
                printf("\n[GAME] === Starting Round %d ===\n", game.total_rounds + 1);
                printf("Balance: $%d\n", game.balance);
                
                int bet_type = get_bet_type();
                game.bet_type = bet_type;
                
                int bet_amount = get_bet_amount();
                game.bet_amount = bet_amount;
                
                if (!game_can_afford_bet(&game)) {
                    printf("\n[ERROR] Insufficient funds!\n");
                    display_error("LOW FUNDS!");
                    usleep(2000000);
                    continue;
                }
                
                int bet_position = get_bet_position(bet_type);
                game.bet_position = bet_position;
                
                display_bet_info(bet_type, bet_amount, bet_position);
                
                printf("\n[GAME] Spinning...\n");
                printf("Bet: $%d, Type: %d, Position: %d\n",
                       game.bet_amount, game.bet_type, game.bet_position);
                
                int final_position = game_generate_random_position();
                led_spin_animation(final_position);
                
                printf("\n[GAME] Landed on: %d\n", final_position);
                
                int won = game_process_round(&game, final_position);
                flash_winning_led(final_position);
                led_clear_all();
                
                int display_amount;
                if (won) {
                    display_amount = game.bet_amount * game_get_payout(game.bet_type);
                    printf("\n*** YOU WON $%d! ***\n", display_amount);
                } else {
                    display_amount = game.bet_amount;
                    printf("\n*** YOU LOST $%d ***\n", display_amount);
                }
                
                display_result(won, display_amount, final_position, game.balance);
                hex_display_number(game.balance);
                
                printf("Balance: $%d | Rounds: %d | Wins: %d\n",
                       game.balance, game.total_rounds, game.total_wins);
                
                if (game.balance <= 0) {
                    printf("\n========================================\n");
                    printf("         GAME OVER\n");
                    printf("========================================\n");
                    display_game_over(game.balance);
                    game.is_game_active = 0;
                    usleep(3000000);
                }
                
                usleep(2000000);
                break;
            }
                
            case 1:
                printf("\n[GAME] Resetting...\n");
                game_reset(&game);
                display_welcome();
                hex_display_number(game.balance);
                printf("Reset complete\n");
                usleep(1000000);
                break;
                
            case 2:
                printf("\n[GAME] Balance: $%d | Rounds: %d | Wins: %d\n",
                       game.balance, game.total_rounds, game.total_wins);
                display_balance(game.balance);
                hex_display_number(game.balance);
                usleep(1500000);
                break;
                
            case 3:
                printf("\n[GAME] Quitting...\n");
                running = 0;
                break;
                
            default:
                break;
        }
    }
    
    if (!game.is_game_active) {
        printf("\n========================================\n");
        printf("  GAME OVER - Final Balance: $%d\n", game.balance);
        printf("  Rounds: %d | Wins: %d\n", game.total_rounds, game.total_wins);
        printf("========================================\n");
        display_game_over(game.balance);
    }
    
    printf("\n[MAIN] Cleaning up...\n");
    led_cleanup();
    switch_cleanup();
    display_cleanup();
    hex_cleanup();
    printf("[MAIN] Done\n");
}