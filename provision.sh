#!/bin/bash

set -x

# Package installation
install_packages() {
    apt-get update
    apt-get install -y \
        curl \
        vim \
        openssh-server \
        openssh-client
}

configure_service() {
    local service_folder=${1}
    cd systemd-services/${service_folder}

    service_file=$(ls *.service)
    script_file=$(ls *.sh)

    mv ${service_file} /etc/systemd/system/
    mv ${script_file} /usr/bin/

    chmod +x /usr/bin/${script_file}

    systemctl start ${service_folder}
    systemctl enable ${service_folder}
    systemctl daemon-reload
}

configure_system_services() {
    configure_service "interface-renaming"
    rm -rf systemd-services
}

configure_ssh() {
    systemctl enable ssh
    systemctl start ssh
}

main() {
    install_packages
    configure_ssh
    configure_system_services
}

main