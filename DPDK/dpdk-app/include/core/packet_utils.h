#ifndef PACKET_UTILS_H
#define PACKET_UTILS_H

#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <stdint.h>

// Packet validation results
enum packet_validity {
    PACKET_VALID,
    PACKET_INVALID_ETH_TYPE,
    PACKET_INVALID_IP_PROTO,
    PACKET_TOO_SHORT
};

// Packet parsing structure
struct parsed_packet {
    struct rte_ether_hdr *eth;
    struct rte_ipv4_hdr *ip;
    void *payload;
    size_t payload_len;
    enum packet_validity valid;
};

// Utility functions for packet processing
struct parsed_packet parse_ipv4_packet(struct rte_mbuf *mbuf);
int validate_ipv4_packet(struct rte_mbuf *mbuf, uint8_t expected_proto);
void free_packet_burst(struct rte_mbuf **bufs, uint16_t nb_pkts);

// Common packet processing loop
typedef int (*packet_handler_t)(struct rte_mbuf *mbuf, void *user_data);

int process_packet_burst(uint16_t port_id, uint16_t queue_id, 
                        packet_handler_t handler, void *user_data);

#endif // PACKET_UTILS_H 