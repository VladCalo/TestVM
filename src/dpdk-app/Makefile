# Makefile for DPDK traffic_engine app

APP = traffic_engine
SRC = traffic_engine.c

PKGCONF = pkg-config
DPDK_CFLAGS = $(shell $(PKGCONF) --cflags libdpdk)
DPDK_LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

CFLAGS += -O3 -g -Wall $(DPDK_CFLAGS)
LDFLAGS += $(DPDK_LDFLAGS)

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(APP)