# =============================================================================
#  UIOX -- 03_SoC/Makefile
#  SoC Platform Layer  (silicon bring-up, PSCI, secure-boot, POST, memory)
#  Source : [github.com](https://github.com/Pramod645/UIOX/tree/main/03_SoC)
#
#  Targets : arm64 | arm32 | x86_64 | riscv64 | all | clean
#
#  Source files in $(SRCDIR)/
#
#  -- Common (compiled into every arch target) --------------------------
#   uiox_soc_clock.c          Clock / PLL registry (stateless + stateful)
#   uiox_soc_irq.c            IRQ manager
#   uiox_soc_mem.c            Memory map + early MMU + no-libc helpers
#   uiox_soc_power.c          Power management (PSCI / ACPI)
#   uiox_soc_post.c           Power-On Self Test
#   uiox_soc_psci.c           PSCI 1.1 dispatch table
#   uiox_soc_secboot.c        Secure boot + SHA-256 + PCR
#   uiox_soc_tz.c             TrustZone / EL3 setup
#   uiox_soc_dma.c            DMA controller (SW-fallback)
#   uiox_soc_hw.c             HW HAL vtable registration + wrappers
#   uiox_soc_main.c           9-stage SoC init pipeline
#
#  -- Architecture-specific (one backend per target) --------------------
#   uiox_soc_arm64.c          ARM64 generic backend (BCM2711/BCM2712/QEMU)
#   uiox_soc_arm32.c          ARM32 generic backend (BCM2836/IMX6Q/QEMU)
#   uiox_soc_x86.c            x86-64 backend (Q35/i440FX/generic)
#   uiox_soc_riscv64.c        RISC-V 64 backend (SiFive U74/QEMU virt)
#
#  -- SoC-specific backends (linked into the matching arch target) ------
#   uiox_soc_imx8mp.c         NXP i.MX 8M Plus  (ARM64)
#   uiox_soc_rk3588.c         Rockchip RK3588    (ARM64)
#   uiox_soc_omap4430.c       TI OMAP4430        (ARM32)
#   uiox_soc_th1520.c         T-Head TH1520      (RISC-V 64)
#
#  Header files in $(INCDIR)/
#   uiox_basetypes.h           Custom bare-metal base types (no libc)
#   uiox_soc_types.h           SoC ID enum, capability flags, descriptor
#   uiox_soc.h                 Master umbrella include
#   uiox_soc_hw.h              Platform HW vtable (18-op)
#   uiox_soc_map.h             MMIO base addresses + IRQ numbers
#   uiox_soc_clk.h             Unified clock tree + PLL + baud helpers
#   uiox_soc_pm.h              Power domains + reset controller
#   uiox_soc_clock.h           High-level clock enable/disable/set-Hz
#   uiox_soc_irq.h             IRQ manager API
#   uiox_soc_mem.h             Physical memory map + MMU early init
#   uiox_soc_power.h           Power management API
#   uiox_soc_post.h            POST API
#   uiox_soc_psci.h            PSCI 1.1 API
#   uiox_soc_secboot.h         Secure boot API + SHA-256
#   uiox_soc_tz.h              TrustZone / EL3 API
#   uiox_soc_dma.h             DMA controller API
#   uiox_soc_pcie.h            PCIe ECAM API
#
#  Each target produces a static library:
#    build/arm64/libuiox_soc_arm64.a
#    build/arm32/libuiox_soc_arm32.a
#    build/x86_64/libuiox_soc_x86_64.a
#    build/riscv64/libuiox_soc_riscv64.a
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 1
VERSION_PATCH := 0

# -----------------------------------------------------------------------------
# Verbosity  (make V=1 <target>  for full command echo)
# -----------------------------------------------------------------------------
V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC      $<"
  LOGAR    := @echo "  AR      $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q        :=
  LOGCC    :=
  LOGAR    :=
  LOGCLEAN :=
endif

SRCDIR   := src
INCDIR   := include
BUILDDIR := build

# =============================================================================
# Cross-toolchain selection
# =============================================================================
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC  := aarch64-elf-gcc
  ARM32_CC  := arm-none-eabi-gcc
  X86_CC    := x86_64-elf-gcc
  RV64_CC   := riscv64-elf-gcc
else
  ARM64_CC  := aarch64-linux-gnu-gcc
  ARM32_CC  := arm-linux-gnueabihf-gcc
  X86_CC    := x86_64-linux-gnu-gcc
  RV64_CC   := riscv64-linux-gnu-gcc
