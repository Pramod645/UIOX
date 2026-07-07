# =============================================================================
# UIOX Firmware — Makefile
# macOS (Homebrew) + Linux compatible.
# Mirrors uiox-bootloader Makefile pattern exactly.
#
# macOS setup:
#   brew install aarch64-elf-gcc x86_64-elf-gcc qemu
#
# Usage:
#   make all              — build all three architectures
#   make ARCH=arm64       — ARM64 only
#   make ARCH=arm32       — ARM32 only
#   make ARCH=x86_64      — x86-64 only
#   make run ARCH=arm64   — build + QEMU run
#   make clean
# =============================================================================
# =============================================================================
# UIOX Firmware — Makefile  (fixed: entry stubs, linker scripts, run target)
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC    $<"
  LOGAS    := @echo "  AS    $<"
  LOGLD    := @echo "  LD    $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q :=
  LOGCC :=
  LOGAS :=
  LOGLD :=
  LOGCLEAN :=
endif

SRCDIR   := src
INCDIR   := include
LDDIR    := linker
BUILDDIR := build

# ── Detect host OS ────────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC      := aarch64-elf-gcc
  ARM64_OBJCOPY := aarch64-elf-objcopy
  ARM32_CC      := arm-none-eabi-gcc
  ARM32_OBJCOPY := arm-none-eabi-objcopy
  X86_CC        := x86_64-elf-gcc
  X86_OBJCOPY   := x86_64-elf-objcopy
else
  ARM64_CC      := aarch64-linux-gnu-gcc
  ARM64_OBJCOPY := aarch64-linux-gnu-objcopy
  ARM32_CC      := arm-linux-gnueabihf-gcc
  ARM32_OBJCOPY := arm-linux-gnueabihf-objcopy
  X86_CC        := x86_64-linux-gnu-gcc
  X86_OBJCOPY   := x86_64-linux-gnu-objcopy
endif

# ── Common C sources ──────────────────────────────────────────────────────────
COMMON_SRCS := \
    $(SRCDIR)/uiox_fw_hw.c      \
    $(SRCDIR)/uiox_fw_irq.c     \
    $(SRCDIR)/uiox_fw_uart.c    \
    $(SRCDIR)/uiox_fw_timer.c   \
    $(SRCDIR)/uiox_fw_gpio.c    \
    $(SRCDIR)/uiox_fw_clock.c   \
    $(SRCDIR)/uiox_fw_power.c   \
    $(SRCDIR)/uiox_fw_mem.c     \
    $(SRCDIR)/uiox_fw_storage.c \
    $(SRCDIR)/uiox_fw_net.c     \
    $(SRCDIR)/uiox_fw_devsw.c   \
    $(SRCDIR)/uiox_fw_sensor.c  \
    $(SRCDIR)/uiox_fw_main.c    \
    $(SRCDIR)/uiox_fw_post.c    \
    $(SRCDIR)/uiox_fw_secboot.c \
    $(SRCDIR)/uiox_fw_tz.c      \
    $(SRCDIR)/uiox_fw_psci.c

# ── Common CFLAGS ─────────────────────────────────────────────────────────────
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
## In 70_build_config/common.mk or your per-arch .mk file
#CFLAGS += -DUIOX_SECBOOT_SKIP_IN_SIM   # remove for production
	

# GCC driver flags (NOT passed via -Wl)
GCC_LD_FLAGS := -nostdlib -nostartfiles
# Raw linker flags (passed via -Wl)
LINKER_FLAGS := -Wl,--gc-sections

DEPFLAGS = -MT $@ -MMD -MP -MF $(@:.o=.d)

# =============================================================================
# ARM64
# =============================================================================
ARM64_CFLAGS  := $(CFLAGS_COMMON) -march=armv8-a -D__aarch64__
ARM64_LD      := $(LDDIR)/uiox_fw_arm64.ld
ARM64_ELF     := $(BUILDDIR)/arm64/uiox_fw_arm64.elf
ARM64_BIN     := $(BUILDDIR)/arm64/uiox_fw_arm64.bin
ARM64_MAP     := $(BUILDDIR)/arm64/uiox_fw_arm64.map

# Arch-specific sources: entry stub + C hw file
ARM64_ARCH_SRCS_S := $(SRCDIR)/arch/arm64/uiox_fw_entry_arm64.S
ARM64_ARCH_SRCS_C := $(SRCDIR)/arch/arm64/uiox_fw_arch_arm64.c

ARM64_ARCH_OBJS := \
    $(BUILDDIR)/arm64/arch/arm64/uiox_fw_entry_arm64.o \
    $(BUILDDIR)/arm64/arch/arm64/uiox_fw_arch_arm64.o

