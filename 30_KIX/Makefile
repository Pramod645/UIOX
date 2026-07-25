# =============================================================================
# 30_KIX/Makefile  — v2.1.0
#
# UIOX Kernel Umbrella Makefile (Linux + macOS).
#
# Build chain (dependency order, bottom-up hardware → top-level services):
#   1. 02_FwHal              (repo root sibling)  → firmware HAL
#   2. 20_DriverInterfaces                         → hardware abstraction
#   3. 30_DeviceDrivers                            → kernel device drivers
#   4. 31_BufferCache                              → buffer cache
#   5. 32_FS                                       → file system
#   6. 33_PCS                                      → process control
#   7. 34_CAS                                      → connected app services
#   8. uiox_kernel_main.c                          → kernel entry point
#
# Usage:
#   make arm64 / arm32 / riscv64 / x86_64
#   make all
#   make arm64 LINK=yes BSP_LIB=<path>   # link final kernel ELF
#   make clean[-arm64|-arm32|-riscv64|-x86_64]
#   make info[-arm64|-arm32|-riscv64|-x86_64]
#
# @version 2.1.0  @date 2026-07-25
# =============================================================================

MFDIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(MFDIR)common
LINK   ?= no
BSP_LIB ?=

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LLVM_BIN := $(shell \
    if command -v llvm-objcopy >/dev/null 2>&1; then \
        dirname $(command -v llvm-objcopy); \
    else \
        echo $(brew --prefix llvm 2>/dev/null || echo /opt/homebrew/opt/llvm)/bin; \
    fi)
  ARM64_CC := aarch64-elf-gcc
  ARM64_AR := aarch64-elf-ar
  ARM64_LD := aarch64-elf-ld
  ARM32_CC := arm-none-eabi-gcc
  ARM32_AR := arm-none-eabi-ar
  ARM32_LD := arm-none-eabi-ld
  RV64_CC  := riscv64-elf-gcc
  RV64_AR  := riscv64-elf-ar
  RV64_LD  := riscv64-elf-ld
  X86_CC   := x86_64-elf-gcc
  X86_AR   := x86_64-elf-ar
  X86_LD   := x86_64-elf-ld
else
  ARM64_CC := aarch64-linux-gnu-gcc
  ARM64_AR := aarch64-linux-gnu-ar
  ARM64_LD := aarch64-linux-gnu-ld
  ARM32_CC := arm-linux-gnueabihf-gcc
  ARM32_AR := arm-linux-gnueabihf-ar
  ARM32_LD := arm-linux-gnueabihf-ld
  RV64_CC  := riscv64-linux-gnu-gcc
  RV64_AR  := riscv64-linux-gnu-ar
  RV64_LD  := riscv64-linux-gnu-ld
  X86_CC   := x86_64-linux-gnu-gcc
  X86_AR   := x86_64-linux-gnu-ar
  X86_LD   := x86_64-linux-gnu-ld
endif

# ── Path roots ────────────────────────────────────────────────────────────────
BSPDIR ?= $(abspath $(MFDIR)../10_BSP)
FWHAL  := $(abspath $(MFDIR)../02_FwHal)
SOCINC  = $(BSPDIR)/03_SoC/include

# ── kernel_main compiler flags ────────────────────────────────────────────────
KERNEL_CFLAGS_BASE := \
  -I$(COMMONDIR)                         \
  -std=c11 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector \
  -ffunction-sections -fdata-sections -nostdinc -Os -g \
  -I$(FWHAL)/include                         \
  -I$(MFDIR)32_FS/include                    \
  -I$(MFDIR)33_PCS/include                   \
  -I$(SOCINC)                                \
  -I$(MFDIR)../01_uBoot/include              \
  -I$(MFDIR)../50_UIX/12_ksign/include

# =============================================================================
# Per-architecture named targets
# =============================================================================

.PHONY: arm64
arm64: _do_arm64
	@echo "[KIX] arm64 done → build/arm64/"

.PHONY: _do_arm64
_do_arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all            \
	    _ARCH=arm64 CC=$(ARM64_CC) AR=$(ARM64_AR) LD=$(ARM64_LD) \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian"  \
	    ARCH_CPUDEF="-D__aarch64__" ARCH_LD_EMUL=aarch64elf

.PHONY: arm32
arm32: _do_arm32
	@echo "[KIX] arm32 done → build/arm32/"

.PHONY: _do_arm32
_do_arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all                 \
	    _ARCH=arm32 CC=$(ARM32_CC) AR=$(ARM32_AR) LD=$(ARM32_LD) \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_CPUDEF="-D__arm__" ARCH_LD_EMUL=armelf

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[KIX] riscv64 done → build/riscv64/"

.PHONY: _do_riscv64
_do_riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all                  \
	    _ARCH=riscv64 CC=$(RV64_CC) AR=$(RV64_AR) LD=$(RV64_LD) \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64" ARCH_LD_EMUL=elf64lriscv

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[KIX] x86_64 done → build/x86_64/"

