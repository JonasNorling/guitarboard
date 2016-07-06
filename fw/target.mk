#
# Makefile with definitions for the ARM Cortex-M4 target
#

BUILDDIR := build

PREFIX := arm-none-eabi
CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump

LIBOPENCM3_DIR := libopencm3
LIBOPENCM3 := $(LIBOPENCM3_DIR)/lib/libopencm3_stm32f4.a

LDFLAGS := -L$(LIBOPENCM3_DIR)/lib/
LDFLAGS += -nostartfiles --specs=nosys.specs --specs=nano.specs
LDFLAGS += -Wl,--gc-sections -lm

COMMONFLAGS := -I. -Isrc -Isrc/target -Iinclude
COMMONFLAGS += -std=c11 -O1 -fno-common -g -fconserve-stack
#COMMONFLAGS += -funsafe-math-optimizations
COMMONFLAGS += -Wall -Wextra -Werror-implicit-function-declaration -Werror -Wno-error=unused-variable
COMMONFLAGS += -I$(LIBOPENCM3_DIR)/include/

# Sources to build for target only
SRCS += src/target/platform-stm32.c
SRCS += src/target/usb.c
SRCS += src/target/wm8731.c

COMMON_OBJS := $(SRCS:src/%.c=$(BUILDDIR)/%.o)

# -------------------------------------

$(LIBOPENCM3_DIR)/ld/Makefile.linker:
	git submodule update --init $(LIBOPENCM3_DIR)

# Generate linker script for the MCU and let libopencm3 set up the appropriate
# compiler flags
DEVICE=stm32f405rg
SRCLIBDIR=$(LIBOPENCM3_DIR)
include $(LIBOPENCM3_DIR)/ld/Makefile.linker
LDFLAGS += -Wl,-T$(LDSCRIPT)

include common.mk

# -------------------------------------

$(LIBOPENCM3):
	@echo
	@echo "##### Building libopencm3"
	@echo
	cd $(LIBOPENCM3_DIR) && $(MAKE) TARGETS=stm32/f4
	touch $@

.PHONY: flash
flash: flash-fxbox

.PHONY: dfu
dfu: dfu-fxbox

$(BUILDDIR)/%.bin-dfu: $(BUILDDIR)/%.bin
	cp $< $@
	dfu-suffix -a $@ -v 0483 -p df11

dfu-%: $(BUILDDIR)/%.bin-dfu
	dfu-util -a 0 -s 0x08000000 -D $<

flash-%: $(BUILDDIR)/%.elf
	openocd -f interface/stlink-v2.cfg \
		    -f board/stm32f4discovery.cfg \
		    -c "init" -c "reset init" \
		    -c "flash write_image erase $<" \
		    -c "reset" \
		    -c "shutdown"

%.bin: %.elf
	@echo flattening $@
	@$(OBJCOPY) -Obinary $< $@

%.asm: %.elf
	@echo disassembling $@
	@$(OBJDUMP) -d -S $< > $@
