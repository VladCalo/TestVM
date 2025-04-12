from scapy.all import *
import socket
import struct

# Create a raw Ethernet + IP + ICMP packet
pkt = Ether(dst="52:54:00:65:D9:EA")/IP(dst="10.0.0.2")/ICMP()

# Serialize the packet
raw_pkt = bytes(pkt)

# Prepend the length (as 2 bytes in network byte order)
length_prefix = struct.pack("H", len(raw_pkt))

# Send over the Unix domain socket
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect("/tmp/dpdk_sock")
sock.sendall(length_prefix + raw_pkt)
sock.close()
