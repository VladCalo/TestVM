#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>  // for sleep()

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {0};

//develop proto icmp
int main(int argc, char *argv[]) {
    uint16_t port_id = 0;
    int is_tx = 0;

    // Check for TX or RX role
    if (argc > 1 && strcmp(argv[1], "tx") == 0) is_tx = 1;

    // Init EAL
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "Failed to initialize EAL\n");

    argc -= ret;
    argv += ret;

    // Create memory pool
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * 2,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    // Configure port
    if (rte_eth_dev_configure(port_id, 1, 1, &port_conf_default) < 0)
        rte_exit(EXIT_FAILURE, "Failed to configure port\n");

    // Setup RX and TX queues
    if (rte_eth_rx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL, mbuf_pool) < 0)
        rte_exit(EXIT_FAILURE, "RX queue setup failed\n");

    if (rte_eth_tx_queue_setup(port_id, 0, 1024, rte_eth_dev_socket_id(port_id), NULL) < 0)
        rte_exit(EXIT_FAILURE, "TX queue setup failed\n");

    if (rte_eth_dev_start(port_id) < 0)
        rte_exit(EXIT_FAILURE, "Failed to start device\n");

    printf("Started DPDK on port %u as %s\n", port_id, is_tx ? "TX" : "RX");

    if (is_tx) {
        while (1) {
            struct rte_mbuf *mbuf = rte_pktmbuf_alloc(mbuf_pool);
            if (!mbuf) continue;

            char *data = rte_pktmbuf_append(mbuf, 64);
            if (!data) {
                rte_pktmbuf_free(mbuf);
                continue;
            }

            struct rte_ether_hdr *eth_hdr = (struct rte_ether_hdr *)data;
            struct rte_ether_addr src = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
            struct rte_ether_addr dst = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}}; // broadcast

            eth_hdr->src_addr = src;
            eth_hdr->dst_addr = dst;
            eth_hdr->ether_type = rte_cpu_to_be_16(0x0800); // dummy ethertype

            char *payload = (char *)(eth_hdr + 1);
            snprintf(payload, 64 - sizeof(struct rte_ether_hdr), "Hello from TX VM");

            const uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, &mbuf, 1);
            if (nb_tx < 1) rte_pktmbuf_free(mbuf);

            sleep(1); // send every second
        }
    } else {
        struct rte_mbuf *bufs[BURST_SIZE];
        while (1) {
            uint16_t nb_rx = rte_eth_rx_burst(port_id, 0, bufs, BURST_SIZE);
            for (int i = 0; i < nb_rx; i++) {
                char *payload = rte_pktmbuf_mtod_offset(bufs[i], char *, sizeof(struct rte_ether_hdr));
                printf("Received: %s\n", payload);
                rte_pktmbuf_free(bufs[i]);
            }
        }
    }

    return 0;
}