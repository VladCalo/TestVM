#ifndef TCP_H
#define TCP_H

#include <rte_mempool.h>
#include <stdint.h>
#include "../core/traffic_modes.h"

void tcp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config, const char *message);
void tcp_rx_loop(uint16_t port_id);

#endif
