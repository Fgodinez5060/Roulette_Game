/*
 * hex.c
 * 7-segment HEX display driver with BCD conversion
 */

#include "peripherals/hex.h"
#include "../lib/address_map_arm.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static void *virtual_base = NULL;
static volatile uint32_t *hex_ptr = NULL;
static int fd = -1;

// Convert decimal number to BCD (Binary Coded Decimal)
static uint32_t decimal_to_bcd(int number) {
    uint32_t bcd = 0;
    int shift = 0;
    
    if (number < 0) number = 0;
    if (number > 999999) number = 999999;
    
    while (number > 0 && shift < 24) {
        int digit = number % 10;
        bcd |= (digit << shift);
        shift += 4;
        number /= 10;
    }
    
    return bcd;
}

void hex_init(void) {
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("Error: could not open /dev/mem for HEX\n");
        return;
    }
    
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE),
                       MAP_SHARED, fd, HW_REGS_BASE);
    
    if (virtual_base == MAP_FAILED) {
        printf("Error: mmap() failed for HEX\n");
        close(fd);
        fd = -1;
        return;
    }
    
    hex_ptr = (uint32_t *)(virtual_base +
              ((unsigned long)(ALT_LWFPGASLVS_OFST + HEX_PIO_BASE) &
               (unsigned long)(HW_REGS_MASK)));
    
    hex_clear();
    printf("HEX display initialized\n");
}

void hex_display_number(int number) {
    if (hex_ptr == NULL) {
        return;
    }
    
    uint32_t bcd = decimal_to_bcd(number);
    *hex_ptr = bcd;
}

void hex_clear(void) {
    if (hex_ptr != NULL) {
        *hex_ptr = 0x000000;
    }
}

void hex_cleanup(void) {
    hex_clear();
    
    if (virtual_base != NULL) {
        munmap(virtual_base, HW_REGS_SPAN);
        virtual_base = NULL;
        hex_ptr = NULL;
    }
    
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}
