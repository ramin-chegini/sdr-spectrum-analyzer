#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <stdint.h>

/*GUI
 │
 │ L2 RX Packet
 ▼
FPGA ID
Register Address
Data[8]
 │
 ▼
l2_receive_packet()
 │
 ▼
Command Dispatcher
 │
 ├── Addr 1  → setSoftReset()
 ├── Addr 2  → setSoftStart()
 ├── Addr 3  → setSendPacket()
 ├── Addr 4  → setModEnable()
 ├── Addr 5  → setSquelchEnable()
 ├── Addr 6  → setMGC2On()
 ├── Addr 7  → setAGC1On()
 ├── Addr 8  → setAGC2On()
 ├── Addr 9  → setAmpOK()
 ├── Addr 10 → setBandwidth()
 ├── Addr 11 → setModeSelect()
 ├── Addr 12 → setLedTimer()
 ├── Addr 13 → setMsgValue()
 ├── Addr 14 → setACoeff1()
 ├── Addr 15 → setACoeff2()
 ├── Addr 16 → setRef1()
 └── Addr 17 → setRef2()  */

/*
 * Convert GUI Data[8] into a 32-bit value.
 * Data[8] = 8-byte Big-Endian value
 * Data format:
 *
 *   00 00 00 00 XX XX XX XX
 *   -----------------------
 *             32-bit value
 *
 * Example:
 *
 *   00 00 00 00 12 34 56 78
 *
 *   => 0x12345678
 *   => 305419896
 */


bool applyRegisterCommand(
    uint16_t fpga_id,
    uint8_t reg_addr,
    const uint8_t *data);

#endif