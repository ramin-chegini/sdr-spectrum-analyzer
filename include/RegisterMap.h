#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <stdint.h>

/* Physical Base Address */
#define FPGA_REG_BASE      0x43C00000
#define FPGA_REG_SIZE      0x1000

/* Register Offsets */

#define REG_CONTROL        0x00
#define REG_MODE           0x04
#define REG_FILTER         0x08
#define REG_PARAM0         0x0C
#define REG_PARAM1         0x10
#define REG_STATUS         0x14
#define REG_VERSION        0x18

/* CONTROL Register Bits */

#define CTRL_ENABLE        (1<<0)
#define CTRL_RESET         (1<<1)

#endif
