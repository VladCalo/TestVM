#include "../../include/core/common.h"
#include <rte_byteorder.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_tcp.h>
#include <rte_icmp.h>
#include <rte_arp.h>
#include <rte_ethdev.h>
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

struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed));

void agnostic_rx_loop(uint16_t port_id) {
    struct rte_mbuf *pkts[BURST_SIZE];
    uint16_t nb_rx;
    
    printf("Starting Agnostic RX loop on port %d\n", port_id);
    printf("Waiting for packets from any protocol...\n");
    
    while (1) {
        nb_rx = rte_eth_rx_burst(port_id, 0, pkts, BURST_SIZE);
        
        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *m = pkts[i];
            struct rte_ether_hdr *eth = rte_pktmbuf_mtod(m, struct rte_ether_hdr *);
            
            printf("=== Received Packet ===\n");
            printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   eth->src_addr.addr_bytes[0], eth->src_addr.addr_bytes[1],
                   eth->src_addr.addr_bytes[2], eth->src_addr.addr_bytes[3],
                   eth->src_addr.addr_bytes[4], eth->src_addr.addr_bytes[5]);
            printf("Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   eth->dst_addr.addr_bytes[0], eth->dst_addr.addr_bytes[1],
                   eth->dst_addr.addr_bytes[2], eth->dst_addr.addr_bytes[3],
                   eth->dst_addr.addr_bytes[4], eth->dst_addr.addr_bytes[5]);
            
            uint16_t ether_type = rte_be_to_cpu_16(eth->ether_type);
            printf("Ethernet Type: 0x%04x", ether_type);
            
            if (ether_type == RTE_ETHER_TYPE_IPV4) {
                printf(" (IPv4)\n");
                struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
                
                printf("Source IP: %d.%d.%d.%d\n",
                       (rte_be_to_cpu_32(ip->src_addr) >> 24) & 0xFF,
                       (rte_be_to_cpu_32(ip->src_addr) >> 16) & 0xFF,
                       (rte_be_to_cpu_32(ip->src_addr) >> 8) & 0xFF,
                       rte_be_to_cpu_32(ip->src_addr) & 0xFF);
                printf("Dest IP: %d.%d.%d.%d\n",
                       (rte_be_to_cpu_32(ip->dst_addr) >> 24) & 0xFF,
                       (rte_be_to_cpu_32(ip->dst_addr) >> 16) & 0xFF,
                       (rte_be_to_cpu_32(ip->dst_addr) >> 8) & 0xFF,
                       rte_be_to_cpu_32(ip->dst_addr) & 0xFF);
                
                uint8_t protocol = ip->next_proto_id;
                printf("Protocol: %d", protocol);
                
                if (protocol == IPPROTO_ICMP) {
                    printf(" (ICMP)\n");
                    struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)(ip + 1);
                    uint8_t icmp_type = icmp->icmp_type;
                    uint8_t icmp_code = icmp->icmp_code;
                    printf("ICMP Type: %d, Code: %d\n", icmp_type, icmp_code);
                    
                    if (icmp_type == 8) {
                        printf("Protocol: ICMP Echo Request\n");
                    } else if (icmp_type == 0) {
                        printf("Protocol: ICMP Echo Reply\n");
                    }
                    
                } else if (protocol == IPPROTO_UDP) {
                    printf(" (UDP)\n");
                    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)(ip + 1);
                    uint16_t src_port = rte_be_to_cpu_16(udp->src_port);
                    uint16_t dst_port = rte_be_to_cpu_16(udp->dst_port);
                    printf("Source Port: %d, Dest Port: %d\n", src_port, dst_port);
                    
                    if (dst_port == 53) {
                        printf("Protocol: DNS Query\n");
                        struct dns_header *dns = (struct dns_header *)(udp + 1);
                        uint16_t dns_id = rte_be_to_cpu_16(dns->id);
                        printf("DNS ID: 0x%04x\n", dns_id);
                    } else {
                        printf("Protocol: UDP\n");
                        char *payload = (char *)(udp + 1);
                        int payload_len = rte_be_to_cpu_16(udp->dgram_len) - sizeof(struct rte_udp_hdr);
                        if (payload_len > 0 && payload_len < 100) {
                            printf("Payload: %.*s\n", payload_len, payload);
                        }
                    }
                    
                } else if (protocol == IPPROTO_TCP) {
                    printf(" (TCP)\n");
                    struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(ip + 1);
                    uint16_t src_port = rte_be_to_cpu_16(tcp->src_port);
                    uint16_t dst_port = rte_be_to_cpu_16(tcp->dst_port);
                    printf("Source Port: %d, Dest Port: %d\n", src_port, dst_port);
                    
                    uint16_t tcp_flags = tcp->tcp_flags;
                    printf("TCP Flags: 0x%02x", tcp_flags);
                    if (tcp_flags & RTE_TCP_SYN_FLAG) printf(" SYN");
                    if (tcp_flags & RTE_TCP_ACK_FLAG) printf(" ACK");
                    if (tcp_flags & RTE_TCP_PSH_FLAG) printf(" PSH");
                    if (tcp_flags & RTE_TCP_FIN_FLAG) printf(" FIN");
                    if (tcp_flags & RTE_TCP_RST_FLAG) printf(" RST");
                    printf("\n");
                    
                    if (tcp_flags & RTE_TCP_PSH_FLAG) {
                        char *payload = (char *)(tcp + 1);
                        int payload_len = rte_be_to_cpu_16(ip->total_length) - sizeof(struct rte_ipv4_hdr) - sizeof(struct rte_tcp_hdr);
                        if (payload_len > 0 && payload_len < 100) {
                            printf("Payload: %.*s\n", payload_len, payload);
                        }
                    }
                    
                    printf("Protocol: TCP\n");
                }
                
            } else if (ether_type == RTE_ETHER_TYPE_ARP) {
                printf(" (ARP)\n");
                struct rte_arp_hdr *arp = (struct rte_arp_hdr *)(eth + 1);
                uint16_t opcode = rte_be_to_cpu_16(arp->arp_opcode);
                printf("ARP Opcode: %d", opcode);
                if (opcode == RTE_ARP_OP_REQUEST) {
                    printf(" (Request)\n");
                } else if (opcode == RTE_ARP_OP_REPLY) {
                    printf(" (Reply)\n");
                } else {
                    printf("\n");
                }
                
                uint32_t sender_ip = rte_be_to_cpu_32(arp->arp_data.arp_sip);
                uint32_t target_ip = rte_be_to_cpu_32(arp->arp_data.arp_tip);
                printf("Sender IP: %d.%d.%d.%d\n",
                       (sender_ip >> 24) & 0xFF, (sender_ip >> 16) & 0xFF,
                       (sender_ip >> 8) & 0xFF, sender_ip & 0xFF);
                printf("Target IP: %d.%d.%d.%d\n",
                       (target_ip >> 24) & 0xFF, (target_ip >> 16) & 0xFF,
                       (target_ip >> 8) & 0xFF, target_ip & 0xFF);
                
                printf("Protocol: ARP\n");
                
            } else {
                printf(" (Unknown)\n");
                printf("Protocol: Unknown Ethernet Type\n");
            }
            
            printf("========================\n\n");
            rte_pktmbuf_free(m);
        }
    }
} 