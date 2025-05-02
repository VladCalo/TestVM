#include "../include/tcp.h"
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_ethdev.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define BURST_SIZE 32
#define IPv4(a,b,c,d) ((uint32_t)(((a&0xff)<<24)|((b&0xff)<<16)|((c&0xff)<<8)|(d&0xff)))

#define TCP_SRC_PORT 11111
#define TCP_DST_PORT 22222

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

void tcp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool) {
    uint32_t tx_seq = 1000;
    uint32_t rx_ack = 0;
    int state = 0; // 0 = SYN sent, 1 = SYN-ACK received, 2 = ACK sent

    while (1) {
        if (state == 0) {
            struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
            if (!mbuf) continue;

            size_t pkt_size = sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr);
            char *pkt_data = rte_pktmbuf_append(mbuf, pkt_size);
            if (!pkt_data) {
                rte_pktmbuf_free(mbuf);
                continue;
            }

            struct rte_ether_hdr *eth = (struct rte_ether_hdr *)pkt_data;
            struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
            struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(ip + 1);

            eth->src_addr = (struct rte_ether_addr){{0x00,0x11,0x22,0x33,0x44,0x55}};
            eth->dst_addr = (struct rte_ether_addr){{0x52,0x54,0x00,0xa2,0x6c,0xb6}};
            eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

            ip->version_ihl = 0x45;
            ip->total_length = rte_cpu_to_be_16(sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr));
            ip->time_to_live = 64;
            ip->next_proto_id = IPPROTO_TCP;
            ip->src_addr = rte_cpu_to_be_32(IPv4(10,0,0,1));
            ip->dst_addr = rte_cpu_to_be_32(IPv4(10,0,0,2));
            ip->hdr_checksum = 0;
            ip->hdr_checksum = checksum(ip, sizeof(struct rte_ipv4_hdr));

            tcp->src_port = rte_cpu_to_be_16(TCP_SRC_PORT);
            tcp->dst_port = rte_cpu_to_be_16(TCP_DST_PORT);
            tcp->sent_seq = rte_cpu_to_be_32(tx_seq);
            tcp->recv_ack = 0;
            tcp->data_off = (sizeof(struct rte_tcp_hdr) / 4) << 4;
            tcp->tcp_flags = RTE_TCP_SYN_FLAG;
            tcp->rx_win = rte_cpu_to_be_16(65535);
            tcp->cksum = 0;

            rte_eth_tx_burst(port_id, 0, &mbuf, 1);
            printf("TX: Sent SYN with seq %u\n", tx_seq);
            state = 1;
            usleep(200000);
        }

        if (state == 1) {
            struct rte_mbuf *bufs[BURST_SIZE];
            uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
            for (int i = 0; i < nb_rx; i++) {
                struct rte_ether_hdr *eth = rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
                if (eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) { rte_pktmbuf_free(bufs[i]); continue; }
                struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)(eth + 1);
                if (ip->next_proto_id != IPPROTO_TCP) { rte_pktmbuf_free(bufs[i]); continue; }
                struct rte_tcp_hdr *tcp = (struct rte_tcp_hdr *)(ip + 1);

                if ((tcp->tcp_flags & (RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG)) == (RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG)) {
                    rx_ack = rte_be_to_cpu_32(tcp->sent_seq) + 1;
                    printf("TX: Got SYN-ACK with seq %u, sending ACK...\n", rte_be_to_cpu_32(tcp->sent_seq));

                    struct rte_mbuf *ack_pkt = rte_pktmbuf_alloc(mbuf_pool);
                    if (!ack_pkt) continue;

                    char *pkt_data = rte_pktmbuf_append(ack_pkt, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr));
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
                    ip2->hdr_checksum = checksum(ip2, sizeof(struct rte_ipv4_hdr));

                    tcp2->src_port = tcp->dst_port;
                    tcp2->dst_port = tcp->src_port;
                    tcp2->sent_seq = rte_cpu_to_be_32(tx_seq + 1);
                    tcp2->recv_ack = rte_cpu_to_be_32(rx_ack);
                    tcp2->data_off = (sizeof(struct rte_tcp_hdr) / 4) << 4;
                    tcp2->tcp_flags = RTE_TCP_ACK_FLAG;
                    tcp2->rx_win = rte_cpu_to_be_16(65535);
                    tcp2->cksum = 0;

                    rte_eth_tx_burst(port_id, 0, &ack_pkt, 1);
                    printf("TX: Sent ACK to complete handshake\n");
                    state = 2;
                }
                rte_pktmbuf_free(bufs[i]);
            }
        }

        if (state == 2) {
            sleep(2);
        }
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
                printf("RX: Got SYN from TX, sending SYN-ACK...\n");

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
                ip2->hdr_checksum = checksum(ip2, sizeof(struct rte_ipv4_hdr));

                tcp2->src_port = tcp->dst_port;
                tcp2->dst_port = tcp->src_port;
                tcp2->sent_seq = rte_cpu_to_be_32(2000);
                tcp2->recv_ack = rte_cpu_to_be_32(rte_be_to_cpu_32(tcp->sent_seq) + 1);
                tcp2->data_off = (sizeof(struct rte_tcp_hdr) / 4) << 4;
                tcp2->tcp_flags = RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG;
                tcp2->rx_win = rte_cpu_to_be_16(65535);
                tcp2->cksum = 0;

                rte_eth_tx_burst(port_id, 0, &syn_ack, 1);
                printf("RX: Sent SYN-ACK\n");
            }
            else if ((tcp->tcp_flags & (RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG)) == RTE_TCP_ACK_FLAG) {
                printf("RX: Got final ACK — handshake complete\n");
            }
            rte_pktmbuf_free(bufs[i]);
        }
    }
}