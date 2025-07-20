#ifndef DNS_H
#define DNS_H

#include <rte_mempool.h>
#include <stdint.h>
#include "../core/traffic_modes.h"

void dns_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config, const char *domain);
void dns_rx_loop(uint16_t port_id);

#endif 