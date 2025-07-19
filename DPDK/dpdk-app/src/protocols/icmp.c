#include "../../include/protocols/icmp.h"
#include "../../include/core/common.h"
#include "../../include/core/config.h"
#include "../../include/core/log.h"
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_icmp.h>
#include <rte_byteorder.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void icmp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool) {
    const struct rte_ether_addr src = {SRC_MAC};
    const struct rte_ether_addr dst = {DST_MAC};
    
    while (1) {
        size_t total_len = sizeof(struct rte_ether_hdr) + 
                          sizeof(struct rte_ipv4_hdr) + 
                          sizeof(struct rte_icmp_hdr);
        
        struct rte_mbuf *mbuf = allocate_packet(mbuf_pool, total_len);
        if (!mbuf) continue;

        char *pkt_data = rte_pktmbuf_mtod(mbuf, char *);
        
        // Setup Ethernet header
        struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt_data;
        setup_ethernet_header(eth_hdr, &src, &dst, RTE_ETHER_TYPE_IPV4);

        // Setup IP header
        struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
        setup_ipv4_header(ip_hdr, SRC_IP, DST_IP, IPPROTO_ICMP, sizeof(struct rte_icmp_hdr));

        struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)(ip_hdr + 1);
        icmp->icmp_type = 8; // Echo Request
        icmp->icmp_code = 0;
        icmp->icmp_ident = rte_cpu_to_be_16(0x1234);
        icmp->icmp_seq_nb = rte_cpu_to_be_16(1);
        icmp->icmp_cksum = 0;
        icmp->icmp_cksum = calculate_checksum(icmp, sizeof(struct rte_icmp_hdr));

        rte_eth_tx_burst(port_id, 0, &mbuf, 1);
        LOG_INFO("TX: Sent ICMP Echo Request");
        usleep(100000);  // wait for reply

        struct rte_mbuf *bufs[BURST_SIZE];
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            struct rte_ether_hdr *eth = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
            if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                rte_pktmbuf_free(bufs[i]);
                continue;
            }

            struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
            if (ip->next_proto_id != IPPROTO_ICMP) {
                rte_pktmbuf_free(bufs[i]);
                continue;
            }

            struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)(ip + 1);
            if (icmp->icmp_type == 0) {
                LOG_INFO("TX: Received ICMP Echo Reply from %d.%d.%d.%d", 
                        (rte_be_to_cpu_32(ip->src_addr) >> 24) & 0xFF,
                        (rte_be_to_cpu_32(ip->src_addr) >> 16) & 0xFF,
                        (rte_be_to_cpu_32(ip->src_addr) >> 8) & 0xFF,
                        rte_be_to_cpu_32(ip->src_addr) & 0xFF);
            }

            rte_pktmbuf_free(bufs[i]);
        }

        sleep(1);
    }
}

void icmp_rx_loop(uint16_t port_id) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *rx_pkt = bufs[i];
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(rx_pkt, struct rte_ether_hdr *);
            if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                rte_pktmbuf_free(rx_pkt);
                continue;
            }

            struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
            if (ip_hdr->next_proto_id != IPPROTO_ICMP) {
                rte_pktmbuf_free(rx_pkt);
                continue;
            }

            struct rte_icmp_hdr *icmp = (struct rte_icmp_hdr *)(ip_hdr + 1);
            LOG_INFO("RX: Received ICMP type %u, code %u from %d.%d.%d.%d", 
                    icmp->icmp_type, icmp->icmp_code,
                    (rte_be_to_cpu_32(ip_hdr->src_addr) >> 24) & 0xFF,
                    (rte_be_to_cpu_32(ip_hdr->src_addr) >> 16) & 0xFF,
                    (rte_be_to_cpu_32(ip_hdr->src_addr) >> 8) & 0xFF,
                    rte_be_to_cpu_32(ip_hdr->src_addr) & 0xFF);

            if (icmp->icmp_type == 8) { // Echo Request
                struct rte_mbuf *tx_pkt = rte_pktmbuf_alloc(rx_pkt->pool);
                if (!tx_pkt) {
                    rte_pktmbuf_free(rx_pkt);
                    continue;
                }

                char *data = rte_pktmbuf_append(tx_pkt,
                    sizeof(struct rte_ether_hdr) +
                    sizeof(struct rte_ipv4_hdr) +
                    sizeof(struct rte_icmp_hdr));

                if (!data) {
                    rte_pktmbuf_free(tx_pkt);
                    rte_pktmbuf_free(rx_pkt);
                    continue;
                }

                struct rte_ether_hdr *tx_eth = (struct rte_ether_hdr *)data;
                tx_eth->src_addr = eth_hdr->dst_addr;
                tx_eth->dst_addr = eth_hdr->src_addr;
                tx_eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

                struct rte_ipv4_hdr *tx_ip = (struct rte_ipv4_hdr *)(tx_eth + 1);
                *tx_ip = *ip_hdr;
                tx_ip->src_addr = ip_hdr->dst_addr;
                tx_ip->dst_addr = ip_hdr->src_addr;
                tx_ip->hdr_checksum = 0;
                tx_ip->hdr_checksum = calculate_checksum(tx_ip, sizeof(struct rte_ipv4_hdr));

                struct rte_icmp_hdr *tx_icmp = (struct rte_icmp_hdr *)(tx_ip + 1);
                tx_icmp->icmp_type = 0;
                tx_icmp->icmp_code = 0;
                tx_icmp->icmp_ident = icmp->icmp_ident;
                tx_icmp->icmp_seq_nb = icmp->icmp_seq_nb;
                tx_icmp->icmp_cksum = 0;
                tx_icmp->icmp_cksum = calculate_checksum(tx_icmp, sizeof(struct rte_icmp_hdr));

                LOG_INFO("RX: Sending ICMP Echo Reply to %d.%d.%d.%d", 
                        (rte_be_to_cpu_32(ip_hdr->src_addr) >> 24) & 0xFF,
                        (rte_be_to_cpu_32(ip_hdr->src_addr) >> 16) & 0xFF,
                        (rte_be_to_cpu_32(ip_hdr->src_addr) >> 8) & 0xFF,
                        rte_be_to_cpu_32(ip_hdr->src_addr) & 0xFF);
                rte_eth_tx_burst(port_id, 0, &tx_pkt, 1);
            }

            rte_pktmbuf_free(rx_pkt);
        }
    }
}
