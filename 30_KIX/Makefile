# =============================================================================
# 30_KIX/Makefile  —  UIOX Kernel Umbrella Makefile
#
# Orchestrates the build of all kernel subsystems and links the final
# kernel ELF for the selected architecture.
#
# Usage:
#   make arm64                   # build kernel for ARM64 (static BSP)
#   make arm32                   # build kernel for ARM32
#   make riscv64                 # build kernel for RISC-V 64
#   make x86_64                  # build kernel for x86-64
#   make all                     # build all four architectures
#
#   make arm64 LINK=yes          # also link final kernel ELF (requires BSP lib)
#   make arm64 BSP_LIB=<path>    # path to libbsp.a from 10_BSP build
#
#   make clean-arm64             # clean ARM64 outputs
#   make clean                   # clean all architectures
#   make info-arm64              # show toolchain for ARM64
#
# Build model:
#   Each subsystem (20..34) has its own Makefile that produces a static lib.
#   This umbrella Makefile invokes each sub-Makefile in dependency order,
#   then optionally links uiox_kernel_main.c with all subsystem libraries
#   and libbsp.a to produce the final kernel ELF.
#
# Subsystem build order (dependency order):
#   1. 20_DriverInterfaces          → lib/libuiox_hw.a
#   2. 30_DeviceDrivers             → lib/libuiox_dev.a
#   3. 31_BufferCache               → lib/libuiox_bcache.a
#   4. 32_FileSystem                → lib/libuiox_fs.a
#   5. 33_ProcessControlSubsystem   → lib/libuiox_pcs.a
#   6. 34_CAS                       → lib/libuiox_cas.a
#   7. uiox_kernel_main.c           → build/<arch>/uiox_kernel_main.o
#   8. Link all → build/<arch>/uiox_kernel.elf   (if LINK=yes)
#
# macOS prerequisites:
#   brew install aarch64-elf-gcc arm-none-eabi-gcc riscv64-elf-gcc x86_64-elf-gcc llvm
#
# Linux prerequisites:
#   sudo apt install gcc-aarch64-linux-gnu gcc-arm-linux-gnueabihf \\
#                    gcc-riscv64-linux-gnu gcc-x86-64-linux-gnu binutils-multiarch
#
# @version 1.1.0  (32/33/34 promoted from optional to required)
# @date    2026-07-23
# =============================================================================

LINK    ?= no
BSP_LIB ?=

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  LLVM_BIN  := $(shell brew --prefix llvm 2>/dev/null || echo /opt/homebrew/opt/llvm)/bin
  ARM64_CC  := aarch64-elf-gcc
  ARM64_AR  := aarch64-elf-ar
  ARM64_LD  := aarch64-elf-ld
  ARM64_OCP := $(LLVM_BIN)/llvm-objcopy
  ARM32_CC  := arm-none-eabi-gcc
  ARM32_AR  := arm-none-eabi-ar
  ARM32_LD  := arm-none-eabi-ld
  ARM32_OCP := $(LLVM_BIN)/llvm-objcopy
  X86_CC    := x86_64-elf-gcc
  X86_AR    := x86_64-elf-ar
  X86_LD    := x86_64-elf-ld
  X86_OCP   := $(LLVM_BIN)/llvm-objcopy
  RV64_CC   := riscv64-elf-gcc
  RV64_AR   := riscv64-elf-ar
  RV64_LD   := riscv64-elf-ld
  RV64_OCP  := $(LLVM_BIN)/llvm-objcopy
else
  ARM64_CC  := aarch64-linux-gnu-gcc
  ARM64_AR  := aarch64-linux-gnu-ar
  ARM64_LD  := aarch64-linux-gnu-ld
  ARM64_OCP := aarch64-linux-gnu-objcopy
  ARM32_CC  := arm-linux-gnueabihf-gcc
  ARM32_AR  := arm-linux-gnueabihf-ar
  ARM32_LD  := arm-linux-gnueabihf-ld
  ARM32_OCP := arm-linux-gnueabihf-objcopy
  X86_CC    := x86_64-linux-gnu-gcc
  X86_AR    := x86_64-linux-gnu-ar
  X86_LD    := x86_64-linux-gnu-ld
  X86_OCP   := x86_64-linux-gnu-objcopy
  RV64_CC   := riscv64-linux-gnu-gcc
  RV64_AR   := riscv64-linux-gnu-ar
  RV64_LD   := riscv64-linux-gnu-ld
  RV64_OCP  := riscv64-linux-gnu-objcopy
