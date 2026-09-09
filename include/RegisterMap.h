#ifndef REGISTER_MAP_H
#define REGISTER_MAP_H

#include <stdint.h>

/* Physical Base Address */
#define FPGA_REG_BASE      0x43C00000
#define FPGA_REG_SIZE      0x1000

/* Register Offsets */

#define REG_CONTROL        0x00
#define REG_BANDWIDTH      0x04
#define REG_MODE           0x08
#define REG_LED_TIMER      0x0C
#define REG_MESSAGE        0x10
#define REG_COEFFICIENTS   0x14
#define REG_REF1           0x18
#define REG_REF2           0x1C

/*REG1 / 0x04
bit 3:0  Bandwidth_Select

REG2 / 0x08
bit 7:0  Mode_Select

REG3 / 0x0C
bit 31:0 Leds_timer_freq_sys

REG4 / 0x10
bit 31:0 Msg_Value

REG5 / 0x14
bit 7:0   A_Coeff1
bit 15:8  A_Coeff2

REG6 / 0x18
bit 23:0 Ref1

REG7 / 0x1C
bit 23:0 Ref2*/

/* CONTROL Register Bits */

/*REG0 / 0x00
bit 0  Soft_Reset
bit 1  Soft_Start
bit 2  Send_Packet
bit 3  Mod_Enable
bit 4  Squelch_Enable
bit 5  MGC2_On
bit 6  AGC1_On
bit 7  AGC2_On
bit 8  Amp_OK*/

#define CTRL_SOFT_RESET       (1u << 0)
#define CTRL_SOFT_START       (1u << 1)
#define CTRL_SEND_PACKET      (1u << 2)
#define CTRL_MOD_ENABLE       (1u << 3)
#define CTRL_SQUELCH_ENABLE   (1u << 4)
#define CTRL_MGC2_ON          (1u << 5)
#define CTRL_AGC1_ON          (1u << 6)
#define CTRL_AGC2_ON          (1u << 7)
#define CTRL_AMP_OK           (1u << 8)

/* BANDWIDTH Register */

#define BANDWIDTH_MASK        0x0000000F

/* MODE Register */

#define MODE_SELECT_MASK      0x000000FF

/* COEFFICIENTS Register */

#define A_COEFF1_MASK         0x000000FF
#define A_COEFF2_MASK         0x0000FF00

/* Reference Registers */

#define REF1_MASK             0x00FFFFFF
#define REF2_MASK             0x00FFFFFF

#endif
