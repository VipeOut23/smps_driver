# Hardware #####################################################################
MCU ?= attiny85
F_CPU ?= 4000000 # 4MHz
PROGRAMMER ?= stk500v1
BAUD ?= 19200
PORT ?= /dev/ttyACM0

# Environment ##################################################################
BUILD = build
SOURCES = main.c uart.c
OBJECTS = $(patsubst %.c,$(BUILD)/%.o,$(SOURCES))

# Compiler & Tools #############################################################
CC = avr-gcc
CFLAGS += -std=c17 -Os
CPPFLAGS += -DF_CPU=$(F_CPU) -mmcu=$(MCU)
LDFLAGS +=
OBJCOPY = avr-objcopy
AVRDUDE ?= avrdude
CPPFLAGS += $(shell echo | $(CC) -xc -E -v - 2>&1 | grep -E '^\s' | sed '1d;s/^\s/-I/' | tr '\n' ' ')

# Rules ########################################################################
V ?= 0
ifeq ("$(V)","1")
	Q:=
else
	Q:=@
endif

std: rom.hex

flash: rom.hex
	$(AVRDUDE) -p$(MCU) -c$(PROGRAMMER) -P$(PORT) -b$(BAUD) -U flash:w:$^

rom.hex: $(BUILD)/rom.elf
	@echo IHEX  $@
	$(Q)$(OBJCOPY) -O ihex $^ $@

$(BUILD)/rom.elf: $(OBJECTS)
	@echo "LD   $@"
	$(Q)$(CC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

$(BUILD)/%.o: %.c
	$(Q)mkdir -p $(BUILD)
	@echo "CC   $@"
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $^

clean:
	@rm -f $(BUILD)/* rom.hex
	@rmdir -p $(BUILD) || true

.PHONY: std flash clean
