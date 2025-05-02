// traffic_engine.c
#include "include/eth.h"
#include "include/icmp.h"
#include "include/udp.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    uint16_t port_id = 0;
    int is_tx = 0;
    enum protocol_type proto = PROTO_ETH;

    // Parse CLI: ./traffic_engine [tx|rx] [eth|icmp|udp]
    if (argc > 1 && strcmp(argv[1], "tx") == 0)
        is_tx = 1;

    if (argc > 2) {
        if (strcmp(argv[2], "icmp") == 0) proto = PROTO_ICMP;
        else if (strcmp(argv[2], "udp") == 0) proto = PROTO_UDP;
        else if (strcmp(argv[2], "eth") == 0) proto = PROTO_ETH;
    }

    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Failed to initialize EAL\n");

    argc -= ret;
    argv += ret;

    struct rte_mempool *mbuf_pool = eth_init(port_id);
    if (!mbuf_pool) return 1;

    if (proto == PROTO_ICMP) {
        if (is_tx)
            icmp_tx_loop(port_id, mbuf_pool);
        else
            icmp_rx_loop(port_id);
    } else if (proto == PROTO_UDP) {
        if (is_tx)
            udp_tx_loop(port_id, mbuf_pool);
        else
            udp_rx_loop(port_id);
    } else {
        if (is_tx)
            eth_tx_loop(port_id, mbuf_pool);
        else
            eth_rx_loop(port_id);
    }

    return 0;
}
