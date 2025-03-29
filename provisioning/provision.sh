#!/bin/bash

set -x

# Package installation
install_packages() {
    apt-get update
    apt-get install -y \
        curl \
        vim \
        openssh-server \
        openssh-client \
        iperf3 \
        meson \
        ninja-build \
        build-essential \
        pkg-config \
        libnuma-dev \
        python3 \
        python3-pip \
        python3-pyelftools
}

build_DPDK() {
    cd ../src/dpdk-*/

    meson -Dexamples=all build
    ninja -C build
    cd build
    ninja install
    ldconfig
}

configure_DPDK() {
    modprobe uio
    modprobe uio_pci_generic
    echo "uio" >> /etc/modules
    echo "uio_pci_generic" >> /etc/modules

    sed -i 's/\(GRUB_CMDLINE_LINUX=".*\)"/\1 intel_iommu=on"/' /etc/default/grub
    update-grub
    # pci id: basename $(readlink -f /sys/class/net/eth1/device/../)
}

install_DPDK(){
    build_DPDK
    configure_DPDK
}

configure_services() {
    for svc in systemd-services/*.service; do
        mv "$svc" /etc/systemd/system/
        systemctl daemon-reload
        systemctl enable $(basename -s .service "$svc")
    done
}

main() {
    install_packages
    install_DPDK
    configure_services
    
    reboot
}

main