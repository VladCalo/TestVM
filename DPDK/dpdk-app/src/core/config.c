#include "../../include/core/config.h"
#include <rte_byteorder.h>

// Global configuration instance with hardcoded values
const struct network_config g_config = {
    // MAC addresses (hardcoded as requested)
    .src_mac = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}},
    .dst_mac = {{0x52, 0x54, 0x00, 0xa2, 0x6c, 0xb6}},
    
    // IP addresses (hardcoded as requested)
    .src_ip = 0x0a000001,  // 10.0.0.1
    .dst_ip = 0x0a000002,  // 10.0.0.2
    
    // Port numbers (hardcoded as requested)
    .udp_src_port = 12345,
    .udp_dst_port = 23456,
    .tcp_src_port = 11111,
    .tcp_dst_port = 22222,
    
    // ICMP configuration (hardcoded as requested)
    .icmp_ident = 0x1234,
    .icmp_seq = 1,
    
    // Timing configuration
    .tx_interval_ms = 1000,
    .rx_timeout_ms = 100
};

void init_network_config(void) {
    // Configuration is statically initialized, no runtime setup needed
    // This function exists for future extensibility
} 