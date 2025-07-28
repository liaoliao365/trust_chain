CC      ?= $(CROSS_COMPILE)gcc
LD      ?= $(CROSS_COMPILE)ld
AR      ?= $(CROSS_COMPILE)ar
NM      ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump
READELF ?= $(CROSS_COMPILE)readelf

OBJS = main.o

$(info TEEC_EXPORT is [${TEEC_EXPORT}])

CFLAGS += -Wall \
		-I../ta/include \
		-I$(TEEC_EXPORT)/include \
		-I./include \
		-I/home/lele/optee-qemu/buildroot/output/host/i586-buildroot-linux-gnu/sysroot/usr/include
#Add/link other required libraries here
LDADD += -lteec -ljansson -L$(TEEC_EXPORT)/lib

BINARY = optee_example_trust_chain

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $< $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
