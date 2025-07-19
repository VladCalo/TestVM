#ifndef ICMP_H
#define ICMP_H

#include <rte_mempool.h>
#include <stdint.h>
#include "../core/traffic_modes.h"

void icmp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config);
void icmp_rx_loop(uint16_t port_id);

#endif
