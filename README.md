# TestVM (test/vladcalo) + from qcow2
create bridges on KVM

managemnt is NAT and connected to wifi card
test interfaces are bridge and type virtio

#### DPDK pmd test start:
- sudo dpdk-testpmd -l 0-1 -n 4 -a 0000:02:00.0 -- -i --port-topology=paired

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



