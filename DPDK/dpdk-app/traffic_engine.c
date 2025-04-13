#include "include/eth.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    uint16_t port_id = 0;
    int is_tx = 0;

    // Detect role: TX or RX
    if (argc > 1 && strcmp(argv[1], "tx") == 0)
        is_tx = 1;

    // Initialize EAL
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Failed to initialize EAL\n");

    argc -= ret;
    argv += ret;

    // Initialize Ethernet and create mempool
    struct rte_mempool *mbuf_pool = eth_init(port_id);
    if (!mbuf_pool)
        return 1;

    if (is_tx) {
        eth_tx_loop(port_id, mbuf_pool);
    } else {
        eth_rx_loop(port_id);
    }

    return 0;
}
