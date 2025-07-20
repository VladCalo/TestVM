#!/bin/bash

set -euo pipefail

readonly INTERFACE="eth1"
readonly ENV_FILE="/etc/dpdk/interface.env"
readonly LOG_FILE="/var/log/dpdk-bind.log"
readonly DPDK_BIND_SCRIPT="/usr/local/bin/dpdk-devbind.py"
readonly SCRIPT_NAME="dpdk-manage"

print_status() {
    local level="$1"
    local message="$2"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[${timestamp}] [${level}] ${message}"
}

show_usage() {
    cat << EOF
DPDK Interface Management Utility

Usage: $SCRIPT_NAME [COMMAND] [OPTIONS]

Commands:
  status     - Show current interface status and DPDK binding
  bind       - Manually bind interface to DPDK
  unbind     - Unbind interface from DPDK (rebind to virtio_net)
  info       - Show detailed interface information
  env        - Show saved environment variables
  restart    - Restart the DPDK binding service
  logs       - Show DPDK binding service logs
  test       - Test DPDK interface functionality
  help       - Show this help message

Examples:
  $SCRIPT_NAME status
  $SCRIPT_NAME bind
  $SCRIPT_NAME logs
  $SCRIPT_NAME test

EOF
}

check_interface() {
    if [[ ! -d "/sys/class/net/$INTERFACE" ]]; then
        print_status "ERROR" "Interface $INTERFACE does not exist"
        return 1
    fi
    return 0
}

get_pci_bus_info() {
    local bus_info
    if ! command -v ethtool >/dev/null 2>&1; then
        print_status "ERROR" "ethtool command not found"
        return 1
    fi
    
    bus_info=$(ethtool -i "$INTERFACE" 2>/dev/null | grep "bus-info:" | cut -d' ' -f2)
    if [[ -z "$bus_info" ]]; then
        print_status "ERROR" "Could not extract bus-info from $INTERFACE"
        return 1
    fi
    echo "$bus_info"
}

get_mac_address() {
    local mac
    mac=$(cat "/sys/class/net/$INTERFACE/address" 2>/dev/null)
    if [[ -z "$mac" ]]; then
        print_status "ERROR" "Could not read MAC address from $INTERFACE"
        return 1
    fi
    echo "$mac"
}

get_dpdk_status() {
    local bus_info="$1"
    if [[ ! -f "$DPDK_BIND_SCRIPT" ]]; then
        echo "DPDK tools not available"
        return 1
    fi
    
    "$DPDK_BIND_SCRIPT" --status 2>/dev/null | grep "$bus_info" || echo "Not found in DPDK binding list"
}

show_status() {
    print_status "INFO" "=== DPDK Interface Status ==="
    
    if ! check_interface; then
        return 1
    fi
    
    local mac bus_info dpdk_status
    
    mac=$(get_mac_address)
    bus_info=$(get_pci_bus_info)
    dpdk_status=$(get_dpdk_status "$bus_info")
    
    echo
    echo "Interface Information:"
    echo "  Name:         $INTERFACE"
    echo "  MAC Address:  $mac"
    echo "  PCI Bus Info: $bus_info"
    echo "  State:        $(cat "/sys/class/net/$INTERFACE/operstate" 2>/dev/null || echo "Unknown")"
    echo
    
    echo "DPDK Binding Status:"
    echo "  Status:       $dpdk_status"
    echo
    
    if [[ -f "$ENV_FILE" ]]; then
        echo "Environment Configuration:"
        cat "$ENV_FILE" | sed 's/^/  /'
    else
        print_status "WARN" "No environment file found at $ENV_FILE"
    fi
    
    echo
}

bind_interface() {
    print_status "INFO" "Manually binding interface $INTERFACE to DPDK..."
    
    if ! check_interface; then
        return 1
    fi
    
    if [[ ! -f "/usr/local/bin/dpdk-bind.sh" ]]; then
        print_status "ERROR" "DPDK bind script not found at /usr/local/bin/dpdk-bind.sh"
        return 1
    fi
    
    if /usr/local/bin/dpdk-bind.sh; then
        print_status "SUCCESS" "Interface binding completed successfully"
    else
        print_status "ERROR" "Interface binding failed"
        return 1
    fi
}

unbind_interface() {
    print_status "WARN" "Unbinding interface $INTERFACE from DPDK..."
    
    if ! check_interface; then
        return 1
    fi
    
    local bus_info
    bus_info=$(get_pci_bus_info)
    
    if [[ ! -f "$DPDK_BIND_SCRIPT" ]]; then
        print_status "ERROR" "DPDK bind script not found"
        return 1
    fi
    
    print_status "INFO" "Unbinding from DPDK..."
    if ! "$DPDK_BIND_SCRIPT" --unbind "$bus_info" >/dev/null 2>&1; then
        print_status "ERROR" "Failed to unbind from DPDK"
        return 1
    fi
    
    print_status "INFO" "Rebinding to virtio_net driver..."
    if ! "$DPDK_BIND_SCRIPT" --bind=virtio_net "$bus_info" >/dev/null 2>&1; then
        print_status "ERROR" "Failed to rebind to virtio_net"
        return 1
    fi
    
    sleep 2
    
    print_status "SUCCESS" "Interface $INTERFACE unbound from DPDK and bound to virtio_net"
}

