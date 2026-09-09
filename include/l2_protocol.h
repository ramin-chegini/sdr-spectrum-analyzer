#pragma once

#include <stdint.h>


// L2 TX → Zynq → GUI
// L2 RX → GUI  → Zynq


/*
 * L2 TX Frame Format
 *
 * Offset   Size       Field
 * ------   --------   ----------------
 * 0        6          Destination MAC
 * 6        6          Source MAC
 * 12       2          EtherType
 * 14       2          Custom ID
 * 16       N          Payload
 * 16+N     8          Footer
 *
 * Total Frame Size = 16 + Payload Length + 8
 */

#define L2_ETHERTYPE_PSD 0x9410
#define L2_ETHERTYPE_IQ  0x9411
#define L2_ETHERTYPE_AUDIO  0x9412


#define L2_ID_PSD 0x0200
#define L2_ID_IQ  0x0208
#define L2_ID_AUDIO 0x0210

#define L2_FOOTER_SIZE 8

#define L2_MAX_PAYLOAD_SIZE 8192

#define L2_ETH_HEADER_SIZE 14
#define L2_CUSTOM_HEADER_SIZE 2

#define L2_FRAME_OVERHEAD \
    (L2_ETH_HEADER_SIZE + L2_CUSTOM_HEADER_SIZE + L2_FOOTER_SIZE)

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


bool l2_send_packet(
    const char *iface,
    uint16_t eth_type,
    uint16_t id,
    const uint8_t *payload,
    uint16_t payload_len
);


/*
 * =====================================================
 * L2 RX Frame Format
 * =====================================================
 *
 * Offset   Size       Field
 * ------   --------   ----------------
 * 0        6          Destination MAC
 * 6        6          Source MAC
 * 12       2          Packet Length = 0x000D
 * 14       2          FPGA ID       = 0x0282
 * 16       1          Register Address = (0-255)
 * 17       8          Data
 * 25       2          Footer = 0x0016
 *
 * Total Frame Size = 27 Bytes
 */

#define L2_RX_PACKETLEN       0x000D
#define L2_RX_FPGA_ID_SIZE    2
#define L2_RX_REG_ADDR_SIZE   1
#define L2_RX_DATA_SIZE       8
#define L2_RX_FOOTER_SIZE     2
#define L2_RX_FRAME_SIZE      27


static const uint8_t L2_RX_FOOTER[L2_RX_FOOTER_SIZE] =
{
    0x00,
    0x16
};

/*
 * =====================================================
 * L2 RX - One Shot Receive
 * =====================================================
 *
 * Existing debug API.
 *
 * Receives one valid GUI packet and returns.
 */

bool l2_receive_packet(
    const char *iface,
    uint16_t expected_packet_len,
    uint16_t *fpga_id,
    uint8_t *reg_addr,
    uint8_t *data
);

/*
 * =====================================================
 * L2 RX Listener
 * =====================================================
 *
 * Persistent Layer-2 listener.
 *
 * Socket and bind are performed only once.
 * receive() can then be called repeatedly.
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
        uint8_t *reg_addr,
        uint8_t *data);

    void close();

private:

    int sock;
    uint16_t expected_packet_len;
};