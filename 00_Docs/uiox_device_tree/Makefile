# =============================================================================
# UIOX Device Tree Makefile
# Compiles all four .dts sources to .dtb binary blobs.
#
# Prerequisites:
#   apt  install device-tree-compiler   (Debian/Ubuntu)
#   brew install dtc                    (macOS)
#
# Usage:
#   make                — build all four .dtb files
#   make arm32          — one target
#   make check          — build + round-trip each blob through dtc
#   make inspect-arm64  — decompile a blob back to .dts for reading
#   make clean
# =============================================================================

DTC      ?= dtc
DTFLAGS  := -I dts -O dtb -W no-unit_address_vs_reg
DTCVER   := $(shell $(DTC) --version 2>/dev/null)

SRCS     := uiox-arm64.dts uiox-arm32.dts uiox-riscv64.dts uiox-x86_64.dts
DTBS     := $(SRCS:.dts=.dtb)
TARGETS  := $(SRCS:.dts=)

.PHONY: all check clean $(TARGETS)

all: $(DTBS)
	@echo ""
	@echo "=== UIOX DTB files built ==="
	@ls -lh $(DTBS)

$(TARGETS): %: uiox-%.dtb

%.dtb: %.dts
	$(DTC) $(DTFLAGS) -o $@ $<
	@echo "  Built $@"

check: $(DTBS)
	@for f in $(DTBS); do \
	    echo "--- Checking $$f ---"; \
	    $(DTC) -I dtb -O dts $$f > /dev/null && echo "  OK"; \
	done

inspect-%: uiox-%.dtb
	$(DTC) -I dtb -O dts -o uiox-$*.inspect.dts $<

clean:
	rm -f $(DTBS) *.inspect.dts
