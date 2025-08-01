package cmd

import (
	"bufio"
	"fmt"
	"math/rand"
	"os"
	"os/exec"
	"os/signal"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/spf13/cobra"
	"golang.org/x/crypto/ssh"
)

var (
	rxVM, txVM, protocol, mode, domain, message string
	rxClient, txClient                          *ssh.Client
	rxSession, txSession                        *ssh.Session
	stopChan                                    chan bool
	rxStats, txStats                            map[string]int
	statsMutex                                  sync.RWMutex
	tcpdumpCmd                                  *exec.Cmd
	tcpdumpFile                                 string
)

var (
	protocols = []string{"icmp", "udp", "tcp", "dns", "arp", "eth"}
	modes     = []string{"continuous", "burst", "rate-limited", "exponential-backoff"}
	domains   = []string{"google.com", "github.com", "stackoverflow.com", "reddit.com", "wikipedia.org", "amazon.com", "netflix.com", "spotify.com", "discord.com", "slack.com"}
	messages  = []string{"hello", "test", "ping", "data", "message", "packet", "traffic", "network", "random", "stress"}
)

var runTrafficCmd = &cobra.Command{
	Use:   "run-traffic",
	Short: "Run traffic test with aggregated stats",
	Long: `Run traffic test with RX in background and aggregated stats display.
This command will:
1. Start RX VM in background
2. Start TX VM and display aggregated stats
3. Show real-time statistics in a clean format
4. Handle Ctrl+C to stop both VMs

Press Ctrl+C to stop traffic and exit.`,
	Example: `  vm-orchestrator run-traffic --rx-vm <rx-vm> --tx-vm <tx-vm> --protocol icmp --mode continuous
  vm-orchestrator run-traffic --rx-vm <rx-vm> --tx-vm <tx-vm> --protocol udp --mode burst --message "test"`,
	Run: func(cmd *cobra.Command, args []string) {
		if rxVM == "" || txVM == "" {
			fmt.Println("Error: RX VM and TX VM required")
			return
		}

		// Initialize random seed
		rand.Seed(time.Now().UnixNano())

		// If random mode is selected, generate random parameters
		if protocol == "random" || mode == "random" {
			if protocol == "random" {
				protocol = protocols[rand.Intn(len(protocols))]
			}
			if mode == "random" {
				mode = modes[rand.Intn(len(modes))]
			}
			
			// Generate random message for UDP/TCP if not provided
			if (protocol == "udp" || protocol == "tcp") && message == "" {
				message = messages[rand.Intn(len(messages))]
			}
			
			// Generate random domain for DNS if not provided
			if protocol == "dns" && domain == "" {
				domain = domains[rand.Intn(len(domains))]
			}
		}

		if protocol == "" || mode == "" {
			fmt.Println("Error: Protocol and mode required (or use 'random' for either)")
			return
		}

		rxStats = make(map[string]int)
		txStats = make(map[string]int)
		stopChan = make(chan bool, 1)

		sigChan := make(chan os.Signal, 1)
		signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

		fmt.Println("=== Starting Traffic Test ===")
		fmt.Printf("RX VM: %s, TX VM: %s\n", rxVM, txVM)
		fmt.Printf("Protocol: %s, Mode: %s\n", protocol, mode)
		if domain != "" {
			fmt.Printf("Domain: %s\n", domain)
		}
		if message != "" {
			fmt.Printf("Message: %s\n", message)
		}
		fmt.Println()

		fmt.Println("1. Starting tcpdump capture...")
		startTcpdump()

		time.Sleep(1 * time.Second)

		fmt.Println("2. Starting RX VM in background...")
		go startRXBackground()

		time.Sleep(3 * time.Second)

		fmt.Println("3. Starting TX VM with aggregated stats...")
		go startTXWithStats()

		time.Sleep(2 * time.Second)

		fmt.Println()
		fmt.Println("=== Traffic Test Running ===")
		fmt.Println("Press Ctrl+C to stop...")
		fmt.Println()

		go displayStats()

		select {
		case <-sigChan:
			fmt.Println("\n=== Stopping Traffic Test ===")
			stopTraffic()
		case <-stopChan:
			fmt.Println("\n=== Traffic Test Stopped ===")
		}
	},
}

