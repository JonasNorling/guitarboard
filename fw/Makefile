.PHONY: all
all:
	$(MAKE) -f target.mk $@
	$(MAKE) -f host.mk $@

.PHONY: clean
clean:
	$(MAKE) -f target.mk $@
	$(MAKE) -f host.mk $@

.PHONY: flash
flash: flash-fxbox

.PHONY: dfu
dfu: dfu-fxbox

flash-%:
	$(MAKE) -f target.mk $@

dfu-%:
	$(MAKE) -f target.mk $@

build/%:
	$(MAKE) -f target.mk $@

build_host/%:
	$(MAKE) -f host.mk $@
