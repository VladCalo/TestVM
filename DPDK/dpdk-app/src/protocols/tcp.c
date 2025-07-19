#include "../../include/protocols/tcp.h"
#include "../../include/core/common.h"
#include "../../include/core/config.h"
#include "../../include/core/log.h"
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_ethdev.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

void tcp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool) {
    const struct rte_ether_addr src = {SRC_MAC};
    const struct rte_ether_addr dst = {DST_MAC};
    
    while (1) {
        uint32_t tx_seq = 1000;
        uint32_t rx_ack = 0;

        // Step 1: Send SYN
        size_t pkt_size = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr);
        struct rte_mbuf *syn_pkt = allocate_packet(mbuf_pool, pkt_size);
        if (!syn_pkt) continue;

        char *data = rte_pktmbuf_mtod(syn_pkt, char *);
        struct rte_ether_hdr *eth = (struct rte_ether_hdr *)data;
        struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
        struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(ip + 1);

        setup_ethernet_header(eth, &src, &dst, RTE_ETHER_TYPE_IPV4);

        setup_ipv4_header(ip, SRC_IP, DST_IP, IPPROTO_TCP, sizeof(struct rte_tcp_hdr));

        tcp->src_port = rte_cpu_to_be_16(g_config.tcp_src_port);
        tcp->dst_port = rte_cpu_to_be_16(g_config.tcp_dst_port);
        tcp->sent_seq = rte_cpu_to_be_32(tx_seq);
        tcp->recv_ack = 0;
        tcp->data_off = (sizeof(struct rte_tcp_hdr) / 4) << 4;
        tcp->tcp_flags = RTE_TCP_SYN_FLAG;
        tcp->rx_win = rte_cpu_to_be_16(65535);
        tcp->cksum = 0;

        rte_eth_tx_burst(port_id, 0, &syn_pkt, 1);
        LOG_INFO("TX: Sent TCP SYN");

        usleep(100000);

        // Step 2: Wait for SYN-ACK
        int handshake_done = 0;
        struct rte_mbuf *bufs[BURST_SIZE];
        for (int retries = 0; retries < 5 && !handshake_done; retries++) {
            uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
            for (int i = 0; i < nb_rx; i++) {
                struct rte_ether_hdr *eth = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
                if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) continue;
                struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
                if (ip->next_proto_id != IPPROTO_TCP) continue;
                struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(ip + 1);

                if ((tcp->tcp_flags & (RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG)) == (RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG)) {
                    LOG_INFO("TX: Received TCP SYN-ACK");
                    rx_ack = rte_be_to_cpu_32(tcp->sent_seq) + 1;

                    // Step 3: Send ACK
                    struct rte_mbuf *ack_pkt = rte_pktmbuf_alloc(mbuf_pool);
                    char *ack_data = rte_pktmbuf_append(ack_pkt, pkt_size);
                    struct rte_ether_hdr *eth2 = (struct rte_ether_hdr *)ack_data;
                    struct rte_ipv4_hdr *ip2 = (struct rte_ipv4_hdr *)(eth2 + 1);
                    struct rte_tcp_hdr *tcp2 = (struct rte_tcp_hdr *)(ip2 + 1);

                    eth2->src_addr = eth->dst_addr;
                    eth2->dst_addr = eth->src_addr;
                    eth2->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

                    ip2->version_ihl = 0x45;
                    ip2->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr));
                    ip2->time_to_live = 64;
                    ip2->next_proto_id = IPPROTO_TCP;
                    ip2->src_addr = ip->dst_addr;
                    ip2->dst_addr = ip->src_addr;
                    ip2->hdr_checksum = 0;
                    ip2->hdr_checksum = calculate_checksum(ip2, sizeof(struct rte_ipv4_hdr));

                    tcp2->src_port = tcp->dst_port;
                    tcp2->dst_port = tcp->src_port;
                    tcp2->sent_seq = rte_cpu_to_be_32(tx_seq + 1);
                    tcp2->recv_ack = rte_cpu_to_be_32(rx_ack);
                    tcp2->data_off = (sizeof(struct rte_tcp_hdr) / 4) << 4;
                    tcp2->tcp_flags = RTE_TCP_ACK_FLAG;
                    tcp2->rx_win = rte_cpu_to_be_16(65535);
                    tcp2->cksum = 0;

                    rte_eth_tx_burst(port_id, 0, &ack_pkt, 1);
                    LOG_INFO("TX: Sent TCP ACK");
                    handshake_done = 1;
                }
                rte_pktmbuf_free(bufs[i]);
            }
        }

        // Step 4: Send Payload if handshake completed
        if (handshake_done) {
            struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);
            const char *msg = "Hello from TX";
            size_t msg_len = strlen(msg);
            size_t total_len = pkt_size + msg_len;
            char *payload = rte_pktmbuf_append(pkt, total_len);

            struct rte_ether_hdr *eth = (struct rte_ether_hdr *)payload;
            struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
            struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(ip + 1);
            char *data = (char *)(tcp + 1);

            eth->src_addr = (struct rte_ether_addr){{0x00,0x11,0x22,0x33,0x44,0x55}};
            eth->dst_addr = (struct rte_ether_addr){{0x52,0x54,0x00,0xa2,0x6c,0xb6}};
            eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

            ip->version_ihl = 0x45;
            ip->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr) + msg_len);
            ip->time_to_live = 64;
            ip->next_proto_id = IPPROTO_TCP;
            ip->src_addr = rte_cpu_to_be_32(SRC_IP);
            ip->dst_addr = rte_cpu_to_be_32(DST_IP);
            ip->hdr_checksum = 0;
            ip->hdr_checksum = calculate_checksum(ip, sizeof(struct rte_ipv4_hdr));

            tcp->src_port = rte_cpu_to_be_16(g_config.tcp_src_port);
            tcp->dst_port = rte_cpu_to_be_16(g_config.tcp_dst_port);
            tcp->sent_seq = rte_cpu_to_be_32(tx_seq + 1);
            tcp->recv_ack = rte_cpu_to_be_32(rx_ack);
            tcp->data_off = (sizeof(struct rte_tcp_hdr) / 4) << 4;
            tcp->tcp_flags = RTE_TCP_ACK_FLAG | RTE_TCP_PSH_FLAG;
            tcp->rx_win = rte_cpu_to_be_16(65535);
            tcp->cksum = 0;

            memcpy(data, msg, msg_len);
            rte_eth_tx_burst(port_id, 0, &pkt, 1);
            LOG_INFO("TX: Sent TCP payload: %s", msg);
        }

        sleep(1); // repeat
    }
}