func startRXBackground() {
	ips, _ := exec.Command("virsh", "domifaddr", "--source", "agent", rxVM).Output()
	ip := extractIP(string(ips))
	if ip == "" {
		fmt.Printf("Error: Could not get IP for RX VM %s\n", rxVM)
		return
	}

	config := createSSHConfig()

	client, err := ssh.Dial("tcp", ip+":22", config)
	if err != nil {
		fmt.Printf("Error connecting to RX VM: %v\n", err)
		return
	}
	rxClient = client

	session, err := client.NewSession()
	if err != nil {
		fmt.Printf("Error creating RX session: %v\n", err)
		return
	}
	rxSession = session

	cmdStr := fmt.Sprintf("cd /home/test/TestVM/DPDK/dpdk-app && ./traffic_engine rx %s -l 0 -n 4 -a 0000:02:00.0", protocol)

	stdout, err := session.StdoutPipe()
	if err != nil {
		fmt.Printf("Error getting RX stdout: %v\n", err)
		return
	}
	stderr, err := session.StderrPipe()
	if err != nil {
		fmt.Printf("Error getting RX stderr: %v\n", err)
		return
	}

	err = session.Start(cmdStr)
	if err != nil {
		fmt.Printf("Error starting RX command: %v\n", err)
		return
	}

	go func() {
		scanner := bufio.NewScanner(stdout)
		for scanner.Scan() {
			line := scanner.Text()
			updateRXStats(line)
		}
	}()

	go func() {
		scanner := bufio.NewScanner(stderr)
		for scanner.Scan() {
			line := scanner.Text()
			updateRXStats(line)
		}
	}()

	fmt.Printf("✓ RX started in background on %s (%s)\n", rxVM, ip)
}

func startTXWithStats() {
	ips, _ := exec.Command("virsh", "domifaddr", "--source", "agent", txVM).Output()
	ip := extractIP(string(ips))
	if ip == "" {
		fmt.Printf("Error: Could not get IP for TX VM %s\n", txVM)
		return
	}

	config := createSSHConfig()

	client, err := ssh.Dial("tcp", ip+":22", config)
	if err != nil {
		fmt.Printf("Error connecting to TX VM: %v\n", err)
		return
	}
	txClient = client

	session, err := client.NewSession()
	if err != nil {
		fmt.Printf("Error creating TX session: %v\n", err)
		return
	}
	txSession = session

	cmdStr := fmt.Sprintf("cd /home/test/TestVM/DPDK/dpdk-app && ./traffic_engine tx %s %s", protocol, mode)
	
	if protocol == "dns" && domain != "" {
		cmdStr += " " + domain
	}
	
	if (protocol == "udp" || protocol == "tcp") && message != "" {
		cmdStr += " \"" + message + "\""
	}
	
	cmdStr += " -l 0 -n 4 -a 0000:02:00.0"

	stdout, err := session.StdoutPipe()
	if err != nil {
		fmt.Printf("Error getting TX stdout: %v\n", err)
		return
	}
	stderr, err := session.StderrPipe()
	if err != nil {
		fmt.Printf("Error getting TX stderr: %v\n", err)
		return
	}

	err = session.Start(cmdStr)
	if err != nil {
		fmt.Printf("Error starting TX command: %v\n", err)
		return
	}

	go func() {
		scanner := bufio.NewScanner(stdout)
		for scanner.Scan() {
			line := scanner.Text()
			updateTXStats(line)
		}
	}()

	go func() {
		scanner := bufio.NewScanner(stderr)
		for scanner.Scan() {
			line := scanner.Text()
			updateTXStats(line)
		}
	}()

	fmt.Printf("✓ TX started on %s (%s)\n", txVM, ip)
}

