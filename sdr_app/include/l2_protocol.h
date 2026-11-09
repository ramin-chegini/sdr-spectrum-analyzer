#pragma once

#include <stdint.h>
#include <stddef.h>


// ============================================================================
// L2 Protocol
// ============================================================================
//
// L2 TX → Zynq → GUI
// L2 RX → GUI → Zynq
//
//
// ============================================================================
// L2 TX Frame Format
// ============================================================================
//
// Offset   Size       Field
// ------   --------   ----------------
// 0        6          Destination MAC
// 6        6          Source MAC
// 12       2          EtherType
// 14       2          Custom ID
// 16       N          Payload
// 16+N     8          Footer
//
// Total Frame Size = 16 + Payload Length + 8
// ============================================================================


#define L2_ETHERTYPE_PSD      0x9410
#define L2_ETHERTYPE_IQ       0x9411
#define L2_ETHERTYPE_AUDIO    0x8400

#define L2_ID_PSD             0x0200
#define L2_ID_IQ              0x0208
#define L2_ID_AUDIO           0x0213


#define L2_FOOTER_SIZE        8
#define L2_MAX_PAYLOAD_SIZE   8192

#define L2_ETH_HEADER_SIZE    14
#define L2_CUSTOM_HEADER_SIZE 2


#define L2_FRAME_OVERHEAD \
    (L2_ETH_HEADER_SIZE + \
     L2_CUSTOM_HEADER_SIZE + \
     L2_FOOTER_SIZE)


#define L2_MAX_FRAME_SIZE \
    (L2_MAX_PAYLOAD_SIZE + L2_FRAME_OVERHEAD)


static const uint8_t L2_FOOTER[L2_FOOTER_SIZE] =
{
    0x00,
    0x16,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00
};


/*
 * ============================================================================
 * L2 TX
 * ============================================================================
 */

bool l2_send_packet(
    const char *iface,
    uint16_t eth_type,
    uint16_t id,
    const uint8_t *payload,
    uint16_t payload_len
);


/*
 * ============================================================================
 * L2 RX Frame Format
 * ============================================================================
 *
 * The complete GUI command frame is 27 bytes.
 *
 * Offset   Size       Field
 * ------   --------   ----------------
 * 0        6          Destination MAC
 * 6        6          Source MAC
 * 12       2          Packet Length = 0x000D
 * 14       2          FPGA ID       = 0x0282
 * 16       11         Command
 *
 * Command occupies Frame[16..26]
 *
 *
 * ============================================================================
 * Command Format
 * ============================================================================
 *
 * command[0]  : Module Address / Direct Command Address
 * command[1]  : Reserved
 * command[2]  : Reserved
 * command[3]  : Reserved
 * command[4]  : Register Address
 * command[5]  : Data[31:24]
 * command[6]  : Data[23:16]
 * command[7]  : Data[15:8]
 * command[8]  : Data[7:0]
 * command[9]  : Reserved
 * command[10] : Command End = 0x16
 *
 *
 * ============================================================================
 * Direct Commands
 * ============================================================================
 *
 * command[0] identifies the command directly.
 *
 *   0x06 -> Soft Reset
 *   0x10 -> Soft Start
 *   0x56 -> Send Packet
 *   0x4B -> LED Timer
 *
 *
 * ============================================================================
 * Demodulator Module
 * ============================================================================
 *
 * command[0] = 0x03
 *
 *   0x03 -> Demodulator
 *
 * command[4] identifies the register/parameter.
 *
 *   0x85 -> Mode Select
 *   0x90 -> Bandwidth
 *   0x93 -> Ref1 / A Coeff1
 *   0x94 -> Ref2 / A Coeff2
 *   0x95 -> Demodulator Control
 *   0x96 -> Message Value
 *
 *
 * Current Demodulator Register Usage
 * -----------------------------------
 *
 * 0x85 -> Mode Select
 *
 *      0x00 -> AM
 *      0x01 -> FM
 *      0x02 -> PM
 *
 * NOTE:
 * The protocol places the mode in Data[15:8].
 *
 *
 * 0x90 -> Bandwidth
 *
 *      0x01 -> 200 kHz
 *      0x02 -> 150 kHz
 *      0x03 -> 60 kHz
 *      0x04 -> 30 kHz
 *      0x05 -> 15 kHz
 *      0x06 -> 12 kHz
 *      0x07 -> 7.5 kHz
 *      0x08 -> 5 kHz
 *      0x09 -> 3 kHz
 *      0x0A -> 1.2 kHz
 *      0x0B -> 600 Hz
 *
 *
 * 0x93 -> Ref1 / A Coeff1
 *
 *      Ref1:
 *
 *          03 00 00 00 93 00 XX XX XX 00 16
 *
 *      A Coeff1:
 *
 *          03 00 00 00 93 XX 00 00 00 00 16
 *
 *
 * 0x94 -> Ref2 / A Coeff2
 *
 *      Ref2:
 *
 *          03 00 00 00 94 00 XX XX XX 00 16
 *
 *      A Coeff2:
 *
 *          03 00 00 00 94 XX 00 00 00 00 16
 *
 *
 * 0x95 -> Demodulator Control
 *
 *      0x01 -> AGC1
 *      0x04 -> AGC2
 *      0x08 -> MGC2
 *      0x20 -> Mod Enable
 *      0x24 -> Squelch Disable
 *      0x34 -> Squelch Enable
 *
 *
 * 0x96 -> Message Value
 *
 *
 * ============================================================================
 * Future Modules
 * ============================================================================
 *
 * Future functional blocks will use their own Module Address in command[0].
 *
 * Example:
 *
 *      0x04 -> Future Module
 *      0x05 -> Future Module
 *
 * Therefore command[4] must only be interpreted as a register address after
 * the module address has been identified.
 *
 * ============================================================================
 */


/* RX frame constants */

#define L2_RX_PACKETLEN        0x000D

#define L2_RX_FPGA_ID_SIZE     2
#define L2_RX_COMMAND_OFFSET   16
#define L2_RX_COMMAND_SIZE     11
#define L2_RX_FOOTER_SIZE      2

#define L2_RX_FRAME_SIZE       27


static const uint8_t L2_RX_FOOTER[L2_RX_FOOTER_SIZE] =
{
    0x00,
    0x16
};


/*
 * ============================================================================
 * L2 RX - One Shot Receive
 * ============================================================================
 *
 * Receives one valid GUI command packet.
 *
 * The command buffer contains exactly 11 bytes:
 *
 *      command[0..10]
 *
 * ============================================================================
 */

bool l2_receive_packet(
    const char *iface,
    uint16_t expected_packet_len,
    uint16_t *fpga_id,
    uint8_t *command
);


/*
 * ============================================================================
 * L2 RX Persistent Listener
 * ============================================================================
 *
 * The socket is opened and bound once.
 *
 * receive() can then be called repeatedly for GUI commands.
 * ============================================================================
 */

class L2RxListener
{
public:

    L2RxListener();
    ~L2RxListener();


    bool open(
        const char *iface,
        uint16_t expected_packet_len);


    bool receive(
        uint16_t *fpga_id,
        uint8_t *command);


    void close();


private:

    int sock;

    uint16_t expected_packet_len;
};