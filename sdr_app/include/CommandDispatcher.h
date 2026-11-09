#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <stdint.h>
#include <stddef.h>
#include "dma.h"

/*
 * ============================================================
 * L2 COMMAND PROTOCOL
 * ============================================================
 *
 * Ethernet L2 Frame
 *
 * Byte  0 ..  5  : Destination MAC
 * Byte  6 .. 11  : Source MAC
 * Byte 12 .. 15  : Ethernet / L2 Header fields
 *
 * Byte 16 (17th byte)
 * -------------------
 * Module / Command Selector
 *
 * This byte determines the target functional block or command
 * group.
 *
 * Current assignment:
 *
 *   0x03 -> DEMODULATOR
 *
 * Direct/system commands use Byte 16 directly as the command.
 *
 *
 * ============================================================
 * 1. DIRECT / SYSTEM COMMANDS
 * ============================================================
 *
 *   Byte 16 = 0x06 -> Soft Reset
 *   Byte 16 = 0x0F -> Mod Enable
 *   Byte 16 = 0x56 -> Send Packet
 *   Byte 16 = 0x4B -> LED Timer
 *
 *
 * Example - Soft Reset:
 *
 *   06 00 00 00 00 00 00 00 01 00 16
 *
 *
 * Example - Soft Start:
 *
 *   0F 00 00 00 00 00 00 00 01 00 16
 *
 *
 * ============================================================
 * 2. DEMODULATOR COMMANDS
 * ============================================================
 *
 * Byte 16 = 0x03
 *
 *   0x03 -> DEMODULATOR module
 *
 * Command structure:
 *
 *   Byte 16       : Module Address = 0x03
 *   Byte 17..19   : Reserved
 *   Byte 20       : Register Address
 *   Byte 21..24   : Register Data
 *   Byte 25       : Reserved
 *   Byte 26       : End of Command = 0x16
 *
 *
 * ============================================================
 * 3. DEMODULATOR REGISTER MAP
 * ============================================================
 *
 * ------------------------------------------------------------
 * 0x80 -> AGC Reset
 * ------------------------------------------------------------
 *
 * Register data:
 *
 *   0x08 -> TRUE
 *   0x00 -> FALSE
 *
 * Example:
 *
 *   03 00 00 00 80 00 00 00 08 00 16
 *
 *
 * ------------------------------------------------------------
 * 0x85 -> Mode Select / Sample Select
 * ------------------------------------------------------------
 *
 * The same register address is currently used by two commands.
 *
 *
 * Mode Select:
 *
 *   03 00 00 00 85 00 00 XX 00 00 16
 *                         ^^
 *                       command[7]
 *
 *   0x00 -> AM
 *   0x01 -> FM
 *   0x02 -> PM
 *
 *
 * Sample Select:
 *
 *   03 00 00 00 85 00 00 00 XX 00 16
 *                            ^^
 *                          command[8]
 *
 * Sample Select is currently receive/decode only.
 * No FPGA register/action is assigned yet.
 *
 *
 * ------------------------------------------------------------
 * 0x90 -> Bandwidth
 * ------------------------------------------------------------
 *
 *   0x01 = 200 kHz
 *   0x02 = 150 kHz
 *   0x03 = 60 kHz
 *   0x04 = 30 kHz
 *   0x05 = 15 kHz
 *   0x06 = 12 kHz
 *   0x07 = 7.5 kHz
 *   0x08 = 5 kHz
 *   0x09 = 3 kHz
 *   0x0A = 1.2 kHz
 *   0x0B = 600 Hz
 *
 *
 * ------------------------------------------------------------
 * 0x93 -> Reference 1 / A Coefficient 1
 * ------------------------------------------------------------
 *
 *   command[5]    -> A_Coeff1
 *   command[6..8] -> Ref1 (24-bit)
 *
 *
 * ------------------------------------------------------------
 * 0x94 -> Reference 2 / A Coefficient 2
 * ------------------------------------------------------------
 *
 *   command[5]    -> A_Coeff2
 *   command[6..8] -> Ref2 (24-bit)
 *
 *
 * ------------------------------------------------------------
 * 0x95 -> Demodulator Control
 * ------------------------------------------------------------
 *
 * Bit field:
 *
 *   Bit 0 -> setAGC1On()
 *   Bit 1 -> setMGC1On()
 *   Bit 2 -> setAGC2On()
 *   Bit 3 -> setMGC2On()
 *   Bit 4 -> setSquelchEnable()
 *   Bit 5 -> setModEnable()
 *
 * Example:
 *
 *   03 00 00 00 95 00 00 00 XX 00 16
 *
 * where:
 *
 *   XX[0] -> AGC1
 *   XX[1] -> MGC1
 *   XX[2] -> AGC2
 *   XX[3] -> MGC2
 *   XX[4] -> Squelch
 *   XX[5] -> Mod Enable
 *
 *
 * ------------------------------------------------------------
 * 0x96 -> Message Value
 * ------------------------------------------------------------
 *
 *
 * ============================================================
 * 4. DISPATCH RULE
 * ============================================================
 *
 * The dispatcher MUST NOT identify a command only by the
 * register address.
 *
 * The Module Address (Byte 16) MUST also be checked.
 *
 * Example:
 *
 *   if (module_addr == 0x03)
 *       -> dispatch to Demodulator command handler
 *
 *   then:
 *
 *       0x80 -> AGC Reset
 *       0x85 -> Mode / Sample Select
 *       0x90 -> Bandwidth
 *       0x93 -> Ref1 / ACoeff1
 *       0x94 -> Ref2 / ACoeff2
 *       0x95 -> Demodulator Control
 *       0x96 -> Message Value
 *
 *
 * ============================================================
 */