func updateRXStats(line string) {
	statsMutex.Lock()
	defer statsMutex.Unlock()
	
	if strings.Contains(line, "Received") {
		rxStats["packets_received"]++
	}
	if strings.Contains(line, "Sending") {
		rxStats["replies_sent"]++
	}
	
	if strings.Contains(line, "echo reply") {
		rxStats["icmp_replies"]++
	}
	if strings.Contains(line, "ICMP: Received type 8") {
		rxStats["icmp_echo_requests"]++
	}
	
	if strings.Contains(line, "TCP") {
		rxStats["tcp_packets"]++
	}
	if strings.Contains(line, "TCP: SYN") {
		rxStats["tcp_syn"]++
	}
	if strings.Contains(line, "TCP: ACK") {
		rxStats["tcp_ack"]++
	}
	if strings.Contains(line, "TCP: FIN") {
		rxStats["tcp_fin"]++
	}
	if strings.Contains(line, "TCP: Data") {
		rxStats["tcp_data"]++
	}
	if strings.Contains(line, "TCP: Connection established") {
		rxStats["tcp_connections"]++
	}
	
	if strings.Contains(line, "UDP") {
		rxStats["udp_packets"]++
	}
	if strings.Contains(line, "UDP: Data received") {
		rxStats["udp_data"]++
	}
	if strings.Contains(line, "UDP: Echo reply") {
		rxStats["udp_echo_replies"]++
	}
	
	if strings.Contains(line, "DNS") {
		rxStats["dns_packets"]++
	}
	if strings.Contains(line, "DNS: Query received") {
		rxStats["dns_queries"]++
	}
	if strings.Contains(line, "DNS: Response sent") {
		rxStats["dns_responses"]++
	}
	if strings.Contains(line, "DNS: A record") {
		rxStats["dns_a_records"]++
	}
	
	if strings.Contains(line, "ARP") {
		rxStats["arp_packets"]++
	}
	if strings.Contains(line, "ARP: Request received") {
		rxStats["arp_requests"]++
	}
	if strings.Contains(line, "ARP: Reply sent") {
		rxStats["arp_replies"]++
	}
	if strings.Contains(line, "ARP: Gratuitous") {
		rxStats["arp_gratuitous"]++
	}
}

func updateTXStats(line string) {
	statsMutex.Lock()
	defer statsMutex.Unlock()
	
	if strings.Contains(line, "Sent") {
		txStats["packets_sent"]++
	}
	if strings.Contains(line, "Burst mode") {
		txStats["burst_cycles"]++
	}
	if strings.Contains(line, "TRAFFIC:") {
		txStats["traffic_events"]++
	}
	
	if strings.Contains(line, "echo request") {
		txStats["icmp_requests"]++
	}
	if strings.Contains(line, "ICMP: Sent echo request") {
		txStats["icmp_echo_requests"]++
	}
	
	if strings.Contains(line, "TCP") {
		txStats["tcp_packets"]++
	}
	if strings.Contains(line, "TCP: SYN sent") {
		txStats["tcp_syn"]++
	}
	if strings.Contains(line, "TCP: ACK sent") {
		txStats["tcp_ack"]++
	}
	if strings.Contains(line, "TCP: FIN sent") {
		txStats["tcp_fin"]++
	}
	if strings.Contains(line, "TCP: Data sent") {
		txStats["tcp_data"]++
	}
	if strings.Contains(line, "TCP: Connection established") {
		txStats["tcp_connections"]++
	}
	if strings.Contains(line, "TCP: Connection failed") {
		txStats["tcp_connection_failures"]++
	}
	
	if strings.Contains(line, "UDP") {
		txStats["udp_packets"]++
	}
	if strings.Contains(line, "UDP: Data sent") {
		txStats["udp_data"]++
	}
	if strings.Contains(line, "UDP: Echo request") {
		txStats["udp_echo_requests"]++
	}
	
	if strings.Contains(line, "DNS") {
		txStats["dns_packets"]++
	}
	if strings.Contains(line, "DNS: Query sent") {
		txStats["dns_queries"]++
	}
	if strings.Contains(line, "DNS: Response received") {
		txStats["dns_responses"]++
	}
	if strings.Contains(line, "DNS: Resolved") {
		txStats["dns_resolved"]++
	}
	if strings.Contains(line, "DNS: Failed") {
		txStats["dns_failures"]++
	}
	
	if strings.Contains(line, "ARP") {
		txStats["arp_packets"]++
	}
	if strings.Contains(line, "ARP: Request sent") {
		txStats["arp_requests"]++
	}
	if strings.Contains(line, "ARP: Reply received") {
		txStats["arp_replies"]++
	}
	if strings.Contains(line, "ARP: Gratuitous sent") {
		txStats["arp_gratuitous"]++
	}
}

