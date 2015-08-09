PROJECT := m4audio

# Cross-platform sources
#SRCS += src/

# Sources to build for target only
ARM_SRCS += src/target/stm32.c

# Sources to build for host only
HOST_SRCS += src/host/host.c

# -------------------------------------

BUILDDIR := build
HOSTBUILDDIR := build_host

PREFIX := arm-none-eabi
ARMCC := $(PREFIX)-gcc
ARMLD := $(PREFIX)-gcc
ARMOBJCOPY := $(PREFIX)-objcopy
ARMOBJDUMP := $(PREFIX)-objdump

LIBOPENCM3_DIR := libopencm3
LIBOPENCM3 := $(LIBOPENCM3_DIR)/lib/libopencm3_stm32f4.a

ARM_LDFLAGS += -L$(LIBOPENCM3_DIR)/lib/ -lopencm3_stm32f4
ARM_LDFLAGS += -lc -lnosys -nostartfiles -Wl,--gc-sections
# FIXME: Cortex-M4 flags for using FPU
ARM_LDFLAGS += -mthumb -march=armv7 -mfix-cortex-m3-ldrd -msoft-float

ARM_COMMONFLAGS += -I. -Isrc -Isrc/target -O1 -fno-common -g -fconserve-stack
ARM_COMMONFLAGS += -mcpu=cortex-m4 -mthumb -DSTM32F4
ARM_COMMONFLAGS += -Wall -Wextra
ARM_COMMONFLAGS += -I$(LIBOPENCM3_DIR)/include/
HOST_COMMONFLAGS += -I. -Isrc -Isrc/host -O0 -fno-common -g -Wall -Wextra -DHOST

CFLAGS += -std=c99 -Werror-implicit-function-declaration

ARM_OBJS += $(ARM_SRCS:src/%.c=$(BUILDDIR)/%.o)
HOST_OBJS += $(HOST_SRCS:src/%.c=$(HOSTBUILDDIR)/%.o)

# -------------------------------------

.PHONY: all
all: $(BUILDDIR)/feedtrough.elf

# Generate linker script for the MCU
DEVICE=stm32f405rg
SRCLIBDIR=$(LIBOPENCM3_DIR)
CC=$(ARMCC)
include $(LIBOPENCM3_DIR)/ld/Makefile.linker
ARM_LDFLAGS += -Wl,-T$(LDSCRIPT)


$(LIBOPENCM3):
	@echo
	@echo "##### Building libopencm3"
	@echo
	cd $(LIBOPENCM3_DIR) && $(MAKE) TARGETS=stm32/f4
	touch $@

# -------------------------------------
# ARM rules

$(BUILDDIR)/%.o: src/%.c Makefile
	@echo ARMCC $< --\> $@
	@mkdir -p $(dir $@)
	@$(ARMCC) -MD $(ARM_COMMONFLAGS) $(CFLAGS) -c -o $@ $<

%.bin: %.elf
	@echo flattening $@
	@$(ARMOBJCOPY) -Obinary $< $@

%.asm: %.elf
	@echo disassembling $@
	@$(ARMOBJDUMP) -d -S $< > $@

# -------------------------------------
# ARM executables rules

$(BUILDDIR)/feedtrough.elf: $(ARM_OBJS) $(BUILDDIR)/feedthrough/feedthrough.o $(LIBOPENCM3) $(LDSCRIPT)
	@echo ARMLD $@
	@$(ARMLD) -o $@ $(ARM_OBJS) $(BUILDDIR)/feedthrough/feedthrough.o $(ARM_LDFLAGS)

# -------------------------------------
# Host rules

$(HOSTBUILDDIR)/%.o: %.c Makefile
	@echo CC $< --\> $@
	@mkdir -p $(dir $@)
	@$(CC) -MD $(HOST_COMMONFLAGS) $(CFLAGS) -c -o $@ $<

# -------------------------------------

-include $(BUILDDIR)/src/*.d
-include $(HOSTBUILDDIR)/src/*.d

.PHONY: clean
clean:
	rm -rf $(BUILDDIR) $(HOSTBUILDDIR)
