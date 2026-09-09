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

//---------------------------------------------------------------------
// L2 TX - Send packet to GUI
//---------------------------------------------------------------------

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

bool l2_send_packet(
    const char *interface_name,
    uint16_t eth_type,
    uint16_t id,
    const uint8_t *payload,
    uint16_t payload_len)
{

    unsigned char dst_mac[6] =  // usb0 Dest.Mac Addr
    {
        0x02,0xAA,0xBB,0xCC,0xDD,0xEE
    };

    // unsigned char dst_mac[6] =  // eth0 Dest.Mac Addr
    // {
    //     0xC8,0x7F,0x54,0xA7,0xDD,0x16
    // };    


    unsigned char src_mac[6] =     // usb0 Src.Mac Addr
    {
        0x02,0x11,0x22,0x33,0x44,0x55
    };

    // unsigned char src_mac[6] =   // eth0 Src.Mac Addr
    // {
    //     0x00, 0x0A, 0x35, 0x00, 0x1E, 0x53
    // };    


    unsigned char frame[L2_MAX_FRAME_SIZE];

    memset(frame,0,sizeof(frame));


    // if((size_t)payload_len + 16 + L2_FOOTER_SIZE > sizeof(frame))
    // {
    //     printf("Payload too large\n");
    //     return false;
    // }

    if(payload_len > L2_MAX_PAYLOAD_SIZE)
    {
        printf("Payload too large: %u bytes (max %u)\n",
            payload_len,
            L2_MAX_PAYLOAD_SIZE);

        return false;
    }
    

    /*
     * Ethernet Header
     */

    memcpy(frame,
           dst_mac,
           6);


    memcpy(frame + 6,
           src_mac,
           6);


    /*
     * EtherType
     */

    uint16_t net_eth_type = htons(eth_type);

    memcpy(frame + 12,
           &net_eth_type,
           2);


    /*
     * Custom ID
     */

    uint16_t net_id = htons(id);

    memcpy(frame + 14,
           &net_id,
           2);


    /*
     * Payload
     */

    memcpy(frame + 16,
           payload,
           payload_len);


    /*
     * Company Footer
     */

    memcpy(frame + 16 + payload_len,
           L2_FOOTER,
           L2_FOOTER_SIZE);



    int frame_len = 16 + payload_len + L2_FOOTER_SIZE;



    int sock = socket(AF_PACKET,
                     SOCK_RAW,
                     htons(ETH_P_ALL));


    if(sock < 0)
    {
        perror("socket");
        return false;
    }



    struct ifreq ifr;

    memset(&ifr,0,sizeof(ifr));


    strncpy(ifr.ifr_name,
            interface_name,
            IFNAMSIZ-1);



    if(ioctl(sock,
             SIOCGIFINDEX,
             &ifr)<0)
    {
        perror("interface");
        close(sock);
        return false;
    }



    struct sockaddr_ll addr;

    memset(&addr,0,sizeof(addr));


    addr.sll_ifindex = ifr.ifr_ifindex;

    addr.sll_halen = ETH_ALEN;


    memcpy(addr.sll_addr,
           dst_mac,
           6);



    int ret = sendto(sock,
                     frame,
                     frame_len,
                     0,
                     (struct sockaddr*)&addr,
                     sizeof(addr));



    close(sock);



    if(ret < 0)
    {
        perror("sendto");
        return false;
    }


    printf("L2 frame sent: %d bytes\n",
           frame_len);


    return true;
}


//---------------------------------------------------------------------
// L2 RX - Receive configuration packet from GUI
//---------------------------------------------------------------------

/*
 * L2 RX Frame Format
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


/*
 * =====================================================
 * L2 RX Listener Constructor
 * =====================================================
 */

L2RxListener::L2RxListener()
    : sock(-1),
      expected_packet_len(0)
{
}


/*
 * =====================================================
 * L2 RX Listener Destructor
 * =====================================================
 */

L2RxListener::~L2RxListener()
{
    close();
}


/*
 * =====================================================
 * L2 RX Listener Open
 *
 * Creates and binds the Layer-2 socket once.
 * =====================================================
 */