func displayStats() {
	ticker := time.NewTicker(2 * time.Second)
	defer ticker.Stop()
	
	for {
		select {
		case <-ticker.C:
			statsMutex.RLock()
			
			fmt.Print("\033[2J\033[H") // Clear screen
			fmt.Println("=== TRAFFIC STATISTICS ===")
			fmt.Printf("Protocol: %s | Mode: %s\n", protocol, mode)
			fmt.Printf("RX VM: %s | TX VM: %s\n", rxVM, txVM)
			fmt.Println("==========================")
			fmt.Println()
			
			// RX Stats
			fmt.Println("📥 RX STATISTICS:")
			if rxStats["packets_received"] > 0 {
				fmt.Printf("  • Packets Received: %d\n", rxStats["packets_received"])
			}
			if rxStats["replies_sent"] > 0 {
				fmt.Printf("  • Replies Sent: %d\n", rxStats["replies_sent"])
			}
			
			// Protocol-specific detailed stats
			if protocol == "icmp" {
				if rxStats["icmp_echo_requests"] > 0 {
					fmt.Printf("  • ICMP Echo Requests: %d\n", rxStats["icmp_echo_requests"])
				}
				if rxStats["icmp_replies"] > 0 {
					fmt.Printf("  • ICMP Echo Replies: %d\n", rxStats["icmp_replies"])
				}
			}
			
			if protocol == "tcp" {
				if rxStats["tcp_packets"] > 0 {
					fmt.Printf("  • TCP Total Packets: %d\n", rxStats["tcp_packets"])
				}
				if rxStats["tcp_syn"] > 0 {
					fmt.Printf("  • TCP SYN: %d\n", rxStats["tcp_syn"])
				}
				if rxStats["tcp_ack"] > 0 {
					fmt.Printf("  • TCP ACK: %d\n", rxStats["tcp_ack"])
				}
				if rxStats["tcp_data"] > 0 {
					fmt.Printf("  • TCP Data: %d\n", rxStats["tcp_data"])
				}
				if rxStats["tcp_fin"] > 0 {
					fmt.Printf("  • TCP FIN: %d\n", rxStats["tcp_fin"])
				}
				if rxStats["tcp_connections"] > 0 {
					fmt.Printf("  • TCP Connections: %d\n", rxStats["tcp_connections"])
				}
			}
			
			if protocol == "udp" {
				if rxStats["udp_packets"] > 0 {
					fmt.Printf("  • UDP Total Packets: %d\n", rxStats["udp_packets"])
				}
				if rxStats["udp_data"] > 0 {
					fmt.Printf("  • UDP Data: %d\n", rxStats["udp_data"])
				}
				if rxStats["udp_echo_replies"] > 0 {
					fmt.Printf("  • UDP Echo Replies: %d\n", rxStats["udp_echo_replies"])
				}
			}
			
			if protocol == "dns" {
				if rxStats["dns_packets"] > 0 {
					fmt.Printf("  • DNS Total Packets: %d\n", rxStats["dns_packets"])
				}
				if rxStats["dns_queries"] > 0 {
					fmt.Printf("  • DNS Queries: %d\n", rxStats["dns_queries"])
				}
				if rxStats["dns_responses"] > 0 {
					fmt.Printf("  • DNS Responses: %d\n", rxStats["dns_responses"])
				}
				if rxStats["dns_a_records"] > 0 {
					fmt.Printf("  • DNS A Records: %d\n", rxStats["dns_a_records"])
				}
			}
			
			if protocol == "arp" {
				if rxStats["arp_packets"] > 0 {
					fmt.Printf("  • ARP Total Packets: %d\n", rxStats["arp_packets"])
				}
				if rxStats["arp_requests"] > 0 {
					fmt.Printf("  • ARP Requests: %d\n", rxStats["arp_requests"])
				}
				if rxStats["arp_replies"] > 0 {
					fmt.Printf("  • ARP Replies: %d\n", rxStats["arp_replies"])
				}
				if rxStats["arp_gratuitous"] > 0 {
					fmt.Printf("  • ARP Gratuitous: %d\n", rxStats["arp_gratuitous"])
				}
			}
			
			fmt.Println()
			
			// TX Stats
			fmt.Println("📤 TX STATISTICS:")
			if txStats["packets_sent"] > 0 {
				fmt.Printf("  • Packets Sent: %d\n", txStats["packets_sent"])
			}
			if txStats["burst_cycles"] > 0 {
				fmt.Printf("  • Burst Cycles: %d\n", txStats["burst_cycles"])
			}
			if txStats["traffic_events"] > 0 {
				fmt.Printf("  • Traffic Events: %d\n", txStats["traffic_events"])
			}
			
			// Protocol-specific detailed stats
			if protocol == "icmp" {
				if txStats["icmp_echo_requests"] > 0 {
					fmt.Printf("  • ICMP Echo Requests: %d\n", txStats["icmp_echo_requests"])
				}
				if txStats["icmp_requests"] > 0 {
					fmt.Printf("  • ICMP Requests: %d\n", txStats["icmp_requests"])
				}
			}
			
			if protocol == "tcp" {
				if txStats["tcp_packets"] > 0 {
					fmt.Printf("  • TCP Total Packets: %d\n", txStats["tcp_packets"])
				}
				if txStats["tcp_syn"] > 0 {
					fmt.Printf("  • TCP SYN: %d\n", txStats["tcp_syn"])
				}
				if txStats["tcp_ack"] > 0 {
					fmt.Printf("  • TCP ACK: %d\n", txStats["tcp_ack"])
				}
				if txStats["tcp_data"] > 0 {
					fmt.Printf("  • TCP Data: %d\n", txStats["tcp_data"])
				}
				if txStats["tcp_fin"] > 0 {
					fmt.Printf("  • TCP FIN: %d\n", txStats["tcp_fin"])
				}
				if txStats["tcp_connections"] > 0 {
					fmt.Printf("  • TCP Connections: %d\n", txStats["tcp_connections"])
				}
				if txStats["tcp_connection_failures"] > 0 {
					fmt.Printf("  • TCP Connection Failures: %d\n", txStats["tcp_connection_failures"])
				}
			}
			
			if protocol == "udp" {
				if txStats["udp_packets"] > 0 {
					fmt.Printf("  • UDP Total Packets: %d\n", txStats["udp_packets"])
				}
				if txStats["udp_data"] > 0 {
					fmt.Printf("  • UDP Data: %d\n", txStats["udp_data"])
				}
				if txStats["udp_echo_requests"] > 0 {
					fmt.Printf("  • UDP Echo Requests: %d\n", txStats["udp_echo_requests"])
				}
			}
			
			if protocol == "dns" {
				if txStats["dns_packets"] > 0 {
					fmt.Printf("  • DNS Total Packets: %d\n", txStats["dns_packets"])
				}
				if txStats["dns_queries"] > 0 {
					fmt.Printf("  • DNS Queries: %d\n", txStats["dns_queries"])
				}
				if txStats["dns_responses"] > 0 {
					fmt.Printf("  • DNS Responses: %d\n", txStats["dns_responses"])
				}
				if txStats["dns_resolved"] > 0 {
					fmt.Printf("  • DNS Resolved: %d\n", txStats["dns_resolved"])
				}
				if txStats["dns_failures"] > 0 {
					fmt.Printf("  • DNS Failures: %d\n", txStats["dns_failures"])
				}
			}
			
			if protocol == "arp" {
				if txStats["arp_packets"] > 0 {
					fmt.Printf("  • ARP Total Packets: %d\n", txStats["arp_packets"])
				}
				if txStats["arp_requests"] > 0 {
					fmt.Printf("  • ARP Requests: %d\n", txStats["arp_requests"])
				}
				if txStats["arp_replies"] > 0 {
					fmt.Printf("  • ARP Replies: %d\n", txStats["arp_replies"])
				}
				if txStats["arp_gratuitous"] > 0 {
					fmt.Printf("  • ARP Gratuitous: %d\n", txStats["arp_gratuitous"])
				}
			}
			
			fmt.Println()
			// Show protocol-specific info
			if domain != "" {
				fmt.Printf("  • Domain: %s\n", domain)
			}
			if message != "" {
				fmt.Printf("  • Message: %s\n", message)
			}
			
			fmt.Println()
			fmt.Println("Press Ctrl+C to stop...")
			if tcpdumpFile != "" {
				fmt.Printf("PCAP saved to: %s\n", tcpdumpFile)
			}
			
			statsMutex.RUnlock()
			
		case <-stopChan:
			return
		}
	}
}

