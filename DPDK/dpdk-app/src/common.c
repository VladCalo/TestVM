#include "../include/common.h"
#include <rte_byteorder.h>
#include <stdio.h>
#include <string.h>

uint16_t calculate_checksum(const void *data, size_t len) {
    const uint16_t *buf = data;
    uint32_t sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1)
        sum += *((uint8_t *)buf);

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

int setup_ethernet_header(struct rte_ether_hdr *eth, const struct rte_ether_addr *src, 
                         const struct rte_ether_addr *dst, uint16_t ether_type) {
    CHECK_RET(eth && src && dst, "Invalid parameters for ethernet header setup", -1);
    
    eth->src_addr = *src;
    eth->dst_addr = *dst;
    eth->ether_type = rte_cpu_to_be_16(ether_type);
    
    return 0;
}

int setup_ipv4_header(struct rte_ipv4_hdr *ip, uint32_t src_ip, uint32_t dst_ip, 
                     uint8_t protocol, uint16_t payload_len) {
    CHECK_RET(ip, "Invalid IP header pointer", -1);
    
    ip->version_ihl = (4 << 4) | (sizeof(struct rte_ipv4_hdr) / 4);
    ip->type_of_service = 0;
    ip->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + payload_len);
    ip->packet_id = 0;
    ip->fragment_offset = 0;
    ip->time_to_live = 64;
    ip->next_proto_id = protocol;
    ip->hdr_checksum = 0;
    ip->src_addr = rte_cpu_to_be_32(src_ip);
    ip->dst_addr = rte_cpu_to_be_32(dst_ip);
    ip->hdr_checksum = calculate_checksum(ip, sizeof(struct rte_ipv4_hdr));
    
    return 0;
}

struct rte_mbuf* allocate_packet(struct rte_mempool *mbuf_pool, size_t total_len) {
    CHECK_NULL(mbuf_pool, "Invalid mbuf pool");
    
    struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
    if (!mbuf) {
        printf("ERROR: Failed to allocate mbuf\n");
        return NULL;
    }
    
    char *data = rte_pktmbuf_append(mbuf, total_len);
    if (!data) {
        printf("ERROR: Failed to append data to mbuf\n");
        rte_pktmbuf_free(mbuf);
        return NULL;
    }
    
    return mbuf;
} 