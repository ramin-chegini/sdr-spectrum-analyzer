#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

// ============================================================
// Network Interface Selection
// ============================================================

 //#define SDR_NET_USB0
 #define SDR_NET_ETH0

#if defined(SDR_NET_USB0) && defined(SDR_NET_ETH0)
#error "Only one network interface can be selected"
#endif

#if !defined(SDR_NET_USB0) && !defined(SDR_NET_ETH0)
#error "No network interface selected"
#endif

#if defined(SDR_NET_USB0)

#define SDR_NET_INTERFACE "usb0"

#elif defined(SDR_NET_ETH0)

#define SDR_NET_INTERFACE "eth0"

#endif

#endif