bool L2RxListener::open(
    const char *interface_name,
    uint16_t packet_len)
{
    expected_packet_len = packet_len;

    sock = socket(
        AF_PACKET,
        SOCK_RAW,
        htons(ETH_P_ALL));

    if (sock < 0)
    {
        perror("socket");
        return false;
    }


    struct ifreq ifr;

    memset(
        &ifr,
        0,
        sizeof(ifr));


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


    struct sockaddr_ll bind_addr;

    memset(
        &bind_addr,
        0,
        sizeof(bind_addr));


    bind_addr.sll_family =
        AF_PACKET;

    bind_addr.sll_protocol =
        htons(ETH_P_ALL);

    bind_addr.sll_ifindex =
        ifr.ifr_ifindex;


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


/*
 * =====================================================
 * L2 RX Listener Receive
 *
 * Waits for the next valid GUI packet.
 *
 * The socket remains open after receive().
 * =====================================================
 */

bool L2RxListener::receive(
    uint16_t *fpga_id,
    uint8_t *reg_addr,
    uint8_t *data)
{
    if (sock < 0)
    {
        return false;
    }


    unsigned char frame[L2_MAX_FRAME_SIZE];


    while (true)
    {
        struct sockaddr_ll addr;

        memset(
            &addr,
            0,
            sizeof(addr));


        socklen_t addr_len =
            sizeof(addr);


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


        /*
         * =================================================
         * Ethernet Header
         * =================================================
         */

        if (frame_len < L2_ETH_HEADER_SIZE)
        {
            printf(
                "Ignoring short Ethernet frame: %d bytes\n",
                frame_len);

            continue;
        }


        /*
         * =================================================
         * Packet Length
         *
         * Offset 12..13
         * =================================================
         */

        uint16_t net_packet_len;

        memcpy(
            &net_packet_len,
            frame + 12,
            2);


        uint16_t packet_len =
            ntohs(net_packet_len);


        /*
         * =================================================
         * Ignore unrelated Ethernet traffic
         *
         * Example:
         *
         * IPv4 = 0x0800
         *
         * No log is printed here.
         * =================================================
         */

        if (packet_len != expected_packet_len)
        {
            continue;
        }


        /*
         * =================================================
         * Validate complete GUI packet
         * =================================================
         */

        if (frame_len != L2_RX_FRAME_SIZE)
        {
            printf(
                "Invalid L2 RX frame size: %d bytes "
                "(expected %d)\n",
                frame_len,
                L2_RX_FRAME_SIZE);

            continue;
        }


        /*
         * =================================================
         * FPGA ID
         *
         * Offset 14..15
         * =================================================
         */

        uint16_t net_fpga_id;

        memcpy(
            &net_fpga_id,
            frame + 14,
            2);


        uint16_t rx_fpga_id =
            ntohs(net_fpga_id);


        /*
         * =================================================
         * Register Address
         *
         * Offset 16
         * =================================================
         */

        uint8_t rx_reg_addr =
            frame[16];


        /*
         * =================================================
         * Data
         *
         * Offset 17..24
         * =================================================
         */

        if (data != NULL)
        {
            memcpy(
                data,
                frame + 17,
                L2_RX_DATA_SIZE);
        }


        /*
         * =================================================
         * Footer
         *
         * Offset 25..26
         * =================================================
         */

        if (memcmp(
                frame + 25,
                L2_RX_FOOTER,
                L2_RX_FOOTER_SIZE) != 0)
        {
            printf(
                "Invalid L2 RX footer\n");

            continue;
        }


        /*
         * =================================================
         * Return decoded values
         * =================================================
         */

        if (fpga_id != NULL)
        {
            *fpga_id =
                rx_fpga_id;
        }


        if (reg_addr != NULL)
        {
            *reg_addr =
                rx_reg_addr;
        }


        printf(
            "L2 RX packet received: %d bytes\n",
            frame_len);


        /*
         * IMPORTANT:
         *
         * Do NOT close the socket here.
         *
         * The socket remains open for the
         * next GUI command.
         */

        return true;
    }
}


/*
 * =====================================================
 * L2 RX Listener Close
 * =====================================================
 */

void L2RxListener::close()
{
    if (sock >= 0)
    {
        ::close(sock);
        sock = -1;
    }
}


/*
 * =====================================================
 * Legacy One-Shot L2 RX
 *
 * Kept for compatibility/debug.
 *
 * Socket is opened for one packet and then closed.
 * =====================================================
 */

bool l2_receive_packet(
    const char *interface_name,
    uint16_t expected_packet_len,
    uint16_t *fpga_id,
    uint8_t *reg_addr,
    uint8_t *data)
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
            reg_addr,
            data);


    listener.close();


    return result;
}