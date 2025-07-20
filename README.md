# TestVM (test/vladcalo) + from qcow2
create bridges on KVM

managemnt is NAT and connected to wifi card
test interfaces are bridge and type virtio

#### Next-steps:
for dns pass the domain as args
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
- RX: sudo ./traffic_engine rx icmp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx icmp [traffic_mode] -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -n -e ip proto 1

##### 2. ETH Frames / Agnostic RX
- RX (Agnostic): sudo ./traffic_engine rx -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -nn -e ether proto 0x080

**Note**: Agnostic RX automatically detects and displays all protocols (ICMP, UDP, TCP, ARP, DNS, ETH)

##### 3. UDP 
###### Note: message can be specified as argument
- RX: sudo ./traffic_engine rx udp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx udp [traffic_mode] [message] -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test udp -n -e

##### 4. TCP fake handshake scenario 
###### Note: message can be specified as argument
- RX: sudo ./traffic_engine rx tcp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx tcp [traffic_mode] [message] -l 0 -n 4 -a 0000:02:00.0

##### 5. ARP
- RX: sudo ./traffic_engine rx arp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx arp [traffic_mode] -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -n -e arp

##### 6. DNS
###### Note: domain name can be specified as argument
- RX: sudo ./traffic_engine rx dns -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx dns [traffic_mode] [domain] -l 0 -n 4 -a 0000:02:00.0
- KVM: sudo tcpdump -i br-test -n -e udp port 53