ARM64_COMMON_OBJS := \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(COMMON_SRCS))

ARM64_OBJS := $(ARM64_ARCH_OBJS) $(ARM64_COMMON_OBJS)

ARM64_QEMU := qemu-system-aarch64 \
              -M virt -cpu cortex-a57 -nographic \
              -kernel $(ARM64_BIN)

.PHONY: arm64
arm64: $(ARM64_BIN)
	$(Q)echo "  ✓  ARM64: $(ARM64_BIN)"

$(ARM64_BIN): $(ARM64_ELF)
	$(Q)$(ARM64_OBJCOPY) -O binary $< $@

$(ARM64_ELF): $(ARM64_OBJS) $(ARM64_LD) | $(BUILDDIR)/arm64/arch/arm64
	$(LOGLD)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) \
	    -T$(ARM64_LD) \
	    $(GCC_LD_FLAGS) $(LINKER_FLAGS) \
	    -Wl,-Map=$(ARM64_MAP) \
	    $(ARM64_OBJS) -o $@

# Assembly entry
$(BUILDDIR)/arm64/arch/arm64/uiox_fw_entry_arm64.o: \
    $(SRCDIR)/arch/arm64/uiox_fw_entry_arm64.S \
    | $(BUILDDIR)/arm64/arch/arm64
	$(LOGAS)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

# Arch C file
$(BUILDDIR)/arm64/arch/arm64/uiox_fw_arch_arm64.o: \
    $(SRCDIR)/arch/arm64/uiox_fw_arch_arm64.c \
    | $(BUILDDIR)/arm64/arch/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) \
	    -I$(SRCDIR)/arch/arm64 $(DEPFLAGS) -c $< -o $@

# Common C files
$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm64/arch/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

# =============================================================================
# ARM32
# =============================================================================
ARM32_CFLAGS  := $(CFLAGS_COMMON) \
                 -march=armv7-a -mthumb -mfpu=vfpv3 -mfloat-abi=hard \
                 -D__arm__
ARM32_LD      := $(LDDIR)/uiox_fw_arm32.ld
ARM32_ELF     := $(BUILDDIR)/arm32/uiox_fw_arm32.elf
ARM32_BIN     := $(BUILDDIR)/arm32/uiox_fw_arm32.bin
ARM32_MAP     := $(BUILDDIR)/arm32/uiox_fw_arm32.map

ARM32_ARCH_OBJS := \
    $(BUILDDIR)/arm32/arch/arm32/uiox_fw_entry_arm32.o \
    $(BUILDDIR)/arm32/arch/arm32/uiox_fw_arch_arm32.o

ARM32_COMMON_OBJS := \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(COMMON_SRCS))

ARM32_OBJS := $(ARM32_ARCH_OBJS) $(ARM32_COMMON_OBJS)

ARM32_QEMU := qemu-system-arm \
              -M versatilepb -cpu cortex-a9 -nographic \
              -kernel $(ARM32_BIN)

.PHONY: arm32
arm32: $(ARM32_BIN)
	$(Q)echo "  ✓  ARM32: $(ARM32_BIN)"

$(ARM32_BIN): $(ARM32_ELF)
	$(Q)$(ARM32_OBJCOPY) -O binary $< $@

$(ARM32_ELF): $(ARM32_OBJS) $(ARM32_LD) | $(BUILDDIR)/arm32/arch/arm32
	$(LOGLD)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) \
	    -T$(ARM32_LD) \
	    $(GCC_LD_FLAGS) $(LINKER_FLAGS) \
	    -Wl,-Map=$(ARM32_MAP) \
	    $(ARM32_OBJS) -o $@

$(BUILDDIR)/arm32/arch/arm32/uiox_fw_entry_arm32.o: \
    $(SRCDIR)/arch/arm32/uiox_fw_entry_arm32.S \
    | $(BUILDDIR)/arm32/arch/arm32
	$(LOGAS)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/arch/arm32/uiox_fw_arch_arm32.o: \
    $(SRCDIR)/arch/arm32/uiox_fw_arch_arm32.c \
    | $(BUILDDIR)/arm32/arch/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) \
	    -I$(SRCDIR)/arch/arm32 $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm32/arch/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

# =============================================================================
# x86_64
# =============================================================================
X86_CFLAGS  := $(CFLAGS_COMMON) \
               -m64 -march=x86-64 -mno-red-zone -mcmodel=kernel \
               -D__x86_64__