func stopTraffic() {
	fmt.Println("=== Stopping Traffic Test ===")
	
	fmt.Println("1. Stopping TX session...")
	if txSession != nil {
		txSession.Close()
		fmt.Println("✓ TX session stopped")
	}
	
	if txClient != nil {
		txClient.Close()
	}
	
	fmt.Println("2. Stopping RX session...")
	if rxSession != nil {
		rxSession.Close()
		fmt.Println("✓ RX session stopped")
	}
	
	if rxClient != nil {
		rxClient.Close()
	}
	
	fmt.Println("3. Stopping tcpdump capture...")
	stopTcpdump()
	
	fmt.Println("✓ All connections closed")
}

func createSSHConfig() *ssh.ClientConfig {
	return &ssh.ClientConfig{
		User: "root",
		Auth: []ssh.AuthMethod{
			ssh.Password("root"),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
		Timeout:         10 * time.Second,
	}
}

func extractIP(output string) string {
	lines := strings.Split(output, "\n")
	for _, line := range lines {
		if strings.Contains(line, "ipv4") {
			parts := strings.Fields(line)
			if len(parts) >= 4 {
						ip := strings.Split(parts[3], "/")[0]
		if ip != "127.0.0.1" && ip != "localhost" {
					return ip
				}
			}
		}
	}
	return ""
}

func startTcpdump() {
	// Check if tcpdump is available
	if _, err := exec.LookPath("tcpdump"); err != nil {
		fmt.Printf("Warning: tcpdump not found in PATH: %v\n", err)
		return
	}
	
	// Check if br-test interface exists
	if _, err := exec.Command("ip", "link", "show", "br-test").Output(); err != nil {
		fmt.Printf("Warning: br-test interface not found. Trying to find available interfaces...\n")
		interfaces, _ := exec.Command("ip", "link", "show").Output()
		fmt.Printf("Available interfaces:\n%s\n", string(interfaces))
		return
	}
	
	timestamp := time.Now().Format("20060102-150405")
	tcpdumpFile = fmt.Sprintf("traffic_%s_%s_%s.pcap", protocol, mode, timestamp)
	
	tcpdumpCmd = exec.Command("sudo", "tcpdump", "-i", "br-test", "-w", tcpdumpFile, "-s", "0", "-U", "-v")
	
	err := tcpdumpCmd.Start()
	if err != nil {
		tcpdumpCmd = exec.Command("tcpdump", "-i", "br-test", "-w", tcpdumpFile, "-s", "0", "-U", "-v")
		err = tcpdumpCmd.Start()
		if err != nil {
			fmt.Printf("Warning: Could not start tcpdump: %v\n", err)
			return
		}
	}
	
	time.Sleep(2 * time.Second)
	
	if tcpdumpCmd.Process == nil {
		fmt.Printf("Warning: tcpdump process not started\n")
		return
	}
	
	if tcpdumpCmd.ProcessState != nil && tcpdumpCmd.ProcessState.Exited() {
		fmt.Printf("Warning: tcpdump process exited immediately\n")
		return
	}
	
	fmt.Printf("✓ Started tcpdump capture on br-test interface (PID: %d)\n", tcpdumpCmd.Process.Pid)
	fmt.Printf("  PCAP file: %s\n", tcpdumpFile)
}

func stopTcpdump() {
	if tcpdumpCmd != nil && tcpdumpCmd.Process != nil {
		fmt.Printf("  Stopping tcpdump process (PID: %d)...\n", tcpdumpCmd.Process.Pid)
		
		tcpdumpCmd.Process.Signal(syscall.SIGTERM)
		
		time.Sleep(2 * time.Second)
		
		if tcpdumpCmd.ProcessState == nil || !tcpdumpCmd.ProcessState.Exited() {
			fmt.Printf("  Force killing tcpdump...\n")
			tcpdumpCmd.Process.Kill()
			time.Sleep(500 * time.Millisecond)
		}
		
		fmt.Printf("✓ Stopped tcpdump capture\n")
		fmt.Printf("  PCAP file: %s\n", tcpdumpFile)
		
		if _, err := os.Stat(tcpdumpFile); err == nil {
			size := getFileSize(tcpdumpFile)
			fmt.Printf("  File size: %d bytes\n", size)
			if size == 0 {
				fmt.Printf("  Warning: PCAP file is empty\n")
			}
		} else {
			fmt.Printf("  Warning: PCAP file not found: %v\n", err)
			files, _ := os.ReadDir(".")
			fmt.Printf("  Files in directory:\n")
			for _, file := range files {
				if strings.HasSuffix(file.Name(), ".pcap") {
					fmt.Printf("    - %s\n", file.Name())
				}
			}
		}
	} else {
		fmt.Printf("  Warning: tcpdump process not found\n")
	}
}

func getFileSize(filename string) int64 {
	info, err := os.Stat(filename)
	if err != nil {
		return 0
	}
	return info.Size()
}

func init() {
	runTrafficCmd.Flags().StringVar(&rxVM, "rx-vm", "", "RX VM name")
	runTrafficCmd.Flags().StringVar(&txVM, "tx-vm", "", "TX VM name")
	runTrafficCmd.Flags().StringVar(&protocol, "protocol", "", "Protocol (icmp, udp, tcp, dns, arp, eth, random)")
	runTrafficCmd.Flags().StringVar(&mode, "mode", "", "Traffic mode (continuous, burst, rate-limited, exponential-backoff, random)")
	runTrafficCmd.Flags().StringVar(&domain, "domain", "", "Domain for DNS")
	runTrafficCmd.Flags().StringVar(&message, "message", "", "Message for UDP/TCP")
} 