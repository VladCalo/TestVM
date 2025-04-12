#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define SOCKET_PATH "/tmp/dpdk_sock"

static const uint16_t rx_port = 0;
static struct rte_mempool *mbuf_pool;

static int init_port(uint16_t port) {
    struct rte_eth_conf port_conf = {0};
    const uint16_t rx_rings = 1, tx_rings = 1;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_rxconf rxq_conf;
    struct rte_eth_txconf txq_conf;

    rte_eth_dev_info_get(port, &dev_info);

    rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);

    rxq_conf = dev_info.default_rxconf;
    txq_conf = dev_info.default_txconf;

    rte_eth_rx_queue_setup(port, 0, 128, rte_eth_dev_socket_id(port), &rxq_conf, mbuf_pool);
    rte_eth_tx_queue_setup(port, 0, 128, rte_eth_dev_socket_id(port), &txq_conf);

    rte_eth_dev_start(port);

    struct rte_ether_addr addr;
    rte_eth_macaddr_get(port, &addr);
    printf("Port %u MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", port,
        addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2],
        addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);

    rte_eth_promiscuous_enable(port);
    return 0;
}

static int init_socket() {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    unlink(SOCKET_PATH);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    listen(sockfd, 1);
    return accept(sockfd, NULL, NULL);
}

static void dpdk_loop(int client_sock) {
    struct rte_mbuf *bufs[BURST_SIZE];
    uint8_t rx_buf[2048];

    while (1) {
        // RX from NIC
        uint16_t nb_rx = rte_eth_rx_burst(rx_port, 0, bufs, BURST_SIZE);
        for (uint16_t i = 0; i < nb_rx; i++) {
            uint16_t len = rte_pktmbuf_pkt_len(bufs[i]);
	    printf("Received packet of length: %u\n", len);
            rte_memcpy(rx_buf, rte_pktmbuf_mtod(bufs[i], void *), len);
            send(client_sock, &len, sizeof(len), 0);
            send(client_sock, rx_buf, len, 0);
            rte_pktmbuf_free(bufs[i]);
        }

        // TX from socket
        uint16_t pkt_len;
        ssize_t n = recv(client_sock, &pkt_len, sizeof(pkt_len), MSG_DONTWAIT);
        if (n == sizeof(pkt_len) && pkt_len <= 2048) {
            struct rte_mbuf *tx_buf = rte_pktmbuf_alloc(mbuf_pool);
            if (!tx_buf) continue;
            void *data = rte_pktmbuf_mtod(tx_buf, void *);
            recv(client_sock, data, pkt_len, MSG_WAITALL);
            tx_buf->data_len = pkt_len;
            tx_buf->pkt_len = pkt_len;
            rte_eth_tx_burst(rx_port, 0, &tx_buf, 1);
        }
    }
}

int main(int argc, char *argv[]) {
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) rte_exit(EXIT_FAILURE, "EAL init failed\n");

    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * rte_lcore_count(),
                                        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (!mbuf_pool) rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    init_port(rx_port);

    int client_sock = init_socket();
    if (client_sock < 0) rte_exit(EXIT_FAILURE, "Socket init failed\n");

    dpdk_loop(client_sock);
    close(client_sock);
    return 0;
}