.PHONY: _do_x86_64
_do_x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all                   \
	    _ARCH=x86_64 CC=$(X86_CC) AR=$(X86_AR) LD=$(X86_LD)  \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__" ARCH_LD_EMUL=elf_x86_64

# =============================================================================
# _build_all: invoke each subsystem in dependency order
# =============================================================================
_BUILD_DIR = $(MFDIR)build/$(_ARCH)
_ARCH_INC  = $(BSPDIR)/10_Arch/$(_ARCH)/include

.PHONY: _build_all
_build_all: _build_subsystems _build_kernel_main
ifeq ($(LINK),yes)
_build_all: _link_kernel
endif

.PHONY: _build_subsystems
_build_subsystems:
	@echo "[KIX] building subsystems for $(_ARCH)..."
	# 1. 02_FwHal — firmware HAL (repo root sibling)
	+$(MAKE) -C $(FWHAL) CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH) 2>/dev/null || echo "[KIX] 02_FwHal: no Makefile or skipped"
	# 2. 20_DriverInterfaces — hardware abstraction
	+$(MAKE) -C $(MFDIR)20_DriverInterfaces CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH)
	# 3. 30_DeviceDrivers — kernel device drivers
	+$(MAKE) -C $(MFDIR)30_DeviceDrivers CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH)
	# 4. 31_BufferCache — buffer cache
	+$(MAKE) -C $(MFDIR)31_BufferCache CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH)
	# 5. 32_FS — file system
	+$(MAKE) -C $(MFDIR)32_FS CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH)
	# 6. 33_PCS — process control subsystem
	+$(MAKE) -C $(MFDIR)33_PCS CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH)
	# 7. 34_CAS — connected application services
	+$(MAKE) -C $(MFDIR)34_CAS CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH) 2>/dev/null || echo "[KIX] 34_CAS: skipped"

.PHONY: _build_kernel_main
_build_kernel_main:
	@mkdir -p $(_BUILD_DIR)
	$(CC) $(KERNEL_CFLAGS_BASE) $(ARCH_CFLAGS) $(ARCH_CPUDEF) \
	    -I$(_ARCH_INC) \
	    -c $(MFDIR)uiox_kernel_main.c \
	    -o $(_BUILD_DIR)/uiox_kernel_main.o
	@echo "  [CC]  uiox_kernel_main.c"

.PHONY: _link_kernel
_link_kernel:
	@echo "[KIX] Linking $(_BUILD_DIR)/uiox_kernel.elf..."
	$(LD) -m $(ARCH_LD_EMUL) \
	    -T $(MFDIR)linker/kernel_$(_ARCH).ld \
	    -Map=$(_BUILD_DIR)/uiox_kernel.map \
	    --gc-sections \
	    $(_BUILD_DIR)/uiox_kernel_main.o \
	    $(wildcard $(FWHAL)/build/$(_ARCH)/*.a)              \
	    $(wildcard $(MFDIR)20_DriverInterfaces/build/$(_ARCH)/*.a) \
	    $(wildcard $(MFDIR)30_DeviceDrivers/build/$(_ARCH)/*.a)    \
	    $(wildcard $(MFDIR)31_BufferCache/build/$(_ARCH)/*.a)      \
	    $(wildcard $(MFDIR)32_FS/build/$(_ARCH)/*.a)               \
	    $(wildcard $(MFDIR)33_PCS/build/$(_ARCH)/*.a)              \
	    $(wildcard $(MFDIR)34_CAS/build/$(_ARCH)/*.a)              \
	    $(BSP_LIB) \
	    -o $(_BUILD_DIR)/uiox_kernel.elf
	@echo "[KIX] Kernel ELF → $(_BUILD_DIR)/uiox_kernel.elf"

# =============================================================================
# Convenience targets
# =============================================================================

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[KIX] all builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; rm -rf $(MFDIR)build/arm64
clean-arm32:   ; rm -rf $(MFDIR)build/arm32
clean-riscv64: ; rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; rm -rf $(MFDIR)build/x86_64

.PHONY: clean-subsys
clean-subsys:
	@for d in $(FWHAL) $(MFDIR)20_DriverInterfaces $(MFDIR)30_DeviceDrivers \
	           $(MFDIR)31_BufferCache $(MFDIR)32_FS $(MFDIR)33_PCS $(MFDIR)34_CAS; do \
	    [ -f $$d/Makefile ] && $(MAKE) -C $$d clean || true; \
	done

.PHONY: clean
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean-subsys
	@echo "[CLN] all kernel build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:
	@echo "HOST=$(UNAME_S)  ARCH=arm64    CC=$(ARM64_CC)  LINK=$(LINK)"
info-arm32:
	@echo "HOST=$(UNAME_S)  ARCH=arm32    CC=$(ARM32_CC)  LINK=$(LINK)"
info-riscv64:
	@echo "HOST=$(UNAME_S)  ARCH=riscv64  CC=$(RV64_CC)   LINK=$(LINK)"
info-x86_64:
	@echo "HOST=$(UNAME_S)  ARCH=x86_64   CC=$(X86_CC)    LINK=$(LINK)"