constexpr uint8_t DEMODULATOR_MODULE_ADDR = 0x03;


/*
 * ============================================================
 * Direct / System Commands
 * ============================================================
 */

constexpr uint8_t CMD_SOFT_RESET  = 0x06;
constexpr uint8_t CMD_MOD_ENABLE  = 0x0F;
constexpr uint8_t CMD_SEND_PACKET = 0x56;
constexpr uint8_t CMD_LED_TIMER   = 0x4B;


/*
 * ============================================================
 * Demodulator Registers
 * ============================================================
 */

constexpr uint8_t DEMOD_REG_AGC_RESET     = 0x80;

constexpr uint8_t DEMOD_REG_MODE          = 0x85;
constexpr uint8_t DEMOD_REG_SAMPLE_SELECT = 0x85;

constexpr uint8_t DEMOD_REG_BANDWIDTH     = 0x90;

constexpr uint8_t DEMOD_REG_REF1_A1       = 0x93;
constexpr uint8_t DEMOD_REG_REF2_A2       = 0x94;

constexpr uint8_t DEMOD_REG_CONTROL       = 0x95;
constexpr uint8_t DEMOD_REG_MESSAGE       = 0x96;


/*
 * ============================================================
 * Demodulator Mode Values
 * ============================================================
 */

constexpr uint8_t DEMOD_MODE_AM = 0x00;
constexpr uint8_t DEMOD_MODE_FM = 0x01;
constexpr uint8_t DEMOD_MODE_PM = 0x02;


/*
 * ============================================================
 * Demodulator Control Bit Field - Register 0x95
 * ============================================================
 *
 *   Bit 0 -> AGC1
 *   Bit 1 -> MGC1
 *   Bit 2 -> AGC2
 *   Bit 3 -> MGC2
 *   Bit 4 -> Squelch
 *   Bit 5 -> Mod Enable
 *
 */

constexpr uint8_t DEMOD_CTRL_AGC1    = (1u << 0);
constexpr uint8_t DEMOD_CTRL_MGC1    = (1u << 1);
constexpr uint8_t DEMOD_CTRL_AGC2    = (1u << 2);
constexpr uint8_t DEMOD_CTRL_MGC2    = (1u << 3);
constexpr uint8_t DEMOD_CTRL_SQUELCH = (1u << 4);
constexpr uint8_t DEMOD_CTRL_MOD     = (1u << 5);


/*
 * ============================================================
 * AGC Reset Values - Register 0x80
 * ============================================================
 */

constexpr uint8_t DEMOD_AGC_RESET_OFF = 0x00;
constexpr uint8_t DEMOD_AGC_RESET_ON  = 0x08;


/*
 * ============================================================
 * Legacy Demodulator Control Values
 * ============================================================
 *
 * These definitions are temporarily kept for compatibility
 * with the current CommandDispatcher.cpp.
 *
 * The 0x95 control handling will be converted to the new
 * bit-field protocol in the next step.
 *
 */

constexpr uint32_t DEMOD_MOD_ENABLE      = 0x20;
constexpr uint32_t DEMOD_SQUELCH_ENABLE  = 0x34;
constexpr uint32_t DEMOD_SQUELCH_DISABLE = 0x24;
constexpr uint32_t DEMOD_MGC2_ON         = 0x08;
constexpr uint32_t DEMOD_AGC1_ON         = 0x01;
constexpr uint32_t DEMOD_AGC2_ON         = 0x04;

/*
 * ============================================================
 * Command Framing
 * ============================================================
 */

constexpr uint8_t L2_COMMAND_END = 0x16;

constexpr size_t L2_COMMAND_SIZE = 11;


/*
 * ============================================================
 * Apply One Complete L2 Command
 * ============================================================
 *
 * command[0] corresponds to Ethernet Frame Byte 17.
 *
 */

bool applyL2Command(
    uint16_t fpga_id,
    const uint8_t *command,
    size_t command_len);


#endif