package cmd

import (
	"fmt"

	"github.com/spf13/cobra"
)

var helpCmd = &cobra.Command{
	Use:   "help",
	Short: "Show help and usage examples",
	Long: `VM Orchestrator - DPDK Traffic Testing Tool

Quick VM management and traffic testing from KVM host.`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("VM Orchestrator - Help")
		fmt.Println("======================")
		fmt.Println()
		
		fmt.Println("Basic Commands:")
		fmt.Println("---------------")
		fmt.Println("start <vm>          - Power on a VM")
		fmt.Println("stop <vm>           - Force shutdown a VM")
		fmt.Println("topology            - List all VMs and status")
		fmt.Println("ips <vm>            - Get VM IP address")
		fmt.Println()
		
		fmt.Println("Traffic Testing:")
		fmt.Println("----------------")
		fmt.Println("run-traffic --rx-vm <rx> --tx-vm <tx> --protocol <proto> --mode <mode>")
		fmt.Println()
		fmt.Println("Protocols: icmp, udp, tcp, dns, arp, eth, random")
		fmt.Println("Modes: continuous, burst, rate-limited, exponential-backoff, random")
		fmt.Println()
		fmt.Println("Examples:")
		fmt.Println("  ./vm-orchestrator run-traffic --rx-vm vm1 --tx-vm vm2 --protocol icmp --mode continuous")
		fmt.Println("  ./vm-orchestrator run-traffic --rx-vm vm1 --tx-vm vm2 --protocol udp --mode burst --message \"hello\"")
		fmt.Println("  ./vm-orchestrator run-traffic --rx-vm vm1 --tx-vm vm2 --protocol dns --mode continuous --domain google.com")
		fmt.Println("  ./vm-orchestrator run-traffic --rx-vm vm1 --tx-vm vm2 --protocol random --mode random")
		fmt.Println()
		
		fmt.Println("How it works:")
		fmt.Println("-------------")
		fmt.Println("1. Starts tcpdump on br-test interface")
		fmt.Println("2. Launches RX VM in background")
		fmt.Println("3. Starts TX VM and shows real-time stats")
		fmt.Println("4. Displays aggregated counters from both VMs")
		fmt.Println("5. Ctrl+C stops everything and saves PCAP")
		fmt.Println()
		fmt.Println("Random mode:")
		fmt.Println("- Use 'random' for protocol or mode to auto-generate")
		fmt.Println("- Random messages for UDP/TCP")
		fmt.Println("- Random domains for DNS")
		fmt.Println()
		
		fmt.Println("Typical workflow:")
		fmt.Println("-----------------")
		fmt.Println("  ./vm-orchestrator start vm1")
		fmt.Println("  ./vm-orchestrator start vm2")
		fmt.Println("  ./vm-orchestrator topology")
		fmt.Println("  ./vm-orchestrator run-traffic --rx-vm vm1 --tx-vm vm2 --protocol icmp --mode burst")
		fmt.Println("  ./vm-orchestrator stop vm1")
		fmt.Println("  ./vm-orchestrator stop vm2")
		fmt.Println()
		
		fmt.Println("Requirements:")
		fmt.Println("-------------")
		fmt.Println("- VMs with SSH root access (password: root)")
		fmt.Println("- DPDK traffic engine at /home/test/TestVM/DPDK/dpdk-app/")
		fmt.Println("- tcpdump available on host")
		fmt.Println("- br-test bridge interface")
		fmt.Println()
		
		fmt.Println("Output:")
		fmt.Println("-------")
		fmt.Println("- Real-time stats table (refreshes every 2s)")
		fmt.Println("- PCAP file: traffic_<proto>_<mode>_<timestamp>.pcap")
		fmt.Println("- Protocol-specific counters (packets, connections, etc.)")
	},
} 