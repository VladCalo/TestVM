#!/bin/bash

ip link set eth2 name eth0_tmp
ip link set eth0 name eth2
ip link set eth0_tmp name eth0