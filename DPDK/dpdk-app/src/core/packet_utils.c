#include "../../include/core/packet_utils.h"
#include "../../include/core/common.h"
#include <rte_ethdev.h>
#include <stdio.h>

struct parsed_packet parse_ipv4_packet(struct rte_mbuf *mbuf) {
    struct parsed_packet parsed = {0};
    
    if (rte_pktmbuf_data_len(mbuf) < sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr)) {
        parsed.valid = PACKET_TOO_SHORT;
        return parsed;
    }
    
    parsed.eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    
    if (parsed.eth->ether_type != rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {
        parsed.valid = PACKET_INVALID_ETH_TYPE;
        return parsed;
    }
    
    parsed.ip = (struct rte_ipv4_hdr *)(parsed.eth + 1);
    parsed.payload = (char *)(parsed.ip + 1);
    parsed.payload_len = rte_pktmbuf_data_len(mbuf) - 
                        sizeof(struct rte_ether_hdr) - 
                        sizeof(struct rte_ipv4_hdr);
    parsed.valid = PACKET_VALID;
    
    return parsed;
}

int validate_ipv4_packet(struct rte_mbuf *mbuf, uint8_t expected_proto) {
    struct parsed_packet parsed = parse_ipv4_packet(mbuf);
    
    if (parsed.valid != PACKET_VALID) {
        return 0;
    }
    
    if (parsed.ip->next_proto_id != expected_proto) {
        return 0;
    }
    
    return 1;
}

void free_packet_burst(struct rte_mbuf **bufs, uint16_t nb_pkts) {
    for (int i = 0; i < nb_pkts; i++) {
        if (bufs[i]) {
            rte_pktmbuf_free(bufs[i]);
        }
    }
}

int process_packet_burst(uint16_t port_id, uint16_t queue_id, 
                        packet_handler_t handler, void *user_data) {
    struct rte_mbuf *bufs[BURST_SIZE];
    uint16_t nb_rx = rte_eth_rx_burst(port_id, queue_id, bufs, BURST_SIZE);
    
    int processed = 0;
    for (int i = 0; i < nb_rx; i++) {
        if (handler(bufs[i], user_data) == 0) {
            processed++;
        }
    }
    
    return processed;
} 