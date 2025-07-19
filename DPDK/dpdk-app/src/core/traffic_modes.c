#include <rte_cycles.h>
#include <rte_lcore.h>
#include <stdlib.h>
#include <time.h>
#include "../include/core/traffic_modes.h"
#include "../include/core/log.h"

void init_traffic_config(traffic_config_t *config, traffic_mode_t mode) {
    config->mode = mode;
    config->retry_count = 0;
    config->last_packet_time = 0;
    
    switch (mode) {
        case TRAFFIC_MODE_CONTINUOUS:
            config->interval_us = 1000000;
            LOG_INFO("TRAFFIC: Initialized Continuous mode - interval: %u us (1 second)", config->interval_us);
            break;
            
        case TRAFFIC_MODE_BURST:
            config->burst_size = 10;
            config->burst_interval_us = 5000000;
            config->interval_us = 10000;
            LOG_INFO("TRAFFIC: Initialized Burst mode - burst size: %d, burst interval: %u us, packet interval: %u us", 
                    config->burst_size, config->burst_interval_us, config->interval_us);
            break;
            
        case TRAFFIC_MODE_RATE_LIMITED:
            config->packets_per_second = 1000;
            config->interval_us = 1000000 / config->packets_per_second;
            LOG_INFO("TRAFFIC: Initialized Rate-limited mode - %u packets/sec, interval: %u us", 
                    config->packets_per_second, config->interval_us);
            break;
            

            
        case TRAFFIC_MODE_EXPONENTIAL_BACKOFF:
            config->base_delay_us = 100000;
            config->max_retries = 5;
            config->interval_us = config->base_delay_us;
            LOG_INFO("TRAFFIC: Initialized Exponential Backoff mode - base delay: %u us, max retries: %d", 
                    config->base_delay_us, config->max_retries);
            break;
    }
}

void reset_traffic_config(traffic_config_t *config) {
    config->retry_count = 0;
    config->last_packet_time = 0;
    
    if (config->mode == TRAFFIC_MODE_EXPONENTIAL_BACKOFF) {
        config->interval_us = config->base_delay_us;
    }
}

uint32_t get_burst_size(const traffic_config_t *config) {
    if (config->mode == TRAFFIC_MODE_BURST) {
        return config->burst_size;
    }
    return 1;
}

void apply_traffic_delay(traffic_config_t *config) {
    uint64_t current_time = rte_get_tsc_cycles();
    uint64_t cycles_per_us = rte_get_tsc_hz() / 1000000;
    
    switch (config->mode) {
        case TRAFFIC_MODE_CONTINUOUS:
            LOG_DEBUG("TRAFFIC: Continuous mode - waiting %u us", config->interval_us);
            rte_delay_us(config->interval_us);
            break;
            
        case TRAFFIC_MODE_BURST:
            config->retry_count++;
            if (config->retry_count % config->burst_size == 0 && config->retry_count > 0) {
                LOG_INFO("TRAFFIC: Burst mode - sent %d packets, now waiting %u us", 
                        config->burst_size, config->burst_interval_us);
                rte_delay_us(config->burst_interval_us);
            } else {
                LOG_DEBUG("TRAFFIC: Burst mode - packet %d/%d, waiting %u us", 
                         config->retry_count % config->burst_size, config->burst_size, config->interval_us);
                rte_delay_us(config->interval_us);
            }
            break;
            
        case TRAFFIC_MODE_RATE_LIMITED:
            if (config->last_packet_time > 0) {
                uint64_t elapsed = (current_time - config->last_packet_time) / cycles_per_us;
                if (elapsed < config->interval_us) {
                    uint32_t wait_time = config->interval_us - elapsed;
                    LOG_DEBUG("TRAFFIC: Rate-limited mode - elapsed %lu us, waiting %u us to maintain %u pps", 
                             elapsed, wait_time, config->packets_per_second);
                    rte_delay_us(wait_time);
                } else {
                    LOG_DEBUG("TRAFFIC: Rate-limited mode - no delay needed, elapsed %lu us", elapsed);
                }
            } else {
                LOG_DEBUG("TRAFFIC: Rate-limited mode - first packet, no delay");
            }
            break;
            

            
        case TRAFFIC_MODE_EXPONENTIAL_BACKOFF:
            if (config->retry_count < config->max_retries) {
                uint32_t delay = config->base_delay_us * (1 << config->retry_count);
                LOG_INFO("TRAFFIC: Exponential backoff - retry %d/%d, waiting %u us (base * 2^%d)", 
                         config->retry_count + 1, config->max_retries, delay, config->retry_count);
                rte_delay_us(delay);
                config->retry_count++;
            } else {
                LOG_INFO("TRAFFIC: Exponential backoff - max retries reached, resetting to base delay %u us", 
                         config->base_delay_us);
                rte_delay_us(config->base_delay_us);
                config->retry_count = 0;
            }
            break;
    }
    
    config->last_packet_time = current_time;
} 