endif

KIX_DIR := $(CURDIR)
BSP_DIR := $(KIX_DIR)/../10_BSP

# ── All six subsystems are now REQUIRED — each has a Makefile ─────────────────
SUBSYS := \
    20_DriverInterfaces        \
    30_DeviceDrivers           \
    31_BufferCache             \
    32_FileSystem              \
    33_ProcessControlSubsystem \
    34_CAS

KERNEL_CFLAGS_BASE := \
    -std=c11              \
    -Wall -Wextra -Werror \
    -ffreestanding        \
    -fno-builtin          \
    -fno-stack-protector  \
    -nostdinc             \
    -nostdlib             \
    -Os                   \
    -g                    \
    -I$(KIX_DIR)/20_DriverInterfaces/include         \
    -I$(KIX_DIR)/30_DeviceDrivers/include            \
    -I$(KIX_DIR)/31_BufferCache/include              \
    -I$(KIX_DIR)/32_FileSystem/include               \
    -I$(KIX_DIR)/33_ProcessControlSubsystem/include  \
    -I$(KIX_DIR)/34_CAS/include                      \
    -I$(BSP_DIR)/include                              \
    -I$(BSP_DIR)/03_SoC/include                       \
    -I$(KIX_DIR)/../01_uBoot/include                  \
    -I$(KIX_DIR)/../02_FwHal/include                  \
    -I$(KIX_DIR)/../50_UIX/12_ksign/include

# ── Helper: build one subsystem ───────────────────────────────────────────────
define build_subsys
	@echo "[KIX] $(1) → $(5)..."
	+$(MAKE) -C $(KIX_DIR)/$(1) \
	    CC="$(2)" AR="$(3)" \
	    CFLAGS="-Wall -Wextra -O2 -std=c11 -Iinclude $(4)" \
	    static
endef

# =============================================================================
# Per-architecture named targets
# =============================================================================

.PHONY: arm64
arm64:
	@echo "" && echo "=== KIX arm64 ==="
	$(call build_subsys,20_DriverInterfaces,$(ARM64_CC),$(ARM64_AR),-march=armv8-a -DARCH_ARM64,arm64)
	$(call build_subsys,30_DeviceDrivers,$(ARM64_CC),$(ARM64_AR),-march=armv8-a -DARCH_ARM64,arm64)
	$(call build_subsys,31_BufferCache,$(ARM64_CC),$(ARM64_AR),-march=armv8-a -DARCH_ARM64,arm64)
	$(call build_subsys,32_FileSystem,$(ARM64_CC),$(ARM64_AR),-march=armv8-a -DARCH_ARM64,arm64)
	$(call build_subsys,33_ProcessControlSubsystem,$(ARM64_CC),$(ARM64_AR),-march=armv8-a -DARCH_ARM64,arm64)
	$(call build_subsys,34_CAS,$(ARM64_CC),$(ARM64_AR),-march=armv8-a -DARCH_ARM64,arm64)
	@mkdir -p $(KIX_DIR)/build/arm64
	$(ARM64_CC) $(KERNEL_CFLAGS_BASE) -march=armv8-a -mabi=lp64 -mlittle-endian -D__aarch64__ \
	    -I$(BSP_DIR)/10_Arch/arm64/include \
	    -c $(KIX_DIR)/uiox_kernel_main.c -o $(KIX_DIR)/build/arm64/uiox_kernel_main.o
	@echo "[KIX] arm64 done"
ifeq ($(LINK),yes)
	$(MAKE) _link_kernel _ARCH=arm64 _LD=$(ARM64_LD) _LD_EMUL=aarch64elf