X86_LD      := $(LDDIR)/uiox_fw_x86_64.ld
X86_ELF     := $(BUILDDIR)/x86_64/uiox_fw_x86_64.elf
X86_BIN     := $(BUILDDIR)/x86_64/uiox_fw_x86_64.bin
X86_MAP     := $(BUILDDIR)/x86_64/uiox_fw_x86_64.map

X86_ARCH_OBJS := \
    $(BUILDDIR)/x86_64/arch/x86_64/uiox_fw_entry_x86.o \
    $(BUILDDIR)/x86_64/arch/x86_64/uiox_fw_arch_x86.o

X86_COMMON_OBJS := \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(COMMON_SRCS))

X86_OBJS := $(X86_ARCH_OBJS) $(X86_COMMON_OBJS)

X86_QEMU := qemu-system-x86_64 -M q35 -nographic -kernel $(X86_BIN)

.PHONY: x86_64
x86_64: $(X86_BIN)
	$(Q)echo "  ✓  x86_64: $(X86_BIN)"

$(X86_BIN): $(X86_ELF)
	$(Q)$(X86_OBJCOPY) -O binary $< $@

$(X86_ELF): $(X86_OBJS) $(X86_LD) | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGLD)
	$(Q)$(X86_CC) $(X86_CFLAGS) \
	    -T$(X86_LD) \
	    $(GCC_LD_FLAGS) $(LINKER_FLAGS) \
	    -Wl,-Map=$(X86_MAP) \
	    $(X86_OBJS) -o $@

$(BUILDDIR)/x86_64/arch/x86_64/uiox_fw_entry_x86.o: \
    $(SRCDIR)/arch/x86_64/uiox_fw_entry_x86.S \
    | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGAS)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/arch/x86_64/uiox_fw_arch_x86.o: \
    $(SRCDIR)/arch/x86_64/uiox_fw_arch_x86.c \
    | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) \
	    -I$(SRCDIR)/arch/x86_64 $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

# =============================================================================
# Default target
# =============================================================================
.PHONY: all
ifndef ARCH
all: arm64 arm32 x86_64
	$(Q)echo ""
	$(Q)echo "  ✓  All firmware images built"
	$(Q)echo "     ARM64 : $(ARM64_BIN)"
	$(Q)echo "     ARM32 : $(ARM32_BIN)"
	$(Q)echo "     x86_64: $(X86_BIN)"
else ifeq ($(ARCH),arm64)
all: arm64
else ifeq ($(ARCH),arm32)
all: arm32
else ifeq ($(ARCH),x86_64)
all: x86_64
endif

# =============================================================================
# Directory creation
# =============================================================================
$(BUILDDIR)/arm64/arch/arm64:
	$(Q)mkdir -p $@

$(BUILDDIR)/arm32/arch/arm32:
	$(Q)mkdir -p $@

$(BUILDDIR)/x86_64/arch/x86_64:
	$(Q)mkdir -p $@

# =============================================================================
# Dependency includes
# =============================================================================
-include $(ARM64_OBJS:.o=.d)
-include $(ARM32_OBJS:.o=.d)
-include $(X86_OBJS:.o=.d)

# =============================================================================
# QEMU run  (fix error 4: graceful default instead of hard exit)
# =============================================================================
.PHONY: run
run:
ifndef ARCH
	$(Q)echo ""
	$(Q)echo "  Usage:  make run ARCH=arm64|arm32|x86_64"
	$(Q)echo ""
	$(Q)echo "  Example:"
	$(Q)echo "    make run ARCH=arm64"
	$(Q)echo ""
else ifeq ($(ARCH),arm64)
	$(ARM64_QEMU)
else ifeq ($(ARCH),arm32)
	$(ARM32_QEMU)
else ifeq ($(ARCH),x86_64)
	$(X86_QEMU)
else
	$(Q)echo "Unknown ARCH='$(ARCH)'. Use arm64, arm32, or x86_64."
endif

# =============================================================================
# Help
# =============================================================================
.PHONY: help
help:
	@echo "UIOX Firmware Build System"
	@echo ""
	@echo "  make all                Build all three architectures"
	@echo "  make ARCH=arm64         ARM64 only"
	@echo "  make ARCH=arm32         ARM32 only"
	@echo "  make ARCH=x86_64        x86-64 only"
	@echo "  make run ARCH=arm64     Build + QEMU run"
	@echo "  make V=1 ARCH=arm64     Verbose build"
	@echo "  make clean"
	@echo ""
	@echo "macOS setup:"
	@echo "  brew install aarch64-elf-gcc x86_64-elf-gcc qemu"

# =============================================================================
# Clean
# =============================================================================
.PHONY: clean
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)
