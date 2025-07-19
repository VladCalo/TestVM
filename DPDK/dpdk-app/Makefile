APP = traffic_engine

SRCS = src/main/traffic_engine.c \
       src/core/config.c \
       src/core/common.c \
       src/core/log.c \
       src/core/packet_utils.c \
       src/protocols/eth.c \
       src/protocols/icmp.c \
       src/protocols/udp.c \
       src/protocols/tcp.c

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