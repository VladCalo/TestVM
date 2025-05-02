APP = traffic_engine

SRCS = traffic_engine.c src/eth.c src/icmp.c src/udp.c src/tcp.c
INCLUDES = -Iinclude

PKGCONF = pkg-config
DPDK_CFLAGS = $(shell $(PKGCONF) --cflags libdpdk)
DPDK_LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

CFLAGS += -O3 -g -Wall $(DPDK_CFLAGS) $(INCLUDES)
LDFLAGS += $(DPDK_LDFLAGS)

all: $(APP)

$(APP): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(APP)