endif

.PHONY: arm32
arm32:
	@echo "" && echo "=== KIX arm32 ==="
	$(call build_subsys,20_DriverInterfaces,$(ARM32_CC),$(ARM32_AR),-march=armv7-a -DARCH_ARM32,arm32)
	$(call build_subsys,30_DeviceDrivers,$(ARM32_CC),$(ARM32_AR),-march=armv7-a -DARCH_ARM32,arm32)
	$(call build_subsys,31_BufferCache,$(ARM32_CC),$(ARM32_AR),-march=armv7-a -DARCH_ARM32,arm32)
	$(call build_subsys,32_FileSystem,$(ARM32_CC),$(ARM32_AR),-march=armv7-a -DARCH_ARM32,arm32)
	$(call build_subsys,33_ProcessControlSubsystem,$(ARM32_CC),$(ARM32_AR),-march=armv7-a -DARCH_ARM32,arm32)
	$(call build_subsys,34_CAS,$(ARM32_CC),$(ARM32_AR),-march=armv7-a -DARCH_ARM32,arm32)
	@mkdir -p $(KIX_DIR)/build/arm32
	$(ARM32_CC) $(KERNEL_CFLAGS_BASE) -march=armv7-a -marm -mfloat-abi=soft -mlittle-endian -D__arm__ \
	    -I$(BSP_DIR)/10_Arch/arm32/include \
	    -c $(KIX_DIR)/uiox_kernel_main.c -o $(KIX_DIR)/build/arm32/uiox_kernel_main.o
	@echo "[KIX] arm32 done"
ifeq ($(LINK),yes)
	$(MAKE) _link_kernel _ARCH=arm32 _LD=$(ARM32_LD) _LD_EMUL=armelf
endif

.PHONY: riscv64
riscv64:
	@echo "" && echo "=== KIX riscv64 ==="
	$(call build_subsys,20_DriverInterfaces,$(RV64_CC),$(RV64_AR),-march=rv64imafdc -DARCH_RISCV64,riscv64)
	$(call build_subsys,30_DeviceDrivers,$(RV64_CC),$(RV64_AR),-march=rv64imafdc -DARCH_RISCV64,riscv64)
	$(call build_subsys,31_BufferCache,$(RV64_CC),$(RV64_AR),-march=rv64imafdc -DARCH_RISCV64,riscv64)
	$(call build_subsys,32_FileSystem,$(RV64_CC),$(RV64_AR),-march=rv64imafdc -DARCH_RISCV64,riscv64)
	$(call build_subsys,33_ProcessControlSubsystem,$(RV64_CC),$(RV64_AR),-march=rv64imafdc -DARCH_RISCV64,riscv64)
	$(call build_subsys,34_CAS,$(RV64_CC),$(RV64_AR),-march=rv64imafdc -DARCH_RISCV64,riscv64)
	@mkdir -p $(KIX_DIR)/build/riscv64
	$(RV64_CC) $(KERNEL_CFLAGS_BASE) -march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany \
	    -D__riscv -D__riscv_xlen=64 \
	    -I$(BSP_DIR)/10_Arch/riscv64/include \
	    -c $(KIX_DIR)/uiox_kernel_main.c -o $(KIX_DIR)/build/riscv64/uiox_kernel_main.o
	@echo "[KIX] riscv64 done"
ifeq ($(LINK),yes)
	$(MAKE) _link_kernel _ARCH=riscv64 _LD=$(RV64_LD) _LD_EMUL=elf64lriscv
endif

