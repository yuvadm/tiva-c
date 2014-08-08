/* constants_dep.h */

#ifndef CONSTANTS_DEP_H
#define CONSTANTS_DEP_H

/* platform dependent */

#ifdef IF_NAMESIZE
    #define IFACE_NAME_LENGTH   IF_NAMESIZE
#else
    #define IFACE_NAME_LENGTH   16
#endif

#ifdef INET_ADDRSTRLEN
    #define NET_ADDRESS_LENGTH  INET_ADDRSTRLEN
#else
    #define NET_ADDRESS_LENGTH  16
#endif

#define IFCONF_LENGTH 10

#define PTPD_LSBF

#define ADJ_MAX  10000000

/* UDP/IPv4 dependent */

#define SUBDOMAIN_ADDRESS_LENGTH  4
#define PORT_ADDRESS_LENGTH       2

#define PACKET_SIZE  300

#define PTP_EVENT_PORT    319
#define PTP_GENERAL_PORT  320

#define DEFAULT_PTP_DOMAIN_ADDRESS     "224.0.1.129"
#define ALTERNATE_PTP_DOMAIN1_ADDRESS  "224.0.1.130"
#define ALTERNATE_PTP_DOMAIN2_ADDRESS  "224.0.1.131"
#define ALTERNATE_PTP_DOMAIN3_ADDRESS  "224.0.1.132"

#define HEADER_LENGTH             40
#define SYNC_PACKET_LENGTH        124
#define DELAY_REQ_PACKET_LENGTH   124
#define FOLLOW_UP_PACKET_LENGTH   52
#define DELAY_RESP_PACKET_LENGTH  60
#define MANAGEMENT_PACKET_LENGTH  136

#define MM_STARTING_BOUNDARY_HOPS  0x7fff

/* others */

#define SCREEN_BUFSZ  128
#define SCREEN_MAXSZ  80

#define PBUF_QUEUE_SIZE 16

/* override default values */
#ifdef      DEFAULT_AP
#   undef   DEFAULT_AP
#   define  DEFAULT_AP  2
#endif

#ifdef      DEFAULT_AI
#   undef   DEFAULT_AI
#   define  DEFAULT_AI  200
#endif

#define     MAX_AP      10
#define     MAX_AI      1000

#ifdef      DEFAULT_INBOUND_LATENCY
#   undef   DEFAULT_INBOUND_LATENCY
#   define  DEFAULT_INBOUND_LATENCY     16500
#endif

#ifdef      DEFAULT_OUTBOUND_LATENCY
#   undef   DEFAULT_OUTBOUND_LATENCY
#   define  DEFAULT_OUTBOUND_LATENCY    16500
#endif

#ifdef      CLOCK_FOLLOWUP
#   undef   CLOCK_FOLLOWUP
#   define  CLOCK_FOLLOWUP              FALSE
#endif

#endif  /* #ifndef CONSTANTS_DEP_H */