show_info() {
    print_status "INFO" "=== Interface Information ==="
    
    if ! check_interface; then
        return 1
    fi
    
    echo
    echo "Basic Interface Info:"
    echo "===================="
    ip link show "$INTERFACE"
    echo
    
    echo "Driver Information:"
    echo "=================="
    ethtool -i "$INTERFACE" 2>/dev/null || echo "ethtool not available"
    echo
    
    echo "Interface Statistics:"
    echo "===================="
    ethtool -S "$INTERFACE" 2>/dev/null | head -20 || echo "Statistics not available"
    echo
    
    echo "PCI Device Information:"
    echo "======================"
    local bus_info
    bus_info=$(get_pci_bus_info)
    if command -v lspci >/dev/null 2>&1; then
        lspci -vvv | grep -A 10 -B 2 "$bus_info" || echo "PCI info not available"
    else
        echo "lspci command not available"
    fi
    echo
}

show_env() {
    if [[ -f "$ENV_FILE" ]]; then
        print_status "INFO" "=== Environment Variables ==="
        echo
        cat "$ENV_FILE"
        echo
    else
        print_status "WARN" "No environment file found at $ENV_FILE"
        print_status "WARN" "Run the binding service first: sudo systemctl start dpdk-bind.service"
    fi
}

restart_service() {
    print_status "INFO" "Restarting DPDK binding service..."
    
    if systemctl restart dpdk-bind.service; then
        print_status "SUCCESS" "Service restarted successfully"
        echo
        echo "Service status:"
        systemctl status dpdk-bind.service --no-pager -l
    else
        print_status "ERROR" "Failed to restart service"
        return 1
    fi
}

show_logs() {
    print_status "INFO" "=== DPDK Binding Service Logs ==="
    echo
    
    if systemctl is-active dpdk-bind.service >/dev/null 2>&1; then
        journalctl -u dpdk-bind.service -f --no-pager
    else
        print_status "WARN" "DPDK binding service is not active"
        echo "Recent logs:"
        journalctl -u dpdk-bind.service --no-pager -n 20
    fi
}

test_interface() {
    print_status "INFO" "=== Testing DPDK Interface ==="
    
    if ! check_interface; then
        return 1
    fi
    
    local bus_info
    bus_info=$(get_pci_bus_info)
    
    echo
    echo "1. Checking interface existence:"
    if [[ -d "/sys/class/net/$INTERFACE" ]]; then
        print_status "PASS" "Interface $INTERFACE exists"
    else
        print_status "FAIL" "Interface $INTERFACE does not exist"
        return 1
    fi
    
    echo
    echo "2. Checking MAC address:"
    local mac
    mac=$(get_mac_address)
    if [[ -n "$mac" ]]; then
        print_status "PASS" "MAC address: $mac"
    else
        print_status "FAIL" "Could not read MAC address"
        return 1
    fi
    
    echo
    echo "3. Checking PCI bus info:"
    if [[ -n "$bus_info" ]]; then
        print_status "PASS" "PCI bus info: $bus_info"
    else
        print_status "FAIL" "Could not get PCI bus info"
        return 1
    fi
    
    echo
    echo "4. Checking DPDK tools:"
    if [[ -f "$DPDK_BIND_SCRIPT" ]]; then
        print_status "PASS" "DPDK bind script available"
    else
        print_status "FAIL" "DPDK bind script not found"
        return 1
    fi
    
    echo
    echo "5. Checking DPDK binding:"
    local dpdk_status
    dpdk_status=$(get_dpdk_status "$bus_info")
    if echo "$dpdk_status" | grep -q "uio_pci_generic"; then
        print_status "PASS" "Interface is bound to DPDK"
    else
        print_status "WARN" "Interface is not bound to DPDK: $dpdk_status"
    fi
    
    echo
    echo "6. Checking environment file:"
    if [[ -f "$ENV_FILE" ]]; then
        print_status "PASS" "Environment file exists"
    else
        print_status "WARN" "Environment file not found"
    fi
    
    echo
    print_status "SUCCESS" "DPDK interface test completed"
}

main() {
    case "${1:-}" in
        "status")
            show_status
            ;;
        "bind")
            bind_interface
            ;;
        "unbind")
            unbind_interface
            ;;
        "info")
            show_info
            ;;
        "env")
            show_env
            ;;
        "restart")
            restart_service
            ;;
        "logs")
            show_logs
            ;;
        "test")
            test_interface
            ;;
        "help"|"-h"|"--help")
            show_usage
            ;;
        "")
            show_usage
            exit 1
            ;;
        *)
            print_status "ERROR" "Unknown command: $1"
            echo
            show_usage
            exit 1
            ;;
    esac
}

main "$@" 