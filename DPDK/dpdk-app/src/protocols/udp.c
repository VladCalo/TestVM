#include "../../include/protocols/udp.h"
#include "../../include/core/common.h"
#include "../../include/core/config.h"
#include "../../include/core/log.h"
#include "../../include/core/traffic_modes.h"
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>



void udp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config, const char *message) {
    const struct rte_ether_addr src = {SRC_MAC};
    const struct rte_ether_addr dst = {DST_MAC};
    
    while (1) {
        const size_t payload_len = strlen(message) + 1;
        const size_t total_len = sizeof(struct rte_ether_hdr) + 
                                sizeof(struct rte_ipv4_hdr) +
                                sizeof(struct rte_udp_hdr) + payload_len;

        struct rte_mbuf *mbuf = allocate_packet(mbuf_pool, total_len);
        if (!mbuf) continue;

        char *pkt_data = rte_pktmbuf_mtod(mbuf, char *);

        struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)pkt_data;
        setup_ethernet_header(eth_hdr, &src, &dst, RTE_ETHER_TYPE_IPV4);

        struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
        setup_ipv4_header(ip_hdr, SRC_IP, DST_IP, IPPROTO_UDP, 
                         sizeof(struct rte_udp_hdr) + payload_len);

        struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);
        udp_hdr->src_port = rte_cpu_to_be_16(g_config.udp_src_port);
        udp_hdr->dst_port = rte_cpu_to_be_16(g_config.udp_dst_port);
        udp_hdr->dgram_len = rte_cpu_to_be_16(sizeof(struct rte_udp_hdr) + payload_len);
        udp_hdr->dgram_cksum = 0;

        char *payload = (char *)(udp_hdr + 1);
        memcpy(payload, message, payload_len);

        rte_eth_tx_burst(port_id, 0, &mbuf, 1);
        LOG_INFO("UDP: Sent packet with payload: %s", message);
        apply_traffic_delay(traffic_config);
    }
}

void udp_rx_loop(uint16_t port_id) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            struct rte_mbuf *mbuf = bufs[i];
            struct rte_ether_hdr *eth_hdr = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);

            if (eth_hdr->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
                rte_pktmbuf_free(mbuf);
                continue;
            }

            struct rte_ipv4_hdr *ip_hdr = (struct rte_ipv4_hdr *)(eth_hdr + 1);
            if (ip_hdr->next_proto_id != IPPROTO_UDP) {
                rte_pktmbuf_free(mbuf);
                continue;
            }

            struct rte_udp_hdr *udp_hdr = (struct rte_udp_hdr *)(ip_hdr + 1);
            char *payload = (char *)(udp_hdr + 1);

            LOG_INFO("UDP: Received packet from %d.%d.%d.%d:%u, payload: %s",
                    (rte_be_to_cpu_32(ip_hdr->src_addr) >> 24) & 0xFF,
                    (rte_be_to_cpu_32(ip_hdr->src_addr) >> 16) & 0xFF,
                    (rte_be_to_cpu_32(ip_hdr->src_addr) >> 8) & 0xFF,
                    rte_be_to_cpu_32(ip_hdr->src_addr) & 0xFF,
                    rte_be_to_cpu_16(udp_hdr->src_port), payload);

            rte_pktmbuf_free(mbuf);
        }
    }
}
