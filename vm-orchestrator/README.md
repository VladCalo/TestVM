# VM Orchestrator CLI

Advanced CLI for controlling VMs and DPDK traffic testing with aggregated statistics.

## Build
```bash
cd vm-orchestrator
go build -o vm-orchestrator
```

## Usage

### VM Control
```bash
# Start/stop VMs
./vm-orchestrator start <vm-name>
./vm-orchestrator stop <vm-name>

# Show topology
./vm-orchestrator topology

# Show VM IPs
./vm-orchestrator ips <vm-name>
```

### Traffic Testing (RECOMMENDED)
```bash
# Run traffic test with aggregated stats
./vm-orchestrator run-traffic --rx-vm <rx-vm> --tx-vm <tx-vm> --protocol icmp --mode continuous
./vm-orchestrator run-traffic --rx-vm <rx-vm> --tx-vm <tx-vm> --protocol udp --mode burst --message "test"
./vm-orchestrator run-traffic --rx-vm <rx-vm> --tx-vm <tx-vm> --protocol tcp --mode rate-limited --message "TCP test"
./vm-orchestrator run-traffic --rx-vm <rx-vm> --tx-vm <tx-vm> --protocol dns --mode continuous --domain example.com
./vm-orchestrator run-traffic --rx-vm <rx-vm> --tx-vm <tx-vm> --protocol arp --mode burst
```

### Help
```bash
# Get comprehensive help
./vm-orchestrator help

# Get help for specific commands
./vm-orchestrator --help
./vm-orchestrator run-traffic --help
```

## Features

### 🚀 **Single Terminal Experience**
- RX runs in background
- TX shows aggregated statistics
- Clean table format display
- No need for multiple tabs

### 📊 **Real-time Statistics**
- Packets sent/received counters
- Protocol-specific metrics
- Traffic mode events
- Burst cycle tracking

### 🎯 **Smart Control**
- Ctrl+C stops both RX and TX
- Automatic cleanup
- Graceful shutdown

### 📁 **Automatic Capture**
- tcpdump on br-test interface
- Timestamped PCAP files
- Background capture

## How It Works

1. **VM Control**: Uses `virsh` commands to manage VMs
2. **SSH Connection**: Connects to VMs via SSH using root credentials
3. **Background RX**: Starts RX VM in background
4. **Foreground TX**: Starts TX VM with stats display
5. **Aggregated Stats**: Shows combined statistics from both VMs
6. **Graceful Shutdown**: Handles Ctrl+C to stop both VMs cleanly

## Prerequisites

- KVM hypervisor with libvirt
- VMs with SSH access (root user with password "root")
- DPDK traffic engine installed on VMs at `/home/test/TestVM/DPDK/dpdk-app/`
- tcpdump available on KVM host

## VM SSH Setup

To enable root SSH access on VMs:
```bash
# On each VM, run:
sudo sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config
sudo systemctl restart sshd
echo "root:root" | sudo chpasswd
```

## Example Output

```
=== TRAFFIC STATISTICS ===
Protocol: icmp | Mode: burst
RX VM: testVM1 | TX VM: testVM2
==========================

📥 RX STATISTICS:
  • Packets Received: 45
  • Replies Sent: 45
  • ICMP Replies: 45

📤 TX STATISTICS:
  • Packets Sent: 45
  • ICMP Requests: 45
  • Burst Cycles: 4
  • Traffic Events: 4

Press Ctrl+C to stop...
``` 