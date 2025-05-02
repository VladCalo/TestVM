#ifndef TCP_H
#define TCP_H

#include <rte_mempool.h>
#include <stdint.h>

void tcp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool);
void tcp_rx_loop(uint16_t port_id);

#endif
