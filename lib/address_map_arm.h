/* 
 * address_map_arm.h
 * Memory-mapped addresses for DE10-Standard peripherals
 */

#ifndef ADDRESS_MAP_ARM_H
#define ADDRESS_MAP_ARM_H

#include <stdint.h>

/* HPS peripheral base */
#define HW_REGS_BASE           0xFC000000 
#define HW_REGS_SPAN           0x04000000
#define HW_REGS_MASK           (HW_REGS_SPAN - 1)

/* FPGA lightweight bridge offset */
#define ALT_LWFPGASLVS_OFST    0xFF200000

/* Peripheral base addresses (offsets for lightweight bridge) */
#define LED_PIO_BASE           0x3000      // LED base address
#define DIPSW_PIO_BASE         0x4000      // DIP switch base address  
#define BUTTON_PIO_BASE        0x5000      // Push button base address
#define HEX_PIO_BASE           0x40070     // HEX display base address
#define LFSR_PIO_BASE          0x6000      // LFSR RNG base address (update after Qsys)

/* Data widths */
#define LED_PIO_DATA_WIDTH     10          // 10 LEDs
#define DIPSW_PIO_DATA_WIDTH   10          // 10 switches
#define BUTTON_PIO_DATA_WIDTH  4           // 4 push buttons

/* HPS GPIO1 controller (for LCD pins) */
#define HPS_GPIO1_BASE_ADDR    0xFF709000
#define HPS_GPIO1_DIR_REG      0x04        // Direction register offset
#define HPS_GPIO1_DATA_REG     0x00        // Data register offset

#endif // ADDRESS_MAP_ARM_H
