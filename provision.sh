#!/bin/bash

# Package installation
install_packages() {
    apt-get update
    apt-get install -y \
        curl \
        vim
}

main() {
    install_packages
}

main