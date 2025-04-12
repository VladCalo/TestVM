# TestVM (test/vladcalo) + from qcow2
create bridges on KVM

managemnt is NAT and connected to wifi card
test interfaces are bridge and type virtio

DPDK atest start:
sudo dpdk-testpmd -l 0-1 -n 4 -a 0000:02:00.0 -- -i --port-topology=paired

sudo apt install -y git build-essential libnuma-dev luajit libluajit-5.1-dev cmake python-is-python3
cd ~/src
git clone https://github.com/emmericp/MoonGen.git
cd MoonGen
./build.sh /usr/local