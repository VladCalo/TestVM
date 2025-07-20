#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "../include/core/common.h"
#include "../include/core/config.h"
#include "../include/core/log.h"
#include "../include/core/packet_utils.h"
#include "../include/core/traffic_modes.h"
#include "../include/protocols/dns.h"

#define DNS_PORT 53
#define DNS_QUERY_TYPE_A 1
#define DNS_CLASS_IN 1

struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed));

struct dns_query {
    char name[256];
    uint16_t qtype;
    uint16_t qclass;
} __attribute__((packed));

static void setup_dns_header(struct dns_header *dns, uint16_t id, uint16_t flags) {
    dns->id = rte_cpu_to_be_16(id);
    dns->flags = rte_cpu_to_be_16(flags);
    dns->qdcount = rte_cpu_to_be_16(1);
    dns->ancount = 0;
    dns->nscount = 0;
    dns->arcount = 0;
}

static int encode_dns_name(const char *domain, char *encoded) {
    char *ptr = encoded;
    char *label_start = ptr;
    int label_len = 0;
    
    while (*domain) {
        if (*domain == '.') {
            *label_start = label_len;
            label_start = ptr + 1;
            label_len = 0;
        } else {
            *ptr = *domain;
            label_len++;
        }
        ptr++;
        domain++;
    }
    
    if (label_len > 0) {
        *label_start = label_len;
        ptr++;
    }
    
    *ptr = 0;
    return ptr - encoded + 1;
}

static void setup_dns_query(struct dns_query *query, const char *domain) {
    encode_dns_name(domain, query->name);
    query->qtype = rte_cpu_to_be_16(DNS_QUERY_TYPE_A);
    query->qclass = rte_cpu_to_be_16(DNS_CLASS_IN);
}

void dns_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config, const char *domain) {
    const struct rte_ether_addr src_mac = {SRC_MAC};
    const struct rte_ether_addr dst_mac = {DST_MAC};
    uint16_t dns_id = 0x1234;
    
    LOG_INFO("Starting DNS TX loop on port %d", port_id);
    
    while (1) {
        struct rte_mbuf *m = allocate_packet(mbuf_pool, 1500);
        if (!m) {
            LOG_ERROR("Failed to allocate mbuf for DNS packet");
            continue;
        }
        
        char *pkt_data = rte_pktmbuf_mtod(m, char *);
        struct rte_ether_hdr *eth = (struct rte_ether_hdr *)pkt_data;
        struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
        struct rte_udp_hdr *udp = (struct rte_udp_hdr *)(ip + 1);
        struct dns_header *dns = (struct dns_header *)(udp + 1);
        struct dns_query *query = (struct dns_query *)(dns + 1);
        
        setup_ethernet_header(eth, &src_mac, &dst_mac, RTE_ETHER_TYPE_IPV4);
        
        uint16_t dns_len = sizeof(struct dns_header) + sizeof(struct dns_query);
        setup_ipv4_header(ip, SRC_IP, DST_IP, IPPROTO_UDP, sizeof(struct rte_udp_hdr) + dns_len);
        
        udp->src_port = rte_cpu_to_be_16(12345);
        udp->dst_port = rte_cpu_to_be_16(DNS_PORT);
        udp->dgram_len = rte_cpu_to_be_16(sizeof(struct rte_udp_hdr) + dns_len);
        udp->dgram_cksum = 0;
        
        setup_dns_header(dns, dns_id++, 0x0100);
        setup_dns_query(query, domain);
        
        m->data_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + 
                     sizeof(struct rte_udp_hdr) + dns_len;
        m->pkt_len = m->data_len;
        
        uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, &m, 1);
        if (nb_tx == 0) {
            rte_pktmbuf_free(m);
            LOG_WARN("DNS: Failed to send packet");
        } else {
            LOG_INFO("DNS: Sent query for %s (ID: 0x%04x)", domain, dns_id - 1);
        }
        
        apply_traffic_delay(traffic_config);
    }
}

void dns_rx_loop(uint16_t port_id) {
    struct rte_mbuf *pkts[BURST_SIZE];
    uint16_t nb_rx;
    
    LOG_INFO("Starting DNS RX loop on port %d", port_id);
    
    while (1) {
        nb_rx = rte_eth_rx_burst(port_id, 0, pkts, BURST_SIZE);
        
        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = pkts[i];
            struct rte_ether_hdr *eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
            
            if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                rte_pktmbuf_free(m);
                continue;
            }
            
            struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
            if (ip->next_proto_id != IPPROTO_UDP) {
                rte_pktmbuf_free(m);
                continue;
            }
            
            struct rte_udp_hdr *udp = (struct rte_udp_hdr *)(ip + 1);
            if (rte_be_to_cpu_16(udp->dst_port) != DNS_PORT) {
                rte_pktmbuf_free(m);
                continue;
            }
            
            struct dns_header *dns = (struct dns_header *)(udp + 1);
            uint16_t dns_id = rte_be_to_cpu_16(dns->id);
            uint16_t flags = rte_be_to_cpu_16(dns->flags);
            uint16_t qdcount = rte_be_to_cpu_16(dns->qdcount);
            
            LOG_INFO("DNS: Received packet - ID: 0x%04x, Flags: 0x%04x, Questions: %d", 
                    dns_id, flags, qdcount);
            
            rte_pktmbuf_free(m);
        }
    }
} 