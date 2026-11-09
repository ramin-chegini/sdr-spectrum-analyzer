#include "l2_protocol.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include <arpa/inet.h>


// ============================================================================
// L2 TX - Send packet to GUI
// ============================================================================

bool l2_send_packet(
    const char *interface_name,
    uint16_t eth_type,
    uint16_t id,
    const uint8_t *payload,
    uint16_t payload_len)
{
    unsigned char dst_mac[6] =
    {
        // 0x02, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF   
    };

    unsigned char src_mac[6] =
    {
        // 0x02, 0x11, 0x22, 0x33, 0x44, 0x55
        0x5A, 0x01, 0x02, 0x03, 0x04, 0x05        
    };

    unsigned char frame[L2_MAX_FRAME_SIZE];

    memset(frame, 0, sizeof(frame));

    if (payload_len > L2_MAX_PAYLOAD_SIZE)
    {
        printf(
            "Payload too large: %u bytes (max %u)\n",
            payload_len,
            L2_MAX_PAYLOAD_SIZE);
        return false;
    }

    memcpy(frame, dst_mac, 6);
    memcpy(frame + 6, src_mac, 6);

    uint16_t net_eth_type = htons(eth_type);
    memcpy(frame + 12, &net_eth_type, 2);

    uint16_t net_id = htons(id);
    memcpy(frame + 14, &net_id, 2);

    memcpy(frame + 16, payload, payload_len);

    memcpy(
        frame + 16 + payload_len,
        L2_FOOTER,
        L2_FOOTER_SIZE);

    int frame_len =
        16 +
        payload_len +
        L2_FOOTER_SIZE;

    int sock =
        socket(
            AF_PACKET,
            SOCK_RAW,
            htons(ETH_P_ALL));

    if (sock < 0)
    {
        perror("socket");
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    strncpy(
        ifr.ifr_name,
        interface_name,
        IFNAMSIZ - 1);

    if (ioctl(
            sock,
            SIOCGIFINDEX,
            &ifr) < 0)
    {
        perror("interface");
        close(sock);
        return false;
    }

    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));

    addr.sll_ifindex = ifr.ifr_ifindex;
    addr.sll_halen = ETH_ALEN;

    memcpy(addr.sll_addr, dst_mac, 6);

    int ret =
        sendto(
            sock,
            frame,
            frame_len,
            0,
            (struct sockaddr *)&addr,
            sizeof(addr));

    close(sock);

    if (ret < 0)
    {
        perror("sendto");
        return false;
    }

    printf(
        "L2 frame sent: %d bytes\n",
        frame_len);

    return true;
}


// ============================================================================
// L2 RX - Persistent Listener
// ============================================================================

L2RxListener::L2RxListener()
    : sock(-1),
      expected_packet_len(0)
{
}

L2RxListener::~L2RxListener()
{
    close();
}


// ============================================================================
// Open
// ============================================================================

bool L2RxListener::open(
    const char *interface_name,
    uint16_t packet_len)
{
    expected_packet_len = packet_len;

    sock =
        socket(
            AF_PACKET,
            SOCK_RAW,
            htons(ETH_P_ALL));

    if (sock < 0)
    {
        perror("socket");
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    strncpy(
        ifr.ifr_name,
        interface_name,
        IFNAMSIZ - 1);

    if (ioctl(
            sock,
            SIOCGIFINDEX,
            &ifr) < 0)
    {
        perror("interface");
        close();
        return false;
    }


    // ------------------------------------------------------------------------
    // Enable promiscuous mode only for eth0
    //
    // The GUI sends frames to:
    //     5A:01:02:03:04:05
    //
    // while the real eth0 MAC is:
    //     00:0A:35:00:1E:53
    //
    // Therefore eth0 must accept frames not addressed to its own MAC.
    // ------------------------------------------------------------------------

    if (strcmp(interface_name, "eth0") == 0)
    {
        struct packet_mreq mr;
        memset(&mr, 0, sizeof(mr));

        mr.mr_ifindex = ifr.ifr_ifindex;
        mr.mr_type = PACKET_MR_PROMISC;

        if (setsockopt(
                sock,
                SOL_PACKET,
                PACKET_ADD_MEMBERSHIP,
                &mr,
                sizeof(mr)) < 0)
        {
            perror("PACKET_MR_PROMISC");
            close();
            return false;
        }

        printf(
            "[L2 RX] Promiscuous mode enabled on %s.\n",
            interface_name);
    }


    // ------------------------------------------------------------------------
    // Bind socket to selected interface
    // ------------------------------------------------------------------------

    struct sockaddr_ll bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));

    bind_addr.sll_family = AF_PACKET;
    bind_addr.sll_protocol = htons(ETH_P_ALL);
    bind_addr.sll_ifindex = ifr.ifr_ifindex;

    if (bind(
            sock,
            (struct sockaddr *)&bind_addr,
            sizeof(bind_addr)) < 0)
    {
        perror("bind");
        close();
        return false;
    }

    return true;
}


