#include "../include/icmp.h"
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_icmp.h>
#include <rte_byteorder.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define BURST_SIZE 32
#define IPv4(a, b, c, d) ((uint32_t)(((a & 0xff) << 24) | ((b & 0xff) << 16) | ((c & 0xff) << 8) | (d & 0xff)))

static uint16_t checksum(const void *data, size_t len) {
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

void icmp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool) {
    while (1) {
        struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
        if (!mbuf) continue;

        char *pkt_data = rte_pktmbuf_append(mbuf, sizeof(struct rte_ether_hdr) +
                                                    sizeof(struct rte_ipv4_hdr) +
                                                    sizeof(struct rte_icmp_hdr));
        if (!pkt_data) {
            rte_pktmbuf_free(mbuf);
            continue;
        }

        struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt_data;
        struct rte_ether_addr src = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
        struct rte_ether_addr dst = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
        eth_hdr->src_addr = src;
        eth_hdr->dst_addr = dst;
        eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

        struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
        ip_hdr->version_ihl = (4 << 4) | (sizeof(struct rte_ipv4_hdr) / 4);
        ip_hdr->type_of_service = 0;
        ip_hdr->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_icmp_hdr));
        ip_hdr->packet_id = 0;
        ip_hdr->fragment_offset = 0;
        ip_hdr->time_to_live = 64;
        ip_hdr->next_proto_id = IPPROTO_ICMP;
        ip_hdr->hdr_checksum = 0;
        ip_hdr->src_addr = rte_cpu_to_be_32(IPv4(10, 0, 0, 1));
        ip_hdr->dst_addr = rte_cpu_to_be_32(IPv4(10, 0, 0, 2));
        ip_hdr->hdr_checksum = checksum(ip_hdr, sizeof(struct rte_ipv4_hdr));

        struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)(ip_hdr + 1);
        icmp->icmp_type = RTE_IP_ICMP_ECHO_REQUEST;
        icmp->icmp_code = 0;
        icmp->icmp_ident = rte_cpu_to_be_16(0x1234);
        icmp->icmp_seq_nb = rte_cpu_to_be_16(1);
        icmp->icmp_cksum = 0;
        icmp->icmp_cksum = checksum(icmp, sizeof(struct rte_icmp_hdr));

        rte_eth_tx_burst(port_id, 0, &mbuf, 1);
        sleep(1);
    }
}

void icmp_rx_loop(uint16_t port_id) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
            if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                rte_pktmbuf_free(bufs[i]);
                continue;
            }

            struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
            if (ip_hdr->next_proto_id != IPPROTO_ICMP) {
                rte_pktmbuf_free(bufs[i]);
                continue;
            }

            struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)(ip_hdr + 1);
            printf("Received ICMP type %u, code %u\n", icmp->icmp_type, icmp->icmp_code);

            rte_pktmbuf_free(bufs[i]);
        }
    }
}
