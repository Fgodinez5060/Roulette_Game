/*
 * game-logic.c
 * Game logic implementation with multiple bet types
 * 
 * Bet types:
 *   NUMBER   - Pick exact position (0-9), pays 10x
 *   ODD/EVEN - Bet on odd or even result, pays 2x
 *   HIGH/LOW - Bet on high (5-9) or low (0-4), pays 2x
 *
 * Random number generation:
 *   Attempts to read from FPGA LFSR via memory-mapped I/O.
 *   Falls back to C rand() if LFSR is not accessible.
 */

#include "game-logic/game-logic.h"
#include "../includes/config.h"
#include "../lib/address_map_arm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// LFSR hardware access
static void *lfsr_virtual_base = NULL;
static volatile uint32_t *lfsr_ptr = NULL;
static int lfsr_fd = -1;
static int lfsr_available = 0;

static void lfsr_init(void) {
    lfsr_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (lfsr_fd < 0) {
        printf("[GAME] LFSR: /dev/mem not available, using rand()\n");
        lfsr_available = 0;
        return;
    }
    
    lfsr_virtual_base = mmap(NULL, HW_REGS_SPAN, PROT_READ | PROT_WRITE,
                             MAP_SHARED, lfsr_fd, HW_REGS_BASE);
    
    if (lfsr_virtual_base == MAP_FAILED) {
        printf("[GAME] LFSR: mmap failed, using rand()\n");
        close(lfsr_fd);
        lfsr_fd = -1;
        lfsr_available = 0;
        return;
    }
    
    lfsr_ptr = (uint32_t *)(lfsr_virtual_base +
               ((unsigned long)(ALT_LWFPGASLVS_OFST + LFSR_PIO_BASE) &
                (unsigned long)(HW_REGS_MASK)));
    
    // Test read - if the address is valid, return a non-zero value
    // (LFSR is seeded with 0xDEADBEEF and free-runs)
    uint32_t test_val = *lfsr_ptr;
    if (test_val != 0) {
        lfsr_available = 1;
        printf("[GAME] LFSR: Hardware RNG available (test read: 0x%08X)\n", test_val);
    } else {
        // fall back if address unreachable
        printf("[GAME] LFSR: Read returned 0, falling back to rand()\n");
        lfsr_available = 0;
    }
}

GameState game_init(void) {
    GameState state;
    state.balance = INITIAL_BALANCE;
    state.bet_amount = 0;
    state.bet_position = 0;
    state.bet_type = BET_TYPE_NUMBER;
    state.is_game_active = 1;
    state.total_rounds = 0;
    state.total_wins = 0;
    
    // Seed C rand() as fallback
    srand(time(NULL));
    
    // Try to initialize LFSR hardware
    lfsr_init();
    
    printf("[GAME] Game initialized with balance: $%d\n", state.balance);
    return state;
}

int game_generate_random_position(void) {
    int position;
    
    if (lfsr_available && lfsr_ptr != NULL) {
        // Read from FPGA LFSR - take modulo NUM_LEDS
        uint32_t lfsr_value = *lfsr_ptr;
        position = (int)(lfsr_value % NUM_LEDS);
        printf("[GAME] LFSR random: 0x%08X -> position %d\n", lfsr_value, position);
    } else {
        // Fallback to C rand()
        position = rand() % NUM_LEDS;
        printf("[GAME] rand() position: %d\n", position);
    }
    
    return position;
}

int game_get_payout(int bet_type) {
    switch (bet_type) {
        case BET_TYPE_NUMBER:    return PAYOUT_NUMBER;
        case BET_TYPE_ODD_EVEN:  return PAYOUT_ODD_EVEN;
        case BET_TYPE_HIGH_LOW:  return PAYOUT_HIGH_LOW;
        default:                 return 2;
    }
}

int game_check_win(int bet_type, int bet_position, int final_position) {
    switch (bet_type) {
        case BET_TYPE_NUMBER:
            // Exact match
            return (bet_position == final_position) ? 1 : 0;
            
        case BET_TYPE_ODD_EVEN:
            // bet_position stores the parity the player chose:
            //   0 = player bet EVEN, 1 = player bet ODD
            return ((final_position % 2) == (bet_position % 2)) ? 1 : 0;
            
        case BET_TYPE_HIGH_LOW:
            // bet_position: 0 = LOW (0-4), 1 = HIGH (5-9)
            if (bet_position == 0) {
                return (final_position < 5) ? 1 : 0;
            } else {
                return (final_position >= 5) ? 1 : 0;
            }
            
        default:
            return 0;
    }
}

int game_process_round(GameState *state, int final_position) {
    state->total_rounds++;
    
    printf("[GAME] Round %d - Type: %d, Position: %d, Final: %d\n",
           state->total_rounds, state->bet_type, state->bet_position, final_position);
    
    int won = game_check_win(state->bet_type, state->bet_position, final_position);
    
    if (won) {
        int payout = game_get_payout(state->bet_type);
        int winnings = state->bet_amount * payout;
        state->balance += winnings;
        state->total_wins++;
        printf("[GAME] WIN! +$%d (payout %dx, balance: $%d)\n",
               winnings, payout, state->balance);
        return 1;
    } else {
        state->balance -= state->bet_amount;
        printf("[GAME] LOSS! -$%d (balance: $%d)\n",
               state->bet_amount, state->balance);
        
        if (state->balance <= 0) {
            state->is_game_active = 0;
            printf("[GAME] Game over - out of funds\n");
        }
        
        return 0;
    }
}

int game_can_afford_bet(GameState *state) {
    int can_afford = (state->balance >= state->bet_amount) ? 1 : 0;
    
    if (!can_afford) {
        printf("[GAME] Cannot afford $%d (balance: $%d)\n",
               state->bet_amount, state->balance);
    }
    
    return can_afford;
}

void game_reset(GameState *state) {
    state->balance = INITIAL_BALANCE;
    state->bet_amount = 0;
    state->bet_position = 0;
    state->bet_type = BET_TYPE_NUMBER;
    state->is_game_active = 1;
    state->total_rounds = 0;
    state->total_wins = 0;
    
    printf("[GAME] Game reset (balance: $%d)\n", INITIAL_BALANCE);
}
