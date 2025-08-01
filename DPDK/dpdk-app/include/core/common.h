#ifndef COMMON_H
#define COMMON_H

#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <stdint.h>
#include <stddef.h>

#define BURST_SIZE 32
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250

#define SRC_MAC {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}
#define DST_MAC {0x52, 0x54, 0x00, 0xa2, 0x6c, 0xb6}
#define SRC_IP 0x0a000001  // 10.0.0.1
#define DST_IP 0x0a000002  // 10.0.0.2

#define IPv4(a,b,c,d) ((uint32_t)(((a&0xff)<<24)|((b&0xff)<<16)|((c&0xff)<<8)|(d&0xff)))

// Protocol types
enum protocol_type {
    PROTO_ETH,
    PROTO_ICMP,
    PROTO_UDP,
    PROTO_TCP,
    PROTO_ARP,
    PROTO_DNS
};

enum traffic_mode {
    TRAFFIC_CONTINUOUS,
    TRAFFIC_BURST,
    TRAFFIC_RATE_LIMITED,
    TRAFFIC_EXPONENTIAL_BACKOFF
};

// Common packet structure
struct packet_headers {
    struct rte_ether_hdr eth;
    struct rte_ipv4_hdr ip;
};

uint16_t calculate_checksum(const void *data, size_t len);
int setup_ethernet_header(struct rte_ether_hdr *eth, const struct rte_ether_addr *src, 
                         const struct rte_ether_addr *dst, uint16_t ether_type);
int setup_ipv4_header(struct rte_ipv4_hdr *ip, uint32_t src_ip, uint32_t dst_ip, 
                     uint8_t protocol, uint16_t payload_len);
struct rte_mbuf* allocate_packet(struct rte_mempool *mbuf_pool, size_t total_len);

#define CHECK_NULL(ptr, msg) do { \
    if (!(ptr)) { \
        printf("ERROR: %s\n", (msg)); \
        return NULL; \
    } \
} while(0)

#define CHECK_RET(cond, msg, ret) do { \
    if (!(cond)) { \
        printf("ERROR: %s\n", (msg)); \
        return (ret); \
    } \
} while(0)

#endif 