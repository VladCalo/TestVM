#ifndef ETH_H
#define ETH_H

#include <rte_mempool.h>

struct rte_mempool* eth_init(uint16_t port_id);
void eth_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool);
void eth_rx_loop(uint16_t port_id);

#endif
