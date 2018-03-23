.PHONY: all
all: $(BUILDDIR)/feedthrough.elf $(BUILDDIR)/sine.elf $(BUILDDIR)/delay.elf
all: $(BUILDDIR)/fxbox.elf $(BUILDDIR)/fxbox2.elf $(BUILDDIR)/guitar.elf
all: $(BUILDDIR)/fft_tests.elf $(BUILDDIR)/sawsynth.elf

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)

$(BUILDDIR)/%.o: src/%.c Makefile $(LIBOPENCM3)
	@echo CC $@
	@mkdir -p $(dir $@)
	@$(CC) -MD $(COMMONFLAGS) $(CFLAGS) -c -o $@ $<

# -------------------------------------

COMMONFLAGS += -Isrc/kiss_fft130

$(BUILDDIR)/feedthrough.elf: $(COMMON_OBJS) $(BUILDDIR)/examples/feedthrough.o
$(BUILDDIR)/sine.elf: $(COMMON_OBJS) $(BUILDDIR)/examples/sine.o
$(BUILDDIR)/delay.elf: $(COMMON_OBJS) $(BUILDDIR)/examples/delay.o
$(BUILDDIR)/fxbox.elf: $(COMMON_OBJS) $(BUILDDIR)/fxbox.o \
	$(BUILDDIR)/dsp/vibrato.o $(BUILDDIR)/dsp/wahwah.o \
	$(BUILDDIR)/dsp/delay.o $(BUILDDIR)/dsp/pitcher.o \
	$(BUILDDIR)/dsp/biquad.o
$(BUILDDIR)/fxbox2.elf: $(COMMON_OBJS) $(BUILDDIR)/fxbox2.o \
	$(BUILDDIR)/dsp/vibrato.o $(BUILDDIR)/dsp/biquad.o \
	$(BUILDDIR)/dsp/delay.o
$(BUILDDIR)/guitar.elf: $(COMMON_OBJS) $(BUILDDIR)/guitar.o \
	$(BUILDDIR)/dsp/vibrato.o $(BUILDDIR)/dsp/delay.o
$(BUILDDIR)/fft_tests.elf: $(COMMON_OBJS) $(BUILDDIR)/tests/fft_tests.o
$(BUILDDIR)/fft_tests.elf: $(BUILDDIR)/kiss_fft130/kiss_fft.o
$(BUILDDIR)/fft_tests.elf: $(BUILDDIR)/kiss_fft130/tools/kiss_fftr.o

src/sawsynth/wt.h: src/sawsynth/make_wavetable.py
	$< > $@
$(BUILDDIR)/sawsynth/sawsynth.o: src/sawsynth/wt.h
$(BUILDDIR)/sawsynth.elf: $(COMMON_OBJS) $(BUILDDIR)/sawsynth/sawsynth.o $(BUILDDIR)/dsp/biquad.o

$(BUILDDIR)/%.elf: $(LIBOPENCM3) $(LDSCRIPT)
	@echo LD $@
	@$(CC) -o $@ $(filter %.o,$^) $(COMMONFLAGS) $(CFLAGS) $(LDFLAGS)

# -------------------------------------

-include $(BUILDDIR)/*.d $(BUILDDIR)/*/*.d
