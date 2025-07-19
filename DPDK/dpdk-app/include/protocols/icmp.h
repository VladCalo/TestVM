#ifndef ICMP_H
#define ICMP_H

#include <rte_mempool.h>
#include <stdint.h>

enum protocol_type {
    PROTO_ETH,
    PROTO_ICMP,
    PROTO_UDP,
    PROTO_TCP
};

void icmp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool);
void icmp_rx_loop(uint16_t port_id);

#endif
