# =============================================================================
#  UIOX Firmware — Makefile
#  Builds a static library per ISA from src/*.c + include/*.h only.
#  No linker scripts. No assembly files. No arch/ subdirectory.
#
#  Each target produces:
#    build/arm64/libuiox_fw_arm64.a
#    build/arm32/libuiox_fw_arm32.a
#    build/x86_64/libuiox_fw_x86_64.a
#    build/riscv64/libuiox_fw_riscv64.a
#
#  Usage:
#    make                  — build all four architectures
#    make arm64            — ARM64 only
#    make arm32            — ARM32 only
#    make x86_64           — x86-64 only
#    make riscv64          — RISC-V 64 only
#    make V=1              — verbose output
#    make clean
#
#  macOS setup:
#    brew install aarch64-elf-gcc x86_64-elf-gcc riscv64-elf-gcc qemu
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC    $<"
  LOGAR    := @echo "  AR    $@"
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
# Cross-toolchain  (auto-detect macOS vs Linux)
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

DEPFLAGS = -MT $@ -MMD -MP -MF $(@:.o=.d)

# =============================================================================
# Common CFLAGS  (no linker flags needed — we are building .a, not .elf/.bin)
# =============================================================================
CFLAGS_COMMON := \
    -std=c11 -Wall -Wextra \
    -ffreestanding -fno-builtin \
    -fno-stack-protector \
    -ffunction-sections -fdata-sections \
    -Os -g \
    -I$(INCDIR) \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

# =============================================================================
# HAL_SRCS — every .c file in src/  (no arch/ subdirectory, no .S files)
# =============================================================================
HAL_SRCS := \
    $(SRCDIR)/uiox_fw_als.c       \
    $(SRCDIR)/uiox_fw_bms.c       \
    $(SRCDIR)/uiox_fw_bt.c        \
    $(SRCDIR)/uiox_fw_camera.c    \
    $(SRCDIR)/uiox_fw_charger.c   \
    $(SRCDIR)/uiox_fw_clock.c     \
    $(SRCDIR)/uiox_fw_devsw.c     \
    $(SRCDIR)/uiox_fw_emmc.c      \
    $(SRCDIR)/uiox_fw_eth.c       \
    $(SRCDIR)/uiox_fw_fan.c       \
    $(SRCDIR)/uiox_fw_gpio.c      \
    $(SRCDIR)/uiox_fw_gpu.c       \
    $(SRCDIR)/uiox_fw_hdmi.c      \
    $(SRCDIR)/uiox_fw_hw.c        \
    $(SRCDIR)/uiox_fw_i2c.c       \
    $(SRCDIR)/uiox_fw_keyboard.c  \
    $(SRCDIR)/uiox_fw_main.c      \
    $(SRCDIR)/uiox_fw_mic.c       \
    $(SRCDIR)/uiox_fw_monitor.c   \
    $(SRCDIR)/uiox_fw_mouse.c     \
    $(SRCDIR)/uiox_fw_net.c       \
    $(SRCDIR)/uiox_fw_nvme.c      \
    $(SRCDIR)/uiox_fw_pmic.c      \
    $(SRCDIR)/uiox_fw_ramrtc.c    \
    $(SRCDIR)/uiox_fw_sata.c      \
    $(SRCDIR)/uiox_fw_sd.c        \
    $(SRCDIR)/uiox_fw_sensor.c    \
    $(SRCDIR)/uiox_fw_speaker.c   \
    $(SRCDIR)/uiox_fw_spi.c       \
    $(SRCDIR)/uiox_fw_storage.c   \
    $(SRCDIR)/uiox_fw_tb4.c       \
    $(SRCDIR)/uiox_fw_thermal.c   \
    $(SRCDIR)/uiox_fw_timer.c     \
    $(SRCDIR)/uiox_fw_touchpwd.c  \
    $(SRCDIR)/uiox_fw_uart.c      \
    $(SRCDIR)/uiox_fw_usb.c       \
    $(SRCDIR)/uiox_fw_wdt.c       \
    $(SRCDIR)/uiox_fw_wifi.c

