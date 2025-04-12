# Makefile for DPDK-based app

APP = dpdk_app
SRC = main.c

CFLAGS = -O3 -g -Wall -Wextra -I/usr/local/include -march=native
LDFLAGS = -L/usr/local/lib -Wl,-rpath=/usr/local/lib
LIBS = -lrte_eal -lrte_mbuf -lrte_mempool -lrte_ring -lrte_ethdev -pthread

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(APP)
