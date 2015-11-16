.PHONY: all
all: $(BUILDDIR)/feedthrough.elf $(BUILDDIR)/sine.elf $(BUILDDIR)/delay.elf
all: $(BUILDDIR)/fxbox.elf

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)

$(BUILDDIR)/%.o: src/%.c Makefile $(LIBOPENCM3)
	@echo CC $@
	@mkdir -p $(dir $@)
	@$(CC) -MD $(COMMONFLAGS) $(CFLAGS) -c -o $@ $<

# -------------------------------------

$(BUILDDIR)/feedthrough.elf: $(COMMON_OBJS) $(BUILDDIR)/examples/feedthrough.o
$(BUILDDIR)/sine.elf: $(COMMON_OBJS) $(BUILDDIR)/examples/sine.o
$(BUILDDIR)/delay.elf: $(COMMON_OBJS) $(BUILDDIR)/examples/delay.o
$(BUILDDIR)/fxbox.elf: $(COMMON_OBJS) $(BUILDDIR)/fxbox.o \
	$(BUILDDIR)/dsp/vibrato.o $(BUILDDIR)/dsp/wahwah.o \
	$(BUILDDIR)/dsp/delay.o $(BUILDDIR)/dsp/pitcher.o \
	$(BUILDDIR)/dsp/biquad.o

$(BUILDDIR)/%.elf: $(LIBOPENCM3) $(LDSCRIPT)
	@echo LD $@
	@$(CC) -o $@ $(filter %.o,$^) $(COMMONFLAGS) $(CFLAGS) $(LDFLAGS)

# -------------------------------------

-include $(BUILDDIR)/*.d $(BUILDDIR)/*/*.d
