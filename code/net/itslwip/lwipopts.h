#pragma once

/*
ITS kernel LWIP port configuration options.
See src/include/lwip/opt.h for a list of all available options.
*/

// Currently we can not provide all the functionality LWIP requires to run in threaded mode
// Instead use the raw API
// Nice side effect: Since there is only one networking thread, system noise is reduced
#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0

// ARP
#define LWIP_ARP 1
#define ETHARP_SUPPORT_VLAN 0

// IPv4
#define LWIP_IPV4 1

// IPv6
#define LWIP_IPV6 0
#define LWIP_IPV6_DHCP6 0

// ICMP
#define LWIP_ICMP 1
#define LWIP_BROADCAST_PING 1
#define LWIP_MULTICAST_PING 1

// DHCP
#define LWIP_DHCP 0
#define LWIP_AUTOIP 0
#define LWIP_DHCP_AUTOIP_COOP 0
#define LWIP_DHCP_AUTOIP_COOP_TRIES 9

// DNS
#define LWIP_DNS 0

// UDP
#define LWIP_UDP 1
#define LWIP_UDPLITE 0

// TCP
#define LWIP_TCP 1
#define MEM_SIZE                        (1024 * 1024) /* 1MiB */
#define MEMP_NUM_PBUF                   1024
#define MEMP_NUM_TCP_PCB                32
#define PBUF_POOL_SIZE                  1024
#define TCP_MSS                         1460
#define TCP_WND                         (4*TCP_MSS)
#define TCP_SND_BUF                     65535
#define TCP_OVERSIZE                    TCP_MSS
#define TCP_SND_QUEUELEN                512
#define MEMP_NUM_TCP_SEG                512
#define MEMP_SANITY_CHECK               0

// Debug flags
/*#define LWIP_DBG_MIN_LEVEL     LWIP_DBG_LEVEL_ALL
#define LWIP_DBG_TYPES_ON      LWIP_DBG_ON
#define ETHARP_DEBUG           LWIP_DBG_ON
#define NETIF_DEBUG            LWIP_DBG_ON
#define PBUF_DEBUG             LWIP_DBG_ON
#define API_LIB_DEBUG          LWIP_DBG_ON
#define API_MSG_DEBUG          LWIP_DBG_ON
#define SOCKETS_DEBUG          LWIP_DBG_ON
#define ICMP_DEBUG             LWIP_DBG_ON
#define IGMP_DEBUG             LWIP_DBG_ON
#define INET_DEBUG             LWIP_DBG_ON
#define IP_DEBUG               LWIP_DBG_ON
#define IP_REASS_DEBUG         LWIP_DBG_ON
#define RAW_DEBUG              LWIP_DBG_ON
#define MEM_DEBUG              LWIP_DBG_ON
#define MEMP_DEBUG             LWIP_DBG_ON
#define SYS_DEBUG              LWIP_DBG_ON
#define TIMERS_DEBUG           LWIP_DBG_ON
#define TCP_DEBUG              LWIP_DBG_ON
#define TCP_INPUT_DEBUG        LWIP_DBG_ON
#define TCP_FR_DEBUG           LWIP_DBG_ON
#define TCP_RTO_DEBUG          LWIP_DBG_ON
#define TCP_CWND_DEBUG         LWIP_DBG_ON
#define TCP_WND_DEBUG          LWIP_DBG_ON
#define TCP_OUTPUT_DEBUG       LWIP_DBG_ON
#define TCP_RST_DEBUG          LWIP_DBG_ON
#define TCP_QLEN_DEBUG         LWIP_DBG_ON
#define UDP_DEBUG              LWIP_DBG_ON
#define TCPIP_DEBUG            LWIP_DBG_ON
#define SLIP_DEBUG             LWIP_DBG_ON
#define DHCP_DEBUG             LWIP_DBG_ON
#define AUTOIP_DEBUG           LWIP_DBG_ON
#define DNS_DEBUG              LWIP_DBG_ON
#define IP6_DEBUG              LWIP_DBG_ON*/