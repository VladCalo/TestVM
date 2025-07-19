# TestVM (test/vladcalo) + from qcow2
create bridges on KVM

managemnt is NAT and connected to wifi card
test interfaces are bridge and type virtio

#### Next-steps:
in provisioning create service that binds to dpdk at everyboot and sets ips, writes mac addresses on disk and then the C code reads them from there
go cli on kvm

#### DPDK pmd test start:
- sudo dpdk-testpmd -l 0-1 -n 4 -a 0000:02:00.0 -- -i --port-topology=paired

#### DPDK bind:
- /usr/local/bin/dpdk-devbind.py --bind=uio_pci_generic 0000:02:00.0

#### DPDK App start:

##### Traffic Modes:
- **continuous**: Constant traffic with 1-second intervals (default)
- **burst**: Send 10 packets rapidly, then pause for 5 seconds
- **rate-limited**: Send at 1000 packets per second
- **exponential-backoff**: Increasing delays between retries

##### Traffic modes examples TX only:
- TX (burst): sudo ./traffic_engine tx icmp burst -l 0 -n 4 -a 0000:02:00.0
- TX (rate-limited): sudo ./traffic_engine tx icmp rate-limited -l 0 -n 4 -a 0000:02:00.0

##### Usage: ./traffic_engine [tx|rx] [protocol] [traffic_mode]

##### 1. ICMP
###### Note: RX mac address currently hardcoded
- RX: sudo ./traffic_engine rx icmp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx icmp -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -n -e ip proto 1

##### 2. ETH Frames
- RX: sudo ./traffic_engine rx -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -nn -e ether proto 0x080

##### 3. UDP 
###### Note: hardcoded msg and hardcoded RX mac
- RX: sudo ./traffic_engine rx udp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx udp -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test udp -n -e

##### 4. TCP fake handshake scenario 
###### Note: hardcoded msg and hardcoded TX/RX mac
- RX: sudo ./traffic_engine rx tcp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx tcp -l 0 -n 4 -a 0000:02:00.0

##### 5. ARP
###### Note: hardcoded IP addresses
- RX: sudo ./traffic_engine rx arp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx arp -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -n -e arp

##### 6. DNS
###### Note: hardcoded domain name (example.com) and hardcoded MAC addresses
- RX: sudo ./traffic_engine rx dns -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx dns -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -n -e udp port 53



