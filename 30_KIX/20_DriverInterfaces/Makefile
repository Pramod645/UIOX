# =============================================================================
# 30_KIX/20_DriverInterfaces/Makefile  — v1.0.0
#
# UIOX Hardware Driver Interfaces — master Makefile (Linux + macOS).
# Builds libhw.a for the selected architecture.
#
# Usage (matches 33_PCS / 32_FS / 10_BSP pattern):
#   make arm64 / arm32 / riscv64 / x86_64
#   make all                     # all four architectures
#   make clean[-arm64|-arm32|-riscv64|-x86_64]
#   make info[-arm64|-arm32|-riscv64|-x86_64]
#
# Called by the 30_KIX umbrella Makefile with:
#   make -C 20_DriverInterfaces CC=<cross> AR=<cross-ar> \
#       ARCH_CFLAGS="<arch flags>" ARCH_CPUDEF="<-D flags>" <arch>
#
# Directory layout:
#   20_DriverInterfaces/
#   ├── include/          uiox_klibc.h
#   ├── 01_Com/           bt  eth  hdmi  tb4
#   ├── 02_Sensors/       WiFi  camera  mic  thermal  touchpwd
#   └── 03_NonSecnsors/   bms  chg  emmc  fan  pmic  rtc  speaker
#
# All .c files in src/ and all peripheral subdirectories are compiled
# automatically via wildcards — no Makefile edit needed when adding files.
#
# @version 1.0.0  @date 2026-07-25
# =============================================================================

# ── Makefile-relative base ────────────────────────────────────────────────────
MFDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# ── Host OS detection ─────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  LLVM_BIN := $(shell \
    if command -v llvm-objcopy >/dev/null 2>&1; then \
        dirname $$(command -v llvm-objcopy); \
    else \
        echo $$(brew --prefix llvm 2>/dev/null || echo /opt/homebrew/opt/llvm)/bin; \
    fi)
  ARM64_CC := aarch64-elf-gcc
  ARM64_AR := aarch64-elf-ar
  ARM32_CC := arm-none-eabi-gcc
  ARM32_AR := arm-none-eabi-ar
  RV64_CC  := riscv64-elf-gcc
  RV64_AR  := riscv64-elf-ar
  X86_CC   := x86_64-elf-gcc
  X86_AR   := x86_64-elf-ar
else
  ARM64_CC := aarch64-linux-gnu-gcc
  ARM64_AR := aarch64-linux-gnu-ar
  ARM32_CC := arm-linux-gnueabihf-gcc
  ARM32_AR := arm-linux-gnueabihf-ar
  RV64_CC  := riscv64-linux-gnu-gcc
  RV64_AR  := riscv64-linux-gnu-ar
  X86_CC   := x86_64-linux-gnu-gcc
  X86_AR   := x86_64-linux-gnu-ar
endif

# ── BSP paths ─────────────────────────────────────────────────────────────────
BSPDIR ?= $(abspath $(MFDIR)../../10_BSP)
SOCINC  = $(BSPDIR)/03_SoC/include

# ── HW include root ───────────────────────────────────────────────────────────
HWINC := $(MFDIR)include

# ── Common compiler flags ─────────────────────────────────────────────────────
COMMON_CFLAGS := \
  -std=c11              \
  -Wall -Wextra -Werror \
  -ffreestanding        \
  -fno-builtin          \
  -fno-stack-protector  \
  -ffunction-sections   \
  -fdata-sections       \
  -nostdinc             \
  -Os -g

# ── Include paths ─────────────────────────────────────────────────────────────
COMMON_INCLUDES := \
  -I$(HWINC)                                    \
  -I$(MFDIR)01_Com/eth                          \
  -I$(MFDIR)01_Com/hdmi                         \
  -I$(MFDIR)01_Com/tb4                          \
  -I$(MFDIR)01_Com/bt                           \
  -I$(MFDIR)02_Sensors/WiFi                     \
  -I$(MFDIR)02_Sensors/camera                   \
  -I$(MFDIR)02_Sensors/mic                      \
  -I$(MFDIR)02_Sensors/thermal                  \
  -I$(MFDIR)02_Sensors/touchpwd                 \
  -I$(MFDIR)03_NonSecnsors/bms                  \
  -I$(MFDIR)03_NonSecnsors/chg                  \
  -I$(MFDIR)03_NonSecnsors/emmc                 \
  -I$(MFDIR)03_NonSecnsors/fan                  \
  -I$(MFDIR)03_NonSecnsors/pmic                 \
  -I$(MFDIR)03_NonSecnsors/rtc                  \
  -I$(MFDIR)03_NonSecnsors/speaker              \
  -I$(SOCINC)