endif

AR := ar

# =============================================================================
# Common compiler flags
# =============================================================================
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

CFLAGS_COMMON := \
  -std=c11                       \
  -Wall                          \
  -Wextra                        \
  -Wpedantic                     \
  -Wmissing-prototypes           \
  -Wstrict-prototypes            \
  -ffreestanding                 \
  -fno-builtin                   \
  -fno-stack-protector           \
  -ffunction-sections            \
  -fdata-sections                \
  -Os                            \
  -g                             \
  -I$(INCDIR)                    \
  -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
  -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
  -DUIOX_VERSION_PATCH=$(VERSION_PATCH) \
  -DUIOX_BASETYPES_COMPAT

# =============================================================================
# SOC_COMMON_SRCS -- 11 files compiled into every arch target
# =============================================================================
SOC_COMMON_SRCS := \
  $(SRCDIR)/uiox_soc_stdio.c    \
  $(SRCDIR)/uiox_soc_clk.c      \
  $(SRCDIR)/uiox_soc_irq.c      \
  $(SRCDIR)/uiox_soc_mem.c      \
  $(SRCDIR)/uiox_soc_power.c    \
  $(SRCDIR)/uiox_soc_post.c     \
   $(SRCDIR)/uiox_soc_pcie.c    \
  $(SRCDIR)/uiox_soc_psci.c     \
  $(SRCDIR)/uiox_soc_secboot.c  \
  $(SRCDIR)/uiox_soc_tz.c       \
  $(SRCDIR)/uiox_soc_dma.c      \
  $(SRCDIR)/uiox_soc_hw.c       \
  $(SRCDIR)/uiox_soc_main.c

# =============================================================================
# ARM64
# Backends: generic arm64 + IMX8MP + RK3588
# =============================================================================
ARM64_CFLAGS := $(CFLAGS_COMMON) \
  -march=armv8-a                 \
  -mstrict-align                 \
  -DARCH_ARM64

ARM64_SRCS := \
  $(SOC_COMMON_SRCS)             \
  $(SRCDIR)/uiox_soc_arm64.c

ARM64_OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(ARM64_SRCS))
ARM64_LIB  := $(BUILDDIR)/arm64/libuiox_soc_arm64.a

.PHONY: arm64
arm64: $(ARM64_LIB)
	$(Q)echo "  OK  SoC ARM64 => $(ARM64_LIB)"

$(ARM64_LIB): $(ARM64_OBJS) | $(BUILDDIR)/arm64
	$(LOGAR)
	$(Q)$(AR) rcs $@ $^

$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64:
	$(Q)mkdir -p $@

# =============================================================================
# ARM32
# Backends: generic arm32 + OMAP4430
# =============================================================================
ARM32_CFLAGS := $(CFLAGS_COMMON) \
  -march=armv7-a             \
  -mthumb                        \
  -mfloat-abi=soft               \
  -DARCH_ARM32

ARM32_SRCS := \
  $(SOC_COMMON_SRCS)             \
  $(SRCDIR)/uiox_soc_arm32.c    \
  $(SRCDIR)/uiox_soc_omap4430.c

ARM32_OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(ARM32_SRCS))
ARM32_LIB  := $(BUILDDIR)/arm32/libuiox_soc_arm32.a

.PHONY: arm32
arm32: $(ARM32_LIB)
	$(Q)echo "  OK  SoC ARM32 => $(ARM32_LIB)"

$(ARM32_LIB): $(ARM32_OBJS) | $(BUILDDIR)/arm32
	$(LOGAR)
	$(Q)$(AR) rcs $@ $^

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32:
	$(Q)mkdir -p $@

# =============================================================================
# x86_64
# Backend: single file handles Q35, i440FX, and generic
# =============================================================================
X86_CFLAGS := $(CFLAGS_COMMON) \
  -m64                         \
  -mno-red-zone                \
  -mno-mmx                     \
  -mno-sse                     \
  -mno-sse2                    \
  -DARCH_X86_64

X86_SRCS := \
  $(SOC_COMMON_SRCS)           \
  $(SRCDIR)/uiox_soc_x86.c

X86_OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(X86_SRCS))
X86_LIB  := $(BUILDDIR)/x86_64/libuiox_soc_x86_64.a

