
#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <stdint.h>

/* Physical Base Address */
#define FPGA_REG_BASE      0x43C00000
#define FPGA_REG_SIZE      0x1000

/* Register Offsets */

#define REG_CONTROL              0x00
#define REG_BANDWIDTH            0x04
#define REG_PSD_RATE             0x08
#define REG_MAXHOLD_DELAY        0x0C
#define REG_LED_TIMER            0x10

/* CONTROL Register Bits */

#define CTRL_SOFT_RESET          (1u << 0)
#define CTRL_PSD_START           (1u << 1)
#define CTRL_PSD_CAPTURE_START   (1u << 2)
#define CTRL_MAXHOLD_ENABLE      (1u << 3)
#define CTRL_RXCH_SELECT         (1u << 4)

/* BANDWIDTH Register */

#define BANDWIDTH_MASK           0x0000000F

/* PSD RATE Register */

#define PSD_RATE_MASK            0x00000FFF

#endif