.PHONY: x86_64
x86_64:
	@echo "" && echo "=== KIX x86_64 ==="
	$(call build_subsys,20_DriverInterfaces,$(X86_CC),$(X86_AR),-m64 -DARCH_X86_64,x86_64)
	$(call build_subsys,30_DeviceDrivers,$(X86_CC),$(X86_AR),-m64 -DARCH_X86_64,x86_64)
	$(call build_subsys,31_BufferCache,$(X86_CC),$(X86_AR),-m64 -DARCH_X86_64,x86_64)
	$(call build_subsys,32_FileSystem,$(X86_CC),$(X86_AR),-m64 -DARCH_X86_64,x86_64)
	$(call build_subsys,33_ProcessControlSubsystem,$(X86_CC),$(X86_AR),-m64 -DARCH_X86_64,x86_64)
	$(call build_subsys,34_CAS,$(X86_CC),$(X86_AR),-m64 -DARCH_X86_64,x86_64)
	@mkdir -p $(KIX_DIR)/build/x86_64
	$(X86_CC) $(KERNEL_CFLAGS_BASE) -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
	    -fno-pic -mcmodel=kernel -D__x86_64__ \
	    -I$(BSP_DIR)/10_Arch/x86_64/include \
	    -c $(KIX_DIR)/uiox_kernel_main.c -o $(KIX_DIR)/build/x86_64/uiox_kernel_main.o
	@echo "[KIX] x86_64 done"
ifeq ($(LINK),yes)
	$(MAKE) _link_kernel _ARCH=x86_64 _LD=$(X86_LD) _LD_EMUL=elf_x86_64
endif

# ── Link step (LINK=yes) ──────────────────────────────────────────────────────
.PHONY: _link_kernel
_link_kernel:
	@echo "[KIX] Linking build/$(_ARCH)/uiox_kernel.elf..."
	$(_LD) -m $(_LD_EMUL) \
	    -T $(KIX_DIR)/linker/kernel_$(_ARCH).ld \
	    -Map=$(KIX_DIR)/build/$(_ARCH)/uiox_kernel.map \
	    --gc-sections \
	    $(KIX_DIR)/build/$(_ARCH)/uiox_kernel_main.o \
	    $(wildcard $(KIX_DIR)/20_DriverInterfaces/lib/libuiox_hw.a)     \
	    $(wildcard $(KIX_DIR)/30_DeviceDrivers/lib/libuiox_dev.a)        \
	    $(wildcard $(KIX_DIR)/31_BufferCache/lib/libuiox_bcache.a)       \
	    $(wildcard $(KIX_DIR)/32_FileSystem/lib/libuiox_fs.a)            \
	    $(wildcard $(KIX_DIR)/33_ProcessControlSubsystem/lib/libuiox_pcs.a) \
	    $(wildcard $(KIX_DIR)/34_CAS/lib/libuiox_cas.a)                  \
	    $(BSP_LIB) \
	    -o $(KIX_DIR)/build/$(_ARCH)/uiox_kernel.elf
	@echo "[KIX] Kernel ELF → build/$(_ARCH)/uiox_kernel.elf"

# ── Convenience targets ───────────────────────────────────────────────────────
.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[KIX] all builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; rm -rf $(KIX_DIR)/build/arm64
clean-arm32:   ; rm -rf $(KIX_DIR)/build/arm32
clean-riscv64: ; rm -rf $(KIX_DIR)/build/riscv64
clean-x86_64:  ; rm -rf $(KIX_DIR)/build/x86_64

.PHONY: clean-subsys
clean-subsys:
	@for d in $(SUBSYS); do \
	    echo "[CLN] $$d"; $(MAKE) -C $(KIX_DIR)/$$d clean; \
	done

.PHONY: clean
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean-subsys
	@echo "[CLN] all kernel build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:
	@echo "HOST=$(UNAME_S)  ARCH=arm64    CC=$(ARM64_CC)  LINK=$(LINK)  BSP_LIB=$(BSP_LIB)"
info-arm32:
	@echo "HOST=$(UNAME_S)  ARCH=arm32    CC=$(ARM32_CC)  LINK=$(LINK)  BSP_LIB=$(BSP_LIB)"
info-riscv64:
	@echo "HOST=$(UNAME_S)  ARCH=riscv64  CC=$(RV64_CC)   LINK=$(LINK)  BSP_LIB=$(BSP_LIB)"
info-x86_64:
	@echo "HOST=$(UNAME_S)  ARCH=x86_64   CC=$(X86_CC)    LINK=$(LINK)  BSP_LIB=$(BSP_LIB)"
