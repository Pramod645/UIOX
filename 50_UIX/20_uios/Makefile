# =============================================================================
# 50_UIX/20_uios/Makefile — the UIOX operating system supervisor
#
# OUTPUT is a USER binary, not a kernel object.  UIOS is exec'd by PID 1
# and runs at EL0 from then on.  It reaches the kernel through the
# 40_SCIX trap layer; no kernel object goes on this link line.
#
# SYS_CALL_ENBLE_DISABLE=0 — STUB build.  uix_sys.h's inline sys_*()
# wrappers return -1 without calling anything, and the #if block that
# pulls kernel headers is skipped.  This builds and links; it does not
# run.  The trap form is the later fix.
#
# No crt0 in a freestanding image: -nostdlib/-nostartfiles stop the
# toolchain asking for crt0.o, -lg and -lc; -Wl,-e,main names the entry
# the kernel jumps to.
#
# install — stages the layout kinit_main execs: <IMAGE_ROOT>/bin/uios.
# Plain files, not a volume.  Getting them onto the device is a separate
# step (the UNFS format).
# =============================================================================

MFDIR       := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE   = $(MFDIR)build
CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
_ARCH       ?= arm64
_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

CFLAGS := -std=c99 -Wall -Wextra \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -nostdinc -Os -g \
          $(ARCH_CFLAGS) $(ARCH_CPUDEF) \
          -I$(MFDIR)../../40_SCIX \
          -I$(MFDIR)../../30_KIX/common \
          -I$(MFDIR)../00_libs/00_uixlibs \
          -I$(MFDIR)../00_libs/00_uixlibs/PoStd \
          -DSYS_CALL_ENBLE_DISABLE=0

LDFLAGS := -nostdlib -nostartfiles -ffreestanding -Wl,-e,main

TARGET := $(_BUILD_DIR)/uios

# One image tree per arch, beside the binaries it copies from — so
# `make clean` sweeps it and nothing is left outside build/.
IMAGE_ROOT := $(BUILD_BASE)/$(_ARCH)/image

# ── THE ARCH TARGETS DO THE INSTALL THEMSELVES ────────────────────────
# The per-arch recipe runs in the PARENT make, where `_ARCH` is still the
# default `arm64` — the target name `arm32` is a target, not a variable.
# Passing `_ARCH=$@` to the recursive _build fixed the BUILD path but not
# the recipe body, so `make arm32 install` installed the arm64 binary into
# the arm64 tree.
#
# Fixing it by hand (`make _ARCH=arm32 install`) works but is easy to
# forget.  Instead the arch target does the install in the SUB-make, where
# _ARCH is already correct:
#
#     make -C 50_UIX/20_uios arm64      → builds AND installs arm64
#
# `_build_install` is an internal target, not the user-facing `install`
# (which is kept below, so `make install` still behaves as before).
.PHONY: arm64 arm32 riscv64 x86_64
arm64 arm32 riscv64 x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build_install   \
	    _ARCH=$@                                 \
	    CC="$(CC)" AR="$(AR)"                    \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)"             \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)"

.PHONY: _build
_build: $(TARGET)
	@echo "[UIOS] $(_ARCH) → $(TARGET)"

.PHONY: _build_install
_build_install: $(TARGET)
	@echo "[UIOS] $(_ARCH) → $(TARGET)"
	@mkdir -p $(IMAGE_ROOT)/bin
	@cp $(TARGET) $(IMAGE_ROOT)/bin/uios
	@echo "[UIOS] installed → $(IMAGE_ROOT)/bin/uios"

$(TARGET): uix_uios.c uios_apps.c uix_uios.h
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ uix_uios.c uios_apps.c

# ── install ───────────────────────────────────────────────────────────
# /bin/uios is the path kinit_main execs (uiox_pcs_init.c).  Nothing else
# stages this layout, so it is the one place the runtime path is built.
#
# THIS ONE STILL NEEDS `_ARCH` ON THE COMMAND LINE when invoked directly:
#     make _ARCH=arm32 install
# because it is a top-level target, not one of the arch targets above.
# The arch targets do not use it.
.PHONY: install
install: $(TARGET)
	@mkdir -p $(IMAGE_ROOT)/bin
	@cp $(TARGET) $(IMAGE_ROOT)/bin/uios
	@echo "[UIOS] installed → $(IMAGE_ROOT)/bin/uios"

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	@rm -rf $(BUILD_BASE)
	@echo "[CLN] 20_uios build outputs removed"
