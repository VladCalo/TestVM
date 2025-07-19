#ifndef TRAFFIC_MODES_H
#define TRAFFIC_MODES_H

#include <stdint.h>
#include <rte_mempool.h>

typedef enum {
    TRAFFIC_MODE_CONTINUOUS,
    TRAFFIC_MODE_BURST,
    TRAFFIC_MODE_RATE_LIMITED,
    TRAFFIC_MODE_EXPONENTIAL_BACKOFF
} traffic_mode_t;

typedef struct {
    traffic_mode_t mode;
    uint32_t interval_us;
    uint32_t burst_size;
    uint32_t burst_interval_us;
    uint32_t packets_per_second;
    uint32_t max_retries;
    uint32_t base_delay_us;
    uint32_t retry_count;
    uint64_t last_packet_time;
} traffic_config_t;

void init_traffic_config(traffic_config_t *config, traffic_mode_t mode);
void apply_traffic_delay(traffic_config_t *config);
uint32_t get_burst_size(const traffic_config_t *config);
void reset_traffic_config(traffic_config_t *config);

#endif 