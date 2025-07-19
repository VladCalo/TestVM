// traffic_engine.c
#include "../include/protocols/eth.h"
#include "../include/protocols/icmp.h"
#include "../include/protocols/udp.h"
#include "../include/protocols/tcp.h"
#include "../include/protocols/arp.h"
#include "../include/protocols/dns.h"
#include "../include/core/common.h"
#include "../include/core/config.h"
#include "../include/core/log.h"
#include "../include/core/traffic_modes.h"
#include <rte_eal.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    uint16_t port_id = 0;
    int is_tx = 0;
    enum protocol_type proto = PROTO_ETH;
    enum traffic_mode mode = TRAFFIC_CONTINUOUS;

    // Parse CLI: ./traffic_engine [tx|rx] [protocol] [traffic_mode]
    if (argc > 1 && strcmp(argv[1], "tx") == 0)
        is_tx = 1;

    if (argc > 2) {
        if (strcmp(argv[2], "icmp") == 0) proto = PROTO_ICMP;
        else if (strcmp(argv[2], "udp") == 0) proto = PROTO_UDP;
        else if (strcmp(argv[2], "tcp") == 0) proto = PROTO_TCP;
        else if (strcmp(argv[2], "arp") == 0) proto = PROTO_ARP;
        else if (strcmp(argv[2], "dns") == 0) proto = PROTO_DNS;
        else if (strcmp(argv[2], "eth") == 0) proto = PROTO_ETH;
    }

    if (argc > 3) {
        if (strcmp(argv[3], "continuous") == 0) mode = TRAFFIC_CONTINUOUS;
        else if (strcmp(argv[3], "burst") == 0) mode = TRAFFIC_BURST;
        else if (strcmp(argv[3], "rate-limited") == 0) mode = TRAFFIC_RATE_LIMITED;
        else if (strcmp(argv[3], "exponential-backoff") == 0) mode = TRAFFIC_EXPONENTIAL_BACKOFF;
    }

    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Failed to initialize EAL\n");

    argc -= ret;
    argv += ret;

    // Initialize configuration
    init_network_config();

    struct rte_mempool *mbuf_pool = eth_init(port_id);
    if (!mbuf_pool) return 1;

    if (is_tx) {
        LOG_INFO("Traffic Engine started - Mode: TX, Protocol: %s, Traffic: %s", 
                 proto == PROTO_ICMP ? "ICMP" : 
                 proto == PROTO_UDP ? "UDP" : 
                 proto == PROTO_TCP ? "TCP" : 
                 proto == PROTO_ARP ? "ARP" : 
                 proto == PROTO_DNS ? "DNS" : "ETH",
                 mode == TRAFFIC_CONTINUOUS ? "Continuous" :
                 mode == TRAFFIC_BURST ? "Burst" :
                 mode == TRAFFIC_RATE_LIMITED ? "Rate-Limited" :
                 mode == TRAFFIC_EXPONENTIAL_BACKOFF ? "Exponential Backoff" : "Continuous");
    } else {
        LOG_INFO("Traffic Engine started - Mode: RX, Protocol: %s", 
                 proto == PROTO_ICMP ? "ICMP" : 
                 proto == PROTO_UDP ? "UDP" : 
                 proto == PROTO_TCP ? "TCP" : 
                 proto == PROTO_ARP ? "ARP" : 
                 proto == PROTO_DNS ? "DNS" : "ETH");
    }

    traffic_config_t traffic_config;
    init_traffic_config(&traffic_config, mode);
    
    if (proto == PROTO_ICMP) {
        if (is_tx)
            icmp_tx_loop(port_id, mbuf_pool, &traffic_config);
        else
            icmp_rx_loop(port_id);
    } else if (proto == PROTO_UDP) {
        if (is_tx)
            udp_tx_loop(port_id, mbuf_pool, &traffic_config);
        else
            udp_rx_loop(port_id);
    } else if (proto == PROTO_TCP) {
        if (is_tx)
            tcp_tx_loop(port_id, mbuf_pool, &traffic_config);
        else
            tcp_rx_loop(port_id);
    } else if (proto == PROTO_ARP) {
        if (is_tx)
            arp_tx_loop(port_id, mbuf_pool, &traffic_config);
        else
            arp_rx_loop(port_id);
    } else if (proto == PROTO_DNS) {
        if (is_tx)
            dns_tx_loop(port_id, mbuf_pool, &traffic_config);
        else
            dns_rx_loop(port_id);
    } else {
        if (is_tx)
            eth_tx_loop(port_id, mbuf_pool, &traffic_config);
        else
            eth_rx_loop(port_id);
    }

    return 0;
}
