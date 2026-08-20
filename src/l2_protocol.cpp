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