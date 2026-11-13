# =============================================================================
# 50_UIX/20_uios/Makefile — the UIOX operating system supervisor
#
# OUTPUT is a USER binary, not a kernel object.  UIOS is exec'd by PID 1
# and runs at EL0 from then on.  It reaches the kernel through the
# 40_SCIX trap layer; no kernel object goes on this link line.
#
# No crt0 in a freestanding image: -nostdlib/-nostartfiles stop the
# toolchain asking for crt0.o, -lg and -lc; -Wl,-e,main names the entry
# the kernel jumps to.
#
# install — stages the layout kinit_main execs: <IMAGE_ROOT>/bin/uios.
#
# ── v2.0.0: CROSS TOOLCHAIN + THE LIBRARY ON THE LINK LINE ───────────
#   Two failures, both visible in one recipe line:
#
#       cc … -o $@ uix_uios.c uios_apps.c libUIXlib.a
#                                        libclang_rt.osx.a
#
#   · CC was "cc" — Apple clang.  It emits Mach-O and resolves
#     $(CC) -print-libgcc-file-name to the DARWIN runtime, so the ELF
#     archive could never link.  Same failure as 21_apps and 01_cli
#     before their Makefiles got the cross toolchain.
#
#   · Before that, there was no archive on the line at all: uix_uios.c
#     calls sys_fork, sys_wait4, sys_kill, sys_sched_setscheduler,
#     sys_sched_yield, sys_execv and sys_write, every one of which
#     resolves through libUIXlib.a.
#
#   LIBS carries the per-arch archive and the compiler runtime.  The
#   sources come FIRST: ld walks archives in command order, so a library
#   ahead of the objects it serves contributes nothing.
# =============================================================================

MFDIR       := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE   = $(MFDIR)build
_ARCH       ?= arm64
_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

# ── toolchain — cross, mirroring 21_apps and 01_cli ──────────────────
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC := aarch64-elf-gcc
  ARM32_CC := arm-none-eabi-gcc
  RV64_CC  := riscv64-elf-gcc
  X86_CC   := x86_64-elf-gcc
else
  ARM64_CC := aarch64-linux-gnu-gcc
  ARM32_CC := arm-linux-gnueabihf-gcc
  RV64_CC  := riscv64-linux-gnu-gcc
  X86_CC   := x86_64-linux-gnu-gcc
endif

# ── arch flag sets — identical to 00_uixlibs, 01_cli and 21_apps ─────
ARM64_FLAGS   := -march=armv8-a -mabi=lp64 -mlittle-endian
ARM32_FLAGS   := -march=armv7-a -marm -mfloat-abi=soft -mlittle-endian
RV64_FLAGS    := -march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany
X86_FLAGS     := -m64 -mno-red-zone -mno-mmx -mcmodel=kernel

ARM64_CPUDEF  := -D__aarch64__
ARM32_CPUDEF  := -D__arm__
RV64_CPUDEF   := -D__riscv -D__riscv_xlen=64
X86_CPUDEF    := -D__x86_64__

UIXLIB      := $(MFDIR)../00_libs/00_uixlibs

CFLAGS := -std=c99 -Wall -Wextra \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -nostdinc -Os -g \
          $(ARCH_CFLAGS) $(ARCH_CPUDEF) \
          -I$(MFDIR)../../40_SCIX \
          -I$(MFDIR)../../30_KIX/common \
          -I$(UIXLIB) \
          -I$(UIXLIB)/PoStd \
          -I$(UIXLIB)/sys

LDFLAGS := -nostdlib -nostartfiles -ffreestanding -Wl,-e,main

# `=` not `:=` — $(CC) is set by the per-arch sub-make, so the $(shell …)
# must not run until the recipe expands.  With `:=` it evaluates in the
# PARENT where CC is unset and returns the HOST runtime.
UIXLIB_AR := $(UIXLIB)/build/$(_ARCH)/libUIXlib.a
LIBS       = $(UIXLIB_AR) $(shell $(CC) -print-libgcc-file-name)

TARGET := $(_BUILD_DIR)/uios

IMAGE_ROOT := $(BUILD_BASE)/$(_ARCH)/image

# ── THE ARCH TARGETS DO THE INSTALL THEMSELVES ────────────────────────
# The per-arch recipe runs in the PARENT make, where `_ARCH` is still the
# default `arm64` — the target name `arm32` is a target, not a variable.
# The arch target does the install in the SUB-make, where _ARCH is
# already correct:
#
#     make -C 50_UIX/20_uios arm64      → builds AND installs arm64
#
# CC is passed per arch and checked: if aarch64-elf-gcc is absent, cc
# would produce Mach-O and link the macOS runtime again.
.PHONY: arm64
arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=arm64 CC="$(ARM64_CC)" \
	    ARCH_CFLAGS="$(ARM64_FLAGS)" ARCH_CPUDEF="$(ARM64_CPUDEF)"

.PHONY: arm32
arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=arm32 CC="$(ARM32_CC)" \
	    ARCH_CFLAGS="$(ARM32_FLAGS)" ARCH_CPUDEF="$(ARM32_CPUDEF)"

.PHONY: riscv64
riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=riscv64 CC="$(RV64_CC)" \
	    ARCH_CFLAGS="$(RV64_FLAGS)" ARCH_CPUDEF="$(RV64_CPUDEF)"

.PHONY: x86_64
x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=x86_64 CC="$(X86_CC)" \
	    ARCH_CFLAGS="$(X86_FLAGS)" ARCH_CPUDEF="$(X86_CPUDEF)"

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "[UIOS] all four arches built and staged"

.PHONY: _build
_build: $(TARGET)
	@echo "[UIOS] $(_ARCH) → $(TARGET)"

.PHONY: _build_install
_build_install: $(TARGET)
	@echo "[UIOS] $(_ARCH) → $(TARGET)"
	@mkdir -p $(IMAGE_ROOT)/bin
	@cp $(TARGET) $(IMAGE_ROOT)/bin/uios
	@echo "[UIOS] installed → $(IMAGE_ROOT)/bin/uios"

# Sources FIRST, libraries AFTER.
$(TARGET): uix_uios.c uios_apps.c uix_uios.h
	@mkdir -p $(dir $@)
	@echo "  [CC]  $(_ARCH)  uix_uios.c + uios_apps.c + libUIXlib.a"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ uix_uios.c uios_apps.c $(LIBS)

# ── install ───────────────────────────────────────────────────────────
# /bin/uios is the path kinit_main execs (uiox_pcs_init.c).  Needs `_ARCH`
# on the command line when invoked directly:
#     make _ARCH=arm32 install
.PHONY: install
install: $(TARGET)
	@mkdir -p $(IMAGE_ROOT)/bin
	@cp $(TARGET) $(IMAGE_ROOT)/bin/uios
	@echo "[UIOS] installed → $(IMAGE_ROOT)/bin/uios"

# ── check-binary ──────────────────────────────────────────────────────
.PHONY: check-binary
check-binary:
	@b=$(TARGET); \
	[ -f $$b ] || { echo "[UIOS] no $$b — build first"; exit 1; }; \
	echo "undefined in uios [$(_ARCH)]:"; \
	nm -u $$b 2>/dev/null | sed 's/^ *U //' | sort -u | sed 's/^/  /' \
	  || echo "  (none)"

.PHONY: info
info:
	@echo "HOST=$(UNAME_S)"
	@echo "  UIXLIB_AR = $(UIXLIB_AR)"
	@echo "  ARM64_CC  = $(ARM64_CC)"

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	@rm -rf $(BUILD_BASE)
	@echo "[CLN] 20_uios build outputs removed"
