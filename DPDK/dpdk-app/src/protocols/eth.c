#include "../../include/protocols/eth.h"
#include "../../include/core/common.h"
#include "../../include/core/config.h"
#include "../../include/core/log.h"
#include "../../include/core/traffic_modes.h"
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const struct rte_eth_conf port_conf_default = {0};

struct rte_mempool* eth_init(uint16_t port_id) {
    char pool_name[32];
    snprintf(pool_name, sizeof(pool_name), "MBUF_POOL_%u", port_id);

    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(pool_name, NUM_MBUFS * 2,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (!mbuf_pool) {
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
        return NULL;
    }

    if (rte_eth_dev_configure(port_id, 1, 1, &port_conf_default) < 0)
        rte_exit(EXIT_FAILURE, "Failed to configure port\n");

    if (rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool) < 0)
        rte_exit(EXIT_FAILURE, "RX queue setup failed\n");

    if (rte_eth_tx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL) < 0)
        rte_exit(EXIT_FAILURE, "TX queue setup failed\n");

    if (rte_eth_dev_start(port_id) < 0)
        rte_exit(EXIT_FAILURE, "Failed to start device\n");

    LOG_INFO("Started DPDK on port %u", port_id);
    return mbuf_pool;
}

void eth_tx_loop(uint16_t port_id, struct rte_mempool *mbuf_pool, traffic_config_t *traffic_config) {
    const struct rte_ether_addr src = {SRC_MAC};
    const struct rte_ether_addr dst = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}; // Broadcast
    
    while (1) {
        struct rte_mbuf *mbuf = allocate_packet(mbuf_pool, 64);
        if (!mbuf) continue;

        char *data = rte_pktmbuf_mtod(mbuf, char *);

        // Setup Ethernet header
        struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)data;
        setup_ethernet_header(eth_hdr, &src, &dst, 0x0800);

        char *payload = (char *)(eth_hdr + 1);
        snprintf(payload, 64 - sizeof(struct rte_ether_hdr), "Hello from TX VM");

        const uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, &mbuf, 1);
        if (nb_tx < 1) {
            rte_pktmbuf_free(mbuf);
            LOG_WARN("ETH: Failed to send frame");
        } else {
            LOG_INFO("ETH: Sent frame with payload: %s", payload);
        }

        apply_traffic_delay(traffic_config);
    }
}

void eth_rx_loop(uint16_t port_id) {
    struct rte_mbuf *bufs[BURST_SIZE];

    while (1) {
        uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
        for (int i = 0; i < nb_rx; i++) {
            char *payload = rte_pktmbuf_mtod_offset(bufs[i], char *, sizeof(struct rte_ether_hdr));
            LOG_INFO("ETH: Received frame: %s", payload);
            rte_pktmbuf_free(bufs[i]);
        }
    }
}
