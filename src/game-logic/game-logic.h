/*
 * game-logic.h
 * Core game logic for roulette gameplay
 * Updated: multiple bet types with realistic payouts
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

// Game state structure
typedef struct {
    int balance;           // Player's current balance
    int bet_amount;        // Current bet amount
    int bet_position;      // Position player bet on (0-9)
    int bet_type;          // Bet type: 0=number, 1=odd/even, 2=high/low
    int is_game_active;    // Flag: 1 = game running, 0 = game over
    int total_rounds;      // Rounds played
    int total_wins;        // Total wins
} GameState;

// Initialize game state 
GameState game_init(void);

// Generate random position (0-9), tries FPGA LFSR first, falls back to rand()
int game_generate_random_position(void);

// Process round - returns 1 if won, 0 if lost. Updates balance.
int game_process_round(GameState *state, int final_position);

// Check if player can afford the current bet
int game_can_afford_bet(GameState *state);

// Reset game state to initial values 
void game_reset(GameState *state);

// Get payout multiplier for current bet type
int game_get_payout(int bet_type);

// Check if bet wins based on type
int game_check_win(int bet_type, int bet_position, int final_position);

#endif // GAME_LOGIC_H
