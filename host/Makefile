CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lpthread -ljansson

# 需要安装 jansson 库
# sudo apt-get install libjansson-dev

all: trust_chain_service

trust_chain_service: main.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -I../ta/include

clean:
	rm -f trust_chain_service

.PHONY: all clean
 