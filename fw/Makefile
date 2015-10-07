PROJECT := m4audio

# Cross-platform sources
#SRCS += src/

# Sources to build for target only
ARM_SRCS += src/target/platform-stm32.c
ARM_SRCS += src/target/usb.c
ARM_SRCS += src/target/wm8731.c

# Sources to build for host only
HOST_SRCS += src/host/platform-host.c
HOST_SRCS += src/host/jackclient.c

# -------------------------------------

BUILDDIR := build
HOSTBUILDDIR := build_host

PREFIX := arm-none-eabi
ARMCC := $(PREFIX)-gcc
# Make libopencm3's makefiles happy
CC := $(ARMCC)
ARMOBJCOPY := $(PREFIX)-objcopy
ARMOBJDUMP := $(PREFIX)-objdump

HOST_CC := gcc

LIBOPENCM3_DIR := libopencm3
LIBOPENCM3 := $(LIBOPENCM3_DIR)/lib/libopencm3_stm32f4.a

ARM_LDFLAGS := -L$(LIBOPENCM3_DIR)/lib/
ARM_LDFLAGS += -nostartfiles --specs=nosys.specs --specs=nano.specs
ARM_LDFLAGS += -Wl,--gc-sections -lm

ARM_COMMONFLAGS := -I. -Isrc -Isrc/target -Iinclude
ARM_COMMONFLAGS += -std=c11 -O1 -fno-common -g -fconserve-stack
#ARM_COMMONFLAGS += -funsafe-math-optimizations
ARM_COMMONFLAGS += -Wall -Wextra -Werror-implicit-function-declaration -Werror -Wno-error=unused-variable
ARM_COMMONFLAGS += -I$(LIBOPENCM3_DIR)/include/

HOST_COMMONFLAGS := -I. -Isrc -Isrc/host -Iinclude
HOST_COMMONFLAGS += -std=c11 -O1 -fno-common -g -Wall -Wextra -DHOST
HOST_COMMONFLAGS += -Wall -Wextra -Werror-implicit-function-declaration  -Werror -Wno-error=unused-variable
HOST_COMMONFLAGS += $(shell pkg-config --cflags jack)

HOST_LDFLAGS += $(shell pkg-config --libs jack) -lm

ARM_OBJS += $(ARM_SRCS:src/%.c=$(BUILDDIR)/%.o)
HOST_OBJS += $(HOST_SRCS:src/%.c=$(HOSTBUILDDIR)/%.o)

# -------------------------------------

.PHONY: all
all: $(BUILDDIR)/feedthrough.elf $(HOSTBUILDDIR)/feedthrough.elf
all: $(BUILDDIR)/sine.elf $(HOSTBUILDDIR)/sine.elf
all: $(BUILDDIR)/delay.elf $(HOSTBUILDDIR)/delay.elf
all: $(BUILDDIR)/fxbox.elf $(HOSTBUILDDIR)/fxbox.elf

$(LIBOPENCM3_DIR)/ld/Makefile.linker:
	git submodule update --init $(LIBOPENCM3_DIR)

# Generate linker script for the MCU and let libopencm3 set up the appropriate
# compiler flags
DEVICE=stm32f405rg
SRCLIBDIR=$(LIBOPENCM3_DIR)
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

flash: flash-fxbox

dfu: dfu-fxbox

dfu-%: $(BUILDDIR)/%.bin
	dfu-util -a 0 -s 0x08000000 -D $<

flash-%: $(BUILDDIR)/%.elf
	openocd -f interface/stlink-v2.cfg \
		    -f board/stm32f4discovery.cfg \
		    -c "init" -c "reset init" \
		    -c "flash write_image erase $<" \
		    -c "reset" \
		    -c "shutdown"

$(BUILDDIR)/%.o: src/%.c Makefile $(LIBOPENCM3)
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

$(BUILDDIR)/feedthrough.elf: $(ARM_OBJS) $(BUILDDIR)/examples/feedthrough.o
$(BUILDDIR)/sine.elf: $(ARM_OBJS) $(BUILDDIR)/examples/sine.o
$(BUILDDIR)/delay.elf: $(ARM_OBJS) $(BUILDDIR)/examples/delay.o
$(BUILDDIR)/fxbox.elf: $(ARM_OBJS) $(BUILDDIR)/fxbox/fxbox.o \
	$(BUILDDIR)/fxbox/vibrato.o $(BUILDDIR)/fxbox/wahwah.o \
	$(BUILDDIR)/fxbox/delay.o $(BUILDDIR)/fxbox/pitcher.o \
	$(BUILDDIR)/biquad.o

$(BUILDDIR)/%.elf: $(LIBOPENCM3) $(LDSCRIPT)
	@echo ARMLD $@
	@$(ARMCC) -o $@ $(filter %.o,$^) \
		$(ARM_COMMONFLAGS) $(CFLAGS) $(ARM_LDFLAGS)

# -------------------------------------
# Host rules

$(HOSTBUILDDIR)/%.o: src/%.c Makefile
	@echo CC $< --\> $@
	@mkdir -p $(dir $@)
	@$(HOST_CC) -MD $(HOST_COMMONFLAGS) $(HOST_CFLAGS) -c -o $@ $<

$(HOSTBUILDDIR)/feedthrough.elf: $(HOST_OBJS) $(HOSTBUILDDIR)/examples/feedthrough.o
$(HOSTBUILDDIR)/sine.elf: $(HOST_OBJS) $(HOSTBUILDDIR)/examples/sine.o
$(HOSTBUILDDIR)/delay.elf: $(HOST_OBJS) $(HOSTBUILDDIR)/examples/delay.o
$(HOSTBUILDDIR)/fxbox.elf: $(HOST_OBJS) $(HOSTBUILDDIR)/fxbox/fxbox.o \
	$(HOSTBUILDDIR)/fxbox/vibrato.o $(HOSTBUILDDIR)/fxbox/wahwah.o \
	$(HOSTBUILDDIR)/fxbox/delay.o $(HOSTBUILDDIR)/fxbox/pitcher.o \
	$(HOSTBUILDDIR)/biquad.o

$(HOSTBUILDDIR)/%.elf:
	@echo LD $@
	@$(HOST_CC) -o $@ $(filter %.o,$^) \
		$(HOST_COMMONFLAGS) $(HOST_CFLAGS) $(HOST_LDFLAGS)

# -------------------------------------

-include $(BUILDDIR)/*.d $(BUILDDIR)/*/*.d
-include $(HOSTBUILDDIR)/*.d $(HOSTBUILDDIR)/*/*.d

.PHONY: clean
clean:
	rm -rf $(BUILDDIR) $(HOSTBUILDDIR)
