#ifndef ETH_H
#define ETH_H

#include <rte_mempool.h>
#include <stdint.h>
#include "../core/traffic_modes.h"

struct rte_mempool* eth_init(uint16_t port_id);
void eth_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config);
void eth_rx_loop(uint16_t port_id);

#endif
