# TestVM
managemnt is bridged and connected to wifi card
test interfaces are internal network (intnet) and type virtio


to do:
set ip for test interfaces (temporar => va fi facut din CLI)




sudo dpdk-testpmd -l 0-1 -n 4 -a 0000:02:00.0 -- -i --port-topology=paired