void tcp_rx_loop(uint16_t port_id) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            struct rte_ether_hdr *eth = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
            if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) { rte_pktmbuf_free(bufs[i]); continue; }
            struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
            if (ip->next_proto_id != IPPROTO_TCP) { rte_pktmbuf_free(bufs[i]); continue; }
            struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(ip + 1);

            if ((tcp->tcp_flags & RTE_TCP_SYN_FLAG) && !(tcp->tcp_flags & RTE_TCP_ACK_FLAG)) {
                LOG_INFO("RX: Received TCP SYN, sending SYN-ACK");

                struct rte_mbuf *syn_ack = rte_pktmbuf_alloc(bufs[i]->pool);
                if (!syn_ack) { rte_pktmbuf_free(bufs[i]); continue; }

                char *pkt_data = rte_pktmbuf_append(syn_ack, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr));
                struct rte_ether_hdr *eth2 = (struct rte_ether_hdr *)pkt_data;
                struct rte_ipv4_hdr *ip2 = (struct rte_ipv4_hdr *)(eth2 + 1);
                struct rte_tcp_hdr *tcp2 = (struct rte_tcp_hdr *)(ip2 + 1);

                eth2->src_addr = eth->dst_addr;
                eth2->dst_addr = eth->src_addr;
                eth2->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

                ip2->version_ihl = 0x45;
                ip2->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr));
                ip2->time_to_live = 64;
                ip2->next_proto_id = IPPROTO_TCP;
                ip2->src_addr = ip->dst_addr;
                ip2->dst_addr = ip->src_addr;
                ip2->hdr_checksum = 0;
                ip2->hdr_checksum = calculate_checksum(ip2, sizeof(struct rte_ipv4_hdr));

                tcp2->src_port = tcp->dst_port;
                tcp2->dst_port = tcp->src_port;
                tcp2->sent_seq = rte_cpu_to_be_32(2000);
                tcp2->recv_ack = rte_cpu_to_be_32(rte_be_to_cpu_32(tcp->sent_seq) + 1);
                tcp2->data_off = (sizeof(struct rte_tcp_hdr) / 4) << 4;
                tcp2->tcp_flags = RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG;
                tcp2->rx_win = rte_cpu_to_be_16(65535);
                tcp2->cksum = 0;

                rte_eth_tx_burst(port_id, 0, &syn_ack, 1);
                LOG_INFO("RX: Sent TCP SYN-ACK");
            }
            else if ((tcp->tcp_flags & RTE_TCP_ACK_FLAG) && !(tcp->tcp_flags & RTE_TCP_PSH_FLAG)) {
                LOG_INFO("RX: Received final TCP ACK — handshake complete");

            } else if (tcp->tcp_flags & RTE_TCP_PSH_FLAG) {
                char *payload = (char *)(tcp + 1);
                int payload_len = rte_be_to_cpu_16(ip->total_length) - sizeof(struct rte_ipv4_hdr) - sizeof(struct rte_tcp_hdr);
                LOG_INFO("RX: Received TCP payload (%d bytes): %.*s", payload_len, payload_len, payload);
            }
            rte_pktmbuf_free(bufs[i]);
        }
    }
}