# =============================================================================
# ARM64  (AArch64 — ARMv8-A)
# =============================================================================
ARM64_CFLAGS := $(CFLAGS_COMMON) -march=armv8-a -mstrict-align
ARM64_OBJS   := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(HAL_SRCS))
ARM64_LIB    := $(BUILDDIR)/arm64/libuiox_fw_arm64.a

.PHONY: arm64
arm64: $(ARM64_LIB)
	$(Q)echo "  OK  arm64  => $(ARM64_LIB)"

$(ARM64_LIB): $(ARM64_OBJS) | $(BUILDDIR)/arm64
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64:
	$(Q)mkdir -p $@

# =============================================================================
# ARM32  (ARMv7-A — Thumb-2)
# =============================================================================
ARM32_CFLAGS := $(CFLAGS_COMMON) -march=armv7-a -mthumb -mfloat-abi=soft
ARM32_OBJS   := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(HAL_SRCS))
ARM32_LIB    := $(BUILDDIR)/arm32/libuiox_fw_arm32.a

.PHONY: arm32
arm32: $(ARM32_LIB)
	$(Q)echo "  OK  arm32  => $(ARM32_LIB)"

$(ARM32_LIB): $(ARM32_OBJS) | $(BUILDDIR)/arm32
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32:
	$(Q)mkdir -p $@

# =============================================================================
# x86_64
# =============================================================================
X86_CFLAGS := $(CFLAGS_COMMON) -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2
X86_OBJS   := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(HAL_SRCS))
X86_LIB    := $(BUILDDIR)/x86_64/libuiox_fw_x86_64.a

.PHONY: x86_64
x86_64: $(X86_LIB)
	$(Q)echo "  OK  x86_64 => $(X86_LIB)"

$(X86_LIB): $(X86_OBJS) | $(BUILDDIR)/x86_64
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64:
	$(Q)mkdir -p $@

# =============================================================================
# RISC-V 64  (rv64imac, medany code model)
# =============================================================================
RV64_CFLAGS := $(CFLAGS_COMMON) -march=rv64imac -mabi=lp64 -mcmodel=medany
RV64_OBJS   := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/riscv64/%.o,$(HAL_SRCS))
RV64_LIB    := $(BUILDDIR)/riscv64/libuiox_fw_riscv64.a

.PHONY: riscv64
riscv64: $(RV64_LIB)
	$(Q)echo "  OK  riscv64 => $(RV64_LIB)"

$(RV64_LIB): $(RV64_OBJS) | $(BUILDDIR)/riscv64
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/riscv64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/riscv64
	$(LOGCC)
	$(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64:
	$(Q)mkdir -p $@

# =============================================================================
# Auto-generated dependency tracking
# =============================================================================
-include $(ARM64_OBJS:.o=.d)
-include $(ARM32_OBJS:.o=.d)
-include $(X86_OBJS:.o=.d)
-include $(RV64_OBJS:.o=.d)

# =============================================================================
# Convenience targets
# =============================================================================
.PHONY: all
all: arm64 arm32 x86_64 riscv64
	$(Q)echo ""
	$(Q)echo "  Built:"
	$(Q)echo "    $(ARM64_LIB)"
	$(Q)echo "    $(ARM32_LIB)"
	$(Q)echo "    $(X86_LIB)"
	$(Q)echo "    $(RV64_LIB)"

.PHONY: clean
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

.PHONY: help
help:
	@echo ""
	@echo "  UIOX Firmware HAL — build targets"
	@echo ""
	@echo "    make              build all four ISA targets"
	@echo "    make arm64        build/arm64/libuiox_fw_arm64.a"
	@echo "    make arm32        build/arm32/libuiox_fw_arm32.a"
	@echo "    make x86_64       build/x86_64/libuiox_fw_x86_64.a"
	@echo "    make riscv64      build/riscv64/libuiox_fw_riscv64.a"
	@echo "    make clean        remove build/"
	@echo "    make V=1          verbose compiler output"
	@echo ""
	@echo "  macOS setup:"
	@echo "    brew install aarch64-elf-gcc x86_64-elf-gcc riscv64-elf-gcc"
	@echo ""
