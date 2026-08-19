#pragma once

#include <stdint.h>


#define L2_ETHERTYPE_PSD 0x9410
#define L2_ETHERTYPE_IQ  0x9411


#define L2_ID_PSD 0x0200
#define L2_ID_IQ  0x0208


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