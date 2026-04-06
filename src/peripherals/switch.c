/*
 * switch.c
 * Switch and button input using memory-mapped I/O
 */

#include "peripherals/switch.h"
#include "../lib/address_map_arm.h"
#include "../includes/config.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

static void *virtual_base = NULL;
static volatile uint32_t *switch_ptr = NULL;
static volatile uint32_t *button_ptr = NULL;
static int fd = -1;

void switch_init(void) {
    printf("[SWITCH] Opening /dev/mem...\n");
    
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("[SWITCH] ERROR: could not open /dev/mem\n");
        return;
    }
    
    printf("[SWITCH] Mapping memory...\n");
    
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE), 
                       MAP_SHARED, fd, HW_REGS_BASE);
    
    if (virtual_base == MAP_FAILED) {
        printf("[SWITCH] ERROR: mmap() failed\n");
        close(fd);
        fd = -1;
        return;
    }
    
    // Calculate switch address
    switch_ptr = (uint32_t *)(virtual_base + 
                 ((unsigned long)(ALT_LWFPGASLVS_OFST + DIPSW_PIO_BASE) & 
                  (unsigned long)(HW_REGS_MASK)));
    
    // Calculate button address
    button_ptr = (uint32_t *)(virtual_base + 
                 ((unsigned long)(ALT_LWFPGASLVS_OFST + BUTTON_PIO_BASE) & 
                  (unsigned long)(HW_REGS_MASK)));
    
    printf("[SWITCH] Initialization complete\n");
}

int switch_read(void) {
    if (switch_ptr == NULL) {
        printf("[SWITCH] ERROR: Switch not initialized\n");
        return 0;
    }
    
    // Read all 10 switches
    uint32_t switch_value = *switch_ptr & 0x3FF;
    
    return (int)switch_value;
}

int switch_get_bet_amount(void) {
    if (switch_ptr == NULL) {
        printf("[SWITCH] ERROR: Switch not initialized\n");
        return -1;
    }
    
    int switch_value = switch_read();
    
    if (!switch_is_valid_selection()) {
        return -1;
    }
    
    // Check which switch is active
    if (switch_value & 0x01) return BET_AMOUNT_SW0;
    if (switch_value & 0x02) return BET_AMOUNT_SW1;
    if (switch_value & 0x04) return BET_AMOUNT_SW2;
    if (switch_value & 0x08) return BET_AMOUNT_SW3;
    
    return -1;
}

int switch_is_valid_selection(void) {
    int switch_value = switch_read();
    
    // Check if exactly one switch is active
    if (switch_value != 0 && (switch_value & (switch_value - 1)) == 0) {
        return 1;
    }
    
    return 0;
}

int button_read(void) {
    if (button_ptr == NULL) {
        return 0x0F;
    }
    
    // Buttons are active-low
    uint32_t button_value = *button_ptr & 0x0F;
    
    return (int)button_value;
}

int button_is_spin_pressed(void) {
    int button_value = button_read();
    
    if ((button_value & (1 << BUTTON_SPIN)) == 0) {
        return 1;
    }
    
    return 0;
}

int button_is_reset_pressed(void) {
    int button_value = button_read();
    
    if ((button_value & (1 << BUTTON_RESET)) == 0) {
        return 1;
    }
    
    return 0;
}

int button_is_key_pressed(int key_number) {
    if (key_number < 0 || key_number > 3) {
        return 0;
    }
    
    static int last_state[4] = {1, 1, 1, 1};
    
    int button_value = button_read();
    int current_state = (button_value >> key_number) & 0x01;
    
    // Detect button press
    if (last_state[key_number] == 1 && current_state == 0) {
        usleep(DEBOUNCE_DELAY_MS * 1000);
        
        button_value = button_read();
        current_state = (button_value >> key_number) & 0x01;
        
        if (current_state == 0) {
            // Wait for release
            while (((button_read() >> key_number) & 0x01) == 0) {
                usleep(10000);
            }
            
            usleep(DEBOUNCE_DELAY_MS * 1000);
            
            last_state[key_number] = 1;
            return 1;
        }
    }
    
    last_state[key_number] = current_state;
    
    return 0;
}

void button_wait_for_press(int button_mask) {
    // Wait for press
    while ((button_read() & button_mask) != 0) {
        usleep(10000);
    }
    
    usleep(DEBOUNCE_DELAY_MS * 1000);
    
    // Wait for release
    while ((button_read() & button_mask) == 0) {
        usleep(10000);
    }
    
    usleep(DEBOUNCE_DELAY_MS * 1000);
}

void switch_cleanup(void) {
    printf("[SWITCH] Cleaning up...\n");
    
    if (virtual_base != NULL) {
        munmap(virtual_base, HW_REGS_SPAN);
        virtual_base = NULL;
        switch_ptr = NULL;
        button_ptr = NULL;
    }
    
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    
    printf("[SWITCH] Cleanup complete\n");
}
