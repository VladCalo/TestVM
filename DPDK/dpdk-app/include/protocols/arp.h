#ifndef ARP_H
#define ARP_H

#include <rte_mempool.h>
#include <stdint.h>
#include "../core/traffic_modes.h"

void arp_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config);
void arp_rx_loop(uint16_t port_id);

#endif 