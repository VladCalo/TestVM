#!/bin/bash

set -euo pipefail

readonly INTERFACE="eth1"
readonly ENV_FILE="/etc/dpdk/interface.env"
readonly LOG_FILE="/var/log/dpdk-bind.log"
readonly DPDK_BIND_SCRIPT="/usr/local/bin/dpdk-devbind.py"
readonly CONFIG_DIR="/etc/dpdk"

log() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "${timestamp} [${level}] ${message}" | tee -a "$LOG_FILE"
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        echo "This script must be run as root" >&2
        exit 1
    fi
}

check_interface() {
    if [[ ! -d "/sys/class/net/$INTERFACE" ]]; then
        log "ERROR" "Interface $INTERFACE does not exist"
        return 1
    fi
    log "INFO" "Interface $INTERFACE found"
}

get_mac_address() {
    local mac
    mac=$(cat "/sys/class/net/$INTERFACE/address" 2>/dev/null)
    if [[ -z "$mac" ]]; then
        log "ERROR" "Could not read MAC address from $INTERFACE"
        return 1
    fi
    echo "$mac"
}

get_pci_bus_info() {
    local bus_info
    if ! command -v ethtool >/dev/null 2>&1; then
        log "ERROR" "ethtool command not found"
        return 1
    fi
    
    bus_info=$(ethtool -i "$INTERFACE" 2>/dev/null | grep "bus-info:" | cut -d' ' -f2)
    if [[ -z "$bus_info" ]]; then
        log "ERROR" "Could not extract bus-info from $INTERFACE"
        return 1
    fi
    echo "$bus_info"
}

check_dpdk_tools() {
    if [[ ! -f "$DPDK_BIND_SCRIPT" ]]; then
        log "ERROR" "DPDK bind script not found at $DPDK_BIND_SCRIPT"
        return 1
    fi
    log "INFO" "DPDK bind script found"
}

bind_interface() {
    local bus_info="$1"
    
    log "INFO" "Unbinding interface from current driver"
    "$DPDK_BIND_SCRIPT" --unbind "$bus_info" >/dev/null 2>&1 || true
    
    log "INFO" "Binding interface to uio_pci_generic driver"
    "$DPDK_BIND_SCRIPT" --bind=uio_pci_generic "$bus_info"
    
    log "INFO" "Interface binding completed"
}

create_config_dir() {
    if [[ ! -d "$CONFIG_DIR" ]]; then
        log "INFO" "Creating configuration directory: $CONFIG_DIR"
        mkdir -p "$CONFIG_DIR"
    fi
}

save_environment() {
    local mac="$1"
    local bus_info="$2"
    
    log "INFO" "Saving environment variables to $ENV_FILE"
    
    cat > "$ENV_FILE" << EOF
DPDK_INTERFACE=$INTERFACE
DPDK_PCI_BUS=$bus_info
DPDK_MAC_ADDRESS=$mac
DPDK_BIND_DRIVER=uio_pci_generic
DPDK_BIND_TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
EOF
    
    chmod 644 "$ENV_FILE"
    log "INFO" "Environment variables saved successfully"
}

display_status() {
    local mac="$1"
    local bus_info="$2"
    
    echo
    echo "=== DPDK Interface Binding Status ==="
    echo "Interface:     $INTERFACE"
    echo "MAC Address:   $mac"
    echo "PCI Bus Info:  $bus_info"
    echo "Config File:   $ENV_FILE"
    echo "Log File:      $LOG_FILE"
    echo
}

main() {
    check_root
    check_interface || exit 1
    check_dpdk_tools || exit 1
    
    log "INFO" "Starting DPDK interface binding process"
    
    local mac bus_info
    mac=$(get_mac_address)
    bus_info=$(get_pci_bus_info)
    
    log "INFO" "Interface: $INTERFACE, MAC: $mac, PCI: $bus_info"
    
    bind_interface "$bus_info"
    create_config_dir
    save_environment "$mac" "$bus_info"
    display_status "$mac" "$bus_info"
    
    log "INFO" "DPDK interface binding process completed successfully"
}

main "$@" 