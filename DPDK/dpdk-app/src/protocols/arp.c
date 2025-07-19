#include "../../include/protocols/arp.h"
#include "../../include/core/common.h"
#include "../../include/core/config.h"
#include "../../include/core/log.h"
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_arp.h>
#include <rte_byteorder.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void arp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool) {
    const struct rte_ether_addr src = {SRC_MAC};
    const struct rte_ether_addr dst = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
    
    while (1) {
        size_t total_len = sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr);
        
        struct rte_mbuf *mbuf = allocate_packet(mbuf_pool, total_len);
        if (!mbuf) continue;

        char *pkt_data = rte_pktmbuf_mtod(mbuf, char *);

        struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt_data;
        setup_ethernet_header(eth_hdr, &src, &dst, RTE_ETHER_TYPE_ARP);

        struct rte_arp_hdr *arp_hdr = (struct rte_arp_hdr *)(eth_hdr + 1);
        arp_hdr->arp_hardware = rte_cpu_to_be_16(RTE_ARP_HRD_ETHER);
        arp_hdr->arp_protocol = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
        arp_hdr->arp_hlen = 6;
        arp_hdr->arp_plen = 4;
        arp_hdr->arp_opcode = rte_cpu_to_be_16(RTE_ARP_OP_REQUEST);

        memcpy(&arp_hdr->arp_data.arp_sha, &src, sizeof(struct rte_ether_addr));
        memset(&arp_hdr->arp_data.arp_tha, 0, sizeof(struct rte_ether_addr));
        arp_hdr->arp_data.arp_sip = rte_cpu_to_be_32(SRC_IP);
        arp_hdr->arp_data.arp_tip = rte_cpu_to_be_32(DST_IP);

        rte_eth_tx_burst(port_id, 0, &mbuf, 1);
        LOG_INFO("TX: Sent ARP Request for %d.%d.%d.%d", 
                (DST_IP >> 24) & 0xFF, (DST_IP >> 16) & 0xFF,
                (DST_IP >> 8) & 0xFF, DST_IP & 0xFF);
        
        usleep(100000);

        struct rte_mbuf *bufs[BURST_SIZE];
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            struct rte_ether_hdr *eth = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
            if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP)) {
                rte_pktmbuf_free(bufs[i]);
                continue;
            }

            struct rte_arp_hdr *arp = (struct rte_arp_hdr *)(eth + 1);
            if (rte_be_to_cpu_16(arp->arp_opcode) == RTE_ARP_OP_REPLY) {
                uint32_t sender_ip = rte_be_to_cpu_32(arp->arp_data.arp_sip);
                LOG_INFO("TX: Received ARP Reply from %d.%d.%d.%d", 
                        (sender_ip >> 24) & 0xFF, (sender_ip >> 16) & 0xFF,
                        (sender_ip >> 8) & 0xFF, sender_ip & 0xFF);
            }

            rte_pktmbuf_free(bufs[i]);
        }

        sleep(1);
    }
}

void arp_rx_loop(uint16_t port_id) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *rx_pkt = bufs[i];
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(rx_pkt, struct rte_ether_hdr *);
            
            if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP)) {
                rte_pktmbuf_free(rx_pkt);
                continue;
            }

            struct rte_arp_hdr *arp_hdr = (struct rte_arp_hdr *)(eth_hdr + 1);
            uint16_t opcode = rte_be_to_cpu_16(arp_hdr->arp_opcode);
            uint32_t sender_ip = rte_be_to_cpu_32(arp_hdr->arp_data.arp_sip);
            uint32_t target_ip = rte_be_to_cpu_32(arp_hdr->arp_data.arp_tip);

            LOG_INFO("RX: Received ARP %s from %d.%d.%d.%d", 
                    opcode == RTE_ARP_OP_REQUEST ? "Request" : "Reply",
                    (sender_ip >> 24) & 0xFF, (sender_ip >> 16) & 0xFF,
                    (sender_ip >> 8) & 0xFF, sender_ip & 0xFF);

            if (opcode == RTE_ARP_OP_REQUEST && target_ip == DST_IP) {
                struct rte_mbuf *tx_pkt = rte_pktmbuf_alloc(rx_pkt->pool);
                if (!tx_pkt) {
                    rte_pktmbuf_free(rx_pkt);
                    continue;
                }

                char *data = rte_pktmbuf_append(tx_pkt, sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr));
                if (!data) {
                    rte_pktmbuf_free(tx_pkt);
                    rte_pktmbuf_free(rx_pkt);
                    continue;
                }

                struct rte_ether_hdr *tx_eth = (struct rte_ether_hdr *)data;
                tx_eth->src_addr = eth_hdr->dst_addr;
                tx_eth->dst_addr = eth_hdr->src_addr;
                tx_eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP);

                struct rte_arp_hdr *tx_arp = (struct rte_arp_hdr *)(tx_eth + 1);
                tx_arp->arp_hardware = rte_cpu_to_be_16(RTE_ARP_HRD_ETHER);
                tx_arp->arp_protocol = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
                tx_arp->arp_hlen = 6;
                tx_arp->arp_plen = 4;
                tx_arp->arp_opcode = rte_cpu_to_be_16(RTE_ARP_OP_REPLY);

                const struct rte_ether_addr src = {SRC_MAC};
                memcpy(&tx_arp->arp_data.arp_sha, &src, sizeof(struct rte_ether_addr));
                memcpy(&tx_arp->arp_data.arp_tha, &arp_hdr->arp_data.arp_sha, sizeof(struct rte_ether_addr));
                tx_arp->arp_data.arp_sip = rte_cpu_to_be_32(DST_IP);
                tx_arp->arp_data.arp_tip = rte_cpu_to_be_32(sender_ip);

                LOG_INFO("RX: Sending ARP Reply to %d.%d.%d.%d", 
                        (sender_ip >> 24) & 0xFF, (sender_ip >> 16) & 0xFF,
                        (sender_ip >> 8) & 0xFF, sender_ip & 0xFF);
                rte_eth_tx_burst(port_id, 0, &tx_pkt, 1);
            }

            rte_pktmbuf_free(rx_pkt);
        }
    }
} 