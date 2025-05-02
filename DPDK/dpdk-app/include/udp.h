#ifndef UDP_H
#define UDP_H

#include <rte_mempool.h>
#include <stdint.h>

void udp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool);
void udp_rx_loop(uint16_t port_id);

#endif
