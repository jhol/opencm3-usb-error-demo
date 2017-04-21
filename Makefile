
TOP=$(dir $(lastword $(MAKEFILE_LIST)))

OPENCM3_DIR=$(TOP)/libopencm3

NEWLIB_DIR=

AR=arm-none-eabi-ar
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy

CFLAGS=-std=c11 -fvisibility=hidden -fdiagnostics-color=always -g -Wa,--compress-debug-sections -nostartfiles -mthumb \
  -mcpu=cortex-m0 -msoft-float -DNDEBUG -Os -I$(OPENCM3_DIR)/include -DSTM32F0
LDFLAGS=-std=c11 -fvisibility=hidden -fdiagnostics-color=always -g -nostartfiles -mthumb -mcpu=cortex-m0 -msoft-float \
  -Os -lc -lnosys -T$(TOP)/dio.ld

SRC_DIR=$(TOP)
SRC=$(SRC_DIR)/main.c
OUTDIR=$(TOP)
BIN=$(OUTDIR)/dio
FW=$(BIN).fw

OBJS=$(patsubst %.c,%.o,$(SRC))

all: $(FW)

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

$(BIN): $(OBJS)
	$(MAKE) -C libopencm3 AR=$(AR) CC=$(CC) CFLAGS="-std=c11"
	$(CC) $(LDFLAGS) $^ -o $@ -Wl,-Bstatic -L$(OPENCM3_DIR)/lib/ -lopencm3_stm32f0 -Wl,-Bdynamic

$(FW): $(BIN)
	$(OBJCOPY) -Obinary -j .text -j .data --pad-to=0x08008000 $^ $@

clean:
	$(MAKE) -C libopencm3 clean
	$(RM) $(FW) $(BIN) $(OBJS)
