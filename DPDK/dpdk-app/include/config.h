#ifndef CONFIG_H
#define CONFIG_H

#include <rte_ether.h>
#include <stdint.h>

// Network configuration (hardcoded as requested)
struct network_config {
    // MAC addresses
    struct rte_ether_addr src_mac;
    struct rte_ether_addr dst_mac;
    
    // IP addresses
    uint32_t src_ip;
    uint32_t dst_ip;
    
    // Port numbers
    uint16_t udp_src_port;
    uint16_t udp_dst_port;
    uint16_t tcp_src_port;
    uint16_t tcp_dst_port;
    
    // ICMP configuration
    uint16_t icmp_ident;
    uint16_t icmp_seq;
    
    // Timing
    uint32_t tx_interval_ms;
    uint32_t rx_timeout_ms;
};

// Global configuration instance
extern const struct network_config g_config;

// Configuration initialization
void init_network_config(void);

// Helper macros for easy access
#define CONFIG_SRC_MAC g_config.src_mac
#define CONFIG_DST_MAC g_config.dst_mac
#define CONFIG_SRC_IP g_config.src_ip
#define CONFIG_DST_IP g_config.dst_ip

#endif // CONFIG_H 