# TestVM (test/vladcalo) + from qcow2
create bridges on KVM

managemnt is NAT and connected to wifi card
test interfaces are bridge and type virtio

#### DPDK pmd test start:
- sudo dpdk-testpmd -l 0-1 -n 4 -a 0000:02:00.0 -- -i --port-topology=paired

#### DPDK bind:
- /usr/local/bin/dpdk-devbind.py --bind=uio_pci_generic 0000:02:00.0

#### DPDK App start:

##### 1. ICMP
###### Note: RX mac address currenty hardcoded (line 47 icmp.c)
^^^Fix before starting dpdk extract pci-id, mac and save them in a file on disk
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

##### 3. TCP fake handshake scenario 
###### Note: hardcoded msg and hardcoded TX/RX mac
- RX: sudo ./traffic_engine rx tcp -l 0 -n 4 -a 0000:02:00.0
- TX: sudo ./traffic_engine tx tcp -l 0 -n 4 -a 0000:02:00.0



