# UIOX Device Tree Makefile
# Compiles all three .dts sources to .dtb binary blobs
#
# Prerequisites:
#   apt  install device-tree-compiler   (Debian/Ubuntu)
#   brew install dtc                     (macOS)
#
# Usage:
#   make          — build all three .dtb files
#   make check    — build + run dtc lint on each
#   make clean    — remove .dtb files

DTC      ?= dtc
DTFLAGS  := -I dts -O dtb -W no-unit_address_vs_reg

SRCS     := uiox-arm64.dts uiox-x86_64.dts uiox-riscv64.dts
DTBS     := $(SRCS:.dts=.dtb)

.PHONY: all check clean

all: $(DTBS)
	@echo ""
	@echo "=== UIOX DTB files built ==="
	@ls -lh $(DTBS)

%.dtb: %.dts
	$(DTC) $(DTFLAGS) -o $@ $<
	@echo "  Built $@"

check: $(DTBS)
	@for f in $(DTBS); do \
	    echo "--- Checking $$f ---"; \
	    $(DTC) -I dtb -O dts $$f > /dev/null && echo "  OK"; \
	done

# Decompile back to DTS for inspection
%.inspect: %.dtb
	$(DTC) -I dtb -O dts -o $(@:.inspect=.inspect.dts) $<

clean:
	rm -f $(DTBS) *.inspect.dts