# ── Source files — wildcards pick up every .c in the tree ────────────────────
HW_SRCS_C :=
HW_SRCS_C += $(wildcard $(MFDIR)src/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)01_Com/bt/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)01_Com/eth/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)01_Com/hdmi/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)01_Com/tb4/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)02_Sensors/WiFi/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)02_Sensors/camera/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)02_Sensors/mic/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)02_Sensors/thermal/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)02_Sensors/touchpwd/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)03_NonSecnsors/bms/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)03_NonSecnsors/chg/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)03_NonSecnsors/emmc/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)03_NonSecnsors/fan/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)03_NonSecnsors/pmic/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)03_NonSecnsors/rtc/*.c)
HW_SRCS_C += $(wildcard $(MFDIR)03_NonSecnsors/speaker/*.c)

# ── vpath — sources found by basename ─────────────────────────────────────────
vpath %.c \
  $(MFDIR)src                       \
  $(MFDIR)01_Com/bt                 \
  $(MFDIR)01_Com/eth                \
  $(MFDIR)01_Com/hdmi               \
  $(MFDIR)01_Com/tb4                \
  $(MFDIR)02_Sensors/WiFi           \
  $(MFDIR)02_Sensors/camera         \
  $(MFDIR)02_Sensors/mic            \
  $(MFDIR)02_Sensors/thermal        \
  $(MFDIR)02_Sensors/touchpwd       \
  $(MFDIR)03_NonSecnsors/bms        \
  $(MFDIR)03_NonSecnsors/chg        \
  $(MFDIR)03_NonSecnsors/emmc       \
  $(MFDIR)03_NonSecnsors/fan        \
  $(MFDIR)03_NonSecnsors/pmic       \
  $(MFDIR)03_NonSecnsors/rtc        \
  $(MFDIR)03_NonSecnsors/speaker

ARFLAGS := rcs

# =============================================================================
# Per-architecture named targets  (same pattern as 10_BSP / 33_PCS / 32_FS)
# =============================================================================

.PHONY: arm64
arm64: _do_arm64
	@echo "[HW] arm64 done → build/arm64/"

.PHONY: _do_arm64
_do_arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-aarch64-linux-gnu"; \
	      echo "  macOS: brew install aarch64-elf-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build              \
	    _ARCH=arm64                                  \
	    CC=$(ARM64_CC) AR=$(ARM64_AR)               \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" \
	    ARCH_CPUDEF="-D__aarch64__"

.PHONY: arm32
arm32: _do_arm32
	@echo "[HW] arm32 done → build/arm32/"

.PHONY: _do_arm32
_do_arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-arm-linux-gnueabihf"; \
	      echo "  macOS: brew install arm-none-eabi-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                    \
	    _ARCH=arm32                                        \
	    CC=$(ARM32_CC) AR=$(ARM32_AR)                     \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_CPUDEF="-D__arm__"

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[HW] riscv64 done → build/riscv64/"

.PHONY: _do_riscv64
_do_riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-riscv64-linux-gnu"; \
	      echo "  macOS: brew install riscv64-elf-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                          \
	    _ARCH=riscv64                                            \
	    CC=$(RV64_CC) AR=$(RV64_AR)                             \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[HW] x86_64 done → build/x86_64/"

.PHONY: _do_x86_64
_do_x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-x86-64-linux-gnu"; \
	      echo "  macOS: brew install x86_64-elf-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                               \
	    _ARCH=x86_64                                                  \
	    CC=$(X86_CC) AR=$(X86_AR)                                    \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__"

# =============================================================================
# Internal _build target
# =============================================================================
_BUILD_DIR = $(MFDIR)build/$(_ARCH)
_ARCH_INC  = $(BSPDIR)/10_Arch/$(_ARCH)/include

_CFLAGS_ALL = $(COMMON_CFLAGS)   \
              $(COMMON_INCLUDES)  \
              -I$(_ARCH_INC)      \
              $(ARCH_CFLAGS)      \
              $(ARCH_CPUDEF)

_OBJS    = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(HW_SRCS_C))
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
-include $(_OBJS:.o=.d)

.PHONY: _build
_build: $(_BUILD_DIR)/libhw.a
	@echo "[HW] _build done: $(_ARCH)"

$(_BUILD_DIR)/libhw.a: $(_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(_CFLAGS_ALL) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

# =============================================================================
# Convenience targets
# =============================================================================

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[HW] all builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; rm -rf $(MFDIR)build/arm64
clean-arm32:   ; rm -rf $(MFDIR)build/arm32
clean-riscv64: ; rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; rm -rf $(MFDIR)build/x86_64
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] all build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:
	@echo "HOST=$(UNAME_S) ARCH=arm64   CC=$(ARM64_CC)"
info-arm32:
	@echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC)"
info-riscv64:
	@echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC)"
info-x86_64:
	@echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC)"