// ============================================================================
// Receive
// ============================================================================

bool L2RxListener::receive(
    uint16_t *fpga_id,
    uint8_t *command)
{
    if (sock < 0)
    {
        return false;
    }

    unsigned char frame[L2_MAX_FRAME_SIZE];

    while (true)
    {
        struct sockaddr_ll addr;
        memset(&addr, 0, sizeof(addr));

        socklen_t addr_len = sizeof(addr);

        int frame_len =
            recvfrom(
                sock,
                frame,
                sizeof(frame),
                0,
                (struct sockaddr *)&addr,
                &addr_len);

        if (frame_len < 0)
        {
            perror("recvfrom");
            return false;
        }


        // --------------------------------------------------------------------
        // Ethernet header check
        // --------------------------------------------------------------------

        if (frame_len < L2_ETH_HEADER_SIZE)
        {
            printf(
                "Ignoring short Ethernet frame: %d bytes\n",
                frame_len);
            continue;
        }


        // --------------------------------------------------------------------
        // Custom protocol packet length
        //
        // Bytes 12..13 contain the logical protocol length.
        // GUI sends:
        //
        //     00 0D
        //
        // which means 13 bytes.
        //
        // The complete logical L2 protocol frame is still 27 bytes.
        // --------------------------------------------------------------------

        uint16_t net_packet_len;

        memcpy(
            &net_packet_len,
            frame + 12,
            2);

        uint16_t packet_len =
            ntohs(net_packet_len);

        if (packet_len != expected_packet_len)
        {
            continue;
        }


        // --------------------------------------------------------------------
        // Ethernet minimum frame size / padding
        //
        // The GUI sends a 27-byte logical frame.
        // Linux AF_PACKET may deliver it as a 60-byte Ethernet frame
        // because Ethernet minimum frame size is 60 bytes without FCS.
        //
        // Therefore:
        //
        //     DO NOT require frame_len == 27
        //
        // Accept 27 bytes or larger.
        // --------------------------------------------------------------------

        if (frame_len < L2_RX_FRAME_SIZE)
        {
            printf(
                "Invalid L2 RX frame size: %d bytes "
                "(minimum %d)\n",
                frame_len,
                L2_RX_FRAME_SIZE);
            continue;
        }


        // --------------------------------------------------------------------
        // FPGA ID
        // --------------------------------------------------------------------

        uint16_t net_fpga_id;

        memcpy(
            &net_fpga_id,
            frame + 14,
            2);

        uint16_t rx_fpga_id =
            ntohs(net_fpga_id);


        // --------------------------------------------------------------------
        // Command
        // --------------------------------------------------------------------

        if (command != NULL)
        {
            memcpy(
                command,
                frame + L2_RX_COMMAND_OFFSET,
                L2_RX_COMMAND_SIZE);
        }


        // --------------------------------------------------------------------
        // Command terminator
        // --------------------------------------------------------------------

        if (frame[
                L2_RX_COMMAND_OFFSET +
                L2_RX_COMMAND_SIZE -
                1] != 0x16)
        {
            printf(
                "Invalid L2 RX command terminator\n");
            continue;
        }


        // --------------------------------------------------------------------
        // Return FPGA ID
        // --------------------------------------------------------------------

        if (fpga_id != NULL)
        {
            *fpga_id = rx_fpga_id;
        }


        // --------------------------------------------------------------------
        // Successful reception
        // --------------------------------------------------------------------

        printf(
            "L2 RX command received: %d bytes\n",
            frame_len);

        return true;
    }
}


// ============================================================================
// Close
// ============================================================================

void L2RxListener::close()
{
    if (sock >= 0)
    {
        ::close(sock);
        sock = -1;
    }
}


// ============================================================================
// L2 RX - One Shot Receive
// ============================================================================

bool l2_receive_packet(
    const char *interface_name,
    uint16_t expected_packet_len,
    uint16_t *fpga_id,
    uint8_t *command)
{
    L2RxListener listener;

    if (!listener.open(
            interface_name,
            expected_packet_len))
    {
        return false;
    }

    bool result =
        listener.receive(
            fpga_id,
            command);

    listener.close();

    return result;
}