.PHONY: x86_64
x86_64: $(X86_LIB)
	$(Q)echo "  OK  SoC x86_64 => $(X86_LIB)"

$(X86_LIB): $(X86_OBJS) | $(BUILDDIR)/x86_64
	$(LOGAR)
	$(Q)$(AR) rcs $@ $^

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64:
	$(Q)mkdir -p $@

# =============================================================================
# RISC-V 64  (rv64imac, medany model)
# Backends: generic riscv64 + TH1520
# =============================================================================
RV64_CFLAGS := $(CFLAGS_COMMON) \
  -march=rv64imac_zicsr               \
  -mabi=lp64                    \
  -mcmodel=medany               \
  -DARCH_RISCV64

RV64_SRCS := \
  $(SOC_COMMON_SRCS)            \
  $(SRCDIR)/uiox_soc_riscv64.c

RV64_OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/riscv64/%.o,$(RV64_SRCS))
RV64_LIB  := $(BUILDDIR)/riscv64/libuiox_soc_riscv64.a

.PHONY: riscv64
riscv64: $(RV64_LIB)
	$(Q)echo "  OK  SoC RISC-V64 => $(RV64_LIB)"

$(RV64_LIB): $(RV64_OBJS) | $(BUILDDIR)/riscv64
	$(LOGAR)
	$(Q)$(AR) rcs $@ $^

$(BUILDDIR)/riscv64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/riscv64
	$(LOGCC)
	$(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64:
	$(Q)mkdir -p $@

# =============================================================================
# Auto-generated dependency includes
# =============================================================================
-include $(ARM64_OBJS:.o=.d)
-include $(ARM32_OBJS:.o=.d)
-include $(X86_OBJS:.o=.d)
-include $(RV64_OBJS:.o=.d)

# =============================================================================
# Top-level convenience targets
# =============================================================================
.PHONY: all
all: arm64 arm32 x86_64 riscv64

.PHONY: clean
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

.PHONY: help
help:
	$(Q)echo "UIOX SoC Platform Layer -- available targets:"
	$(Q)echo "  make arm64          build ARM64 static library"
	$(Q)echo "                        (arm64 + imx8mp + rk3588 backends)"
	$(Q)echo "  make arm32          build ARM32 static library"
	$(Q)echo "                        (arm32 + omap4430 backends)"
	$(Q)echo "  make x86_64         build x86-64 static library"
	$(Q)echo "                        (q35 + i440fx + generic backends)"
	$(Q)echo "  make riscv64        build RISC-V 64 static library"
	$(Q)echo "                        (qemu-virt + sifive-u74 + th1520)"
	$(Q)echo "  make all            build all four targets"
	$(Q)echo "  make clean          remove build/ directory"
	$(Q)echo "  make V=1 <target>   verbose build output"
	$(Q)echo ""
	$(Q)echo "Output libraries:"
	$(Q)echo "  build/arm64/libuiox_soc_arm64.a"
	$(Q)echo "  build/arm32/libuiox_soc_arm32.a"
	$(Q)echo "  build/x86_64/libuiox_soc_x86_64.a"
	$(Q)echo "  build/riscv64/libuiox_soc_riscv64.a"

# =============================================================================
# Source listing (for auditing — not a build target)
# =============================================================================
.PHONY: list-srcs
list-srcs:
	$(Q)echo "--- Common sources (all targets) ---"
	$(Q)for f in $(SOC_COMMON_SRCS); do echo "  $$f"; done
	$(Q)echo "--- ARM64-specific ---"
	$(Q)echo "  $(SRCDIR)/uiox_soc_arm64.c"
	$(Q)echo "  $(SRCDIR)/uiox_soc_imx8mp.c"
	$(Q)echo "  $(SRCDIR)/uiox_soc_rk3588.c"
	$(Q)echo "--- ARM32-specific ---"
	$(Q)echo "  $(SRCDIR)/uiox_soc_arm32.c"
	$(Q)echo "  $(SRCDIR)/uiox_soc_omap4430.c"
	$(Q)echo "--- x86_64-specific ---"
	$(Q)echo "  $(SRCDIR)/uiox_soc_x86.c"
	$(Q)echo "--- RISC-V64-specific ---"
	$(Q)echo "  $(SRCDIR)/uiox_soc_riscv64.c"
	$(Q)echo "  $(SRCDIR)/uiox_soc_th1520.c"
