# =============================================================================
# UIOX Bootloader — Makefile  (macOS + Linux compatible)
#
# macOS prerequisites (Homebrew):
#   brew install aarch64-elf-gcc       # ARM64 cross-compiler
#   brew install x86_64-elf-gcc        # x86-64 cross-compiler
#   brew install arm-linux-gnueabihf   # ARM32 cross-compiler (optional tap)
#   brew install riscv64-elf-gcc       # RISC-V cross-compiler  <<< NEW
#   brew install qemu                  # QEMU for 'make run'
#   Ubuntu/Debian:
#     sudo apt install gcc-aarch64-linux-gnu gcc-arm-linux-gnueabihf \
#                      gcc gcc-riscv64-linux-gnu binutils \           <<< NEW
#                      qemu-system-arm qemu-system-x86 \
#                      qemu-system-misc   
#
# Usage:
#   make all                — build all three architectures
#   make ARCH=arm64         — ARM64 only
#   make ARCH=arm32         — ARM32 only
#   make ARCH=x86_64        — x86-64 only
#   make run ARCH=arm64     — build + QEMU run
#   make clean
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0
VERSION       := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC    $<"
  LOGAS    := @echo "  AS    $<"
  LOGLD    := @echo "  LD    $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q        :=
  LOGCC    :=
  LOGAS    :=
  LOGLD    :=
  LOGCLEAN :=
endif

# ── Directories ───────────────────────────────────────────────────────────────
SRCDIR   := src
INCDIR   := include
LDDIR    := linker
BUILDDIR := build

# ── Detect host OS → choose correct cross-toolchain prefix ───────────────────
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  # macOS: Homebrew bare-metal toolchains (no OS suffix on target)
  ARM64_CC      := aarch64-elf-gcc
  ARM64_OBJCOPY := aarch64-elf-objcopy
  ARM32_CC      := arm-none-eabi-gcc
  ARM32_OBJCOPY := arm-none-eabi-objcopy
  X86_CC        := x86_64-elf-gcc
  X86_OBJCOPY   := x86_64-elf-objcopy
  RISCV_CC      := riscv64-elf-gcc
  RISCV_OBJCOPY := riscv64-elf-objcopy
else
  # Linux: standard GNU cross-compilers
  ARM64_CC      := aarch64-linux-gnu-gcc
  ARM64_OBJCOPY := aarch64-linux-gnu-objcopy
  ARM32_CC      := arm-linux-gnueabihf-gcc
  ARM32_OBJCOPY := arm-linux-gnueabihf-objcopy
  X86_CC        := x86_64-linux-gnu-gcc
  X86_OBJCOPY   := x86_64-linux-gnu-objcopy
  RISCV_CC      := riscv64-linux-gnu-gcc
  RISCV_OBJCOPY := riscv64-linux-gnu-objcopy
endif

# ── Common source files (arch-independent) ────────────────────────────────────
COMMON_SRCS := \
    $(SRCDIR)/uiox_boot_hw.c      \
    $(SRCDIR)/uiox_boot_console.c \
    $(SRCDIR)/uiox_boot_mem.c     \
    $(SRCDIR)/uiox_boot_unfs.c    \
    $(SRCDIR)/uiox_boot_verify.c  \
    $(SRCDIR)/uiox_boot_handoff.c \
    $(SRCDIR)/uiox_boot_main.c

# ── ARM64 ─────────────────────────────────────────────────────────────────────
ARM64_ARCH  := -march=armv8-a
ARM64_ASRCS := $(SRCDIR)/arch/arm64/uiox_boot_entry_arm64.S
ARM64_CSRCS := $(SRCDIR)/arch/arm64/uiox_boot_hw_arm64.c
ARM64_LD    := $(LDDIR)/uiox_boot_arm64.ld
ARM64_ELF   := $(BUILDDIR)/arm64/uiox_boot_arm64.elf
ARM64_BIN   := $(BUILDDIR)/arm64/uiox_boot_arm64.bin
ARM64_MAP   := $(BUILDDIR)/arm64/uiox_boot_arm64.map
ARM64_QEMU  := qemu-system-aarch64 -M virt -cpu cortex-a57 \
               -nographic -kernel $(ARM64_BIN)

# ── ARM32 ─────────────────────────────────────────────────────────────────────
ARM32_ARCH  := -march=armv7-a -mthumb -mfpu=vfpv3 -mfloat-abi=hard
ARM32_ASRCS := $(SRCDIR)/arch/arm32/uiox_boot_entry_arm32.S
ARM32_CSRCS := $(SRCDIR)/arch/arm32/uiox_boot_hw_arm32.c
ARM32_LD    := $(LDDIR)/uiox_boot_arm32.ld
ARM32_ELF   := $(BUILDDIR)/arm32/uiox_boot_arm32.elf
ARM32_BIN   := $(BUILDDIR)/arm32/uiox_boot_arm32.bin
ARM32_MAP   := $(BUILDDIR)/arm32/uiox_boot_arm32.map
ARM32_QEMU  := qemu-system-arm -M versatilepb -cpu cortex-a9 \
               -nographic -kernel $(ARM32_BIN)

# ── x86_64 ────────────────────────────────────────────────────────────────────
X86_ARCH  := -m64 -march=x86-64
X86_ASRCS := $(SRCDIR)/arch/x86_64/uiox_boot_entry_x86.S
X86_CSRCS := $(SRCDIR)/arch/x86_64/uiox_boot_hw_x86.c
X86_LD    := $(LDDIR)/uiox_boot_x86_64.ld
X86_ELF   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.elf
X86_BIN   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.bin
X86_MAP   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.map
X86_QEMU  := qemu-system-x86_64 -M q35 -nographic -kernel $(X86_BIN)


# ── RISC-V RV64GC ─────────────────────────────────────────────────────────────
# <<< NEW — mirrors the ARM64/ARM32/x86 blocks exactly >>>
#
# Compiler flags:
#   -march=rv64gc       RV64 base + G (IMAFD) + C (compressed) extensions
#   -mabi=lp64d         64-bit int/ptr, double-precision FP regs
#   -mcmodel=medany     +/-2 GB PC-relative addressing (needed for > 2 GB PA)
#   -mno-relax          disable linker relaxation (safer at boot stage)
#
# QEMU flags:
#   -machine virt       Standard RISC-V virtual machine
#   -bios default       Load OpenSBI (M-mode FW) automatically
#   -kernel             Our ELF is the S-mode payload
RISCV_ARCH  := -march=rv64gc -mabi=lp64d -mcmodel=medany -mno-relax
RISCV_ASRCS := $(SRCDIR)/arch/riscv64/uiox_boot_entry_riscv64.S
RISCV_CSRCS := $(SRCDIR)/arch/riscv64/uiox_boot_hw_riscv64.c
RISCV_LD    := $(LDDIR)/uiox_boot_riscv64.ld
RISCV_ELF   := $(BUILDDIR)/riscv64/uiox_boot_riscv64.elf
RISCV_BIN   := $(BUILDDIR)/riscv64/uiox_boot_riscv64.bin
RISCV_MAP   := $(BUILDDIR)/riscv64/uiox_boot_riscv64.map
RISCV_QEMU  := qemu-system-riscv64 \
               -machine virt \
               -bios default \
               -nographic \
               -kernel $(RISCV_BIN)
# ── Common compiler flags ─────────────────────────────────────────────────────
CFLAGS_COMMON := \
    -std=c11 -Wall -Wextra -Wpedantic \
    -ffreestanding \
    -fno-builtin \
    -fno-stack-protector \
    -ffunction-sections \
    -fdata-sections \
    -Os \
    -g \
    -I$(INCDIR) \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

# ─────────────────────────────────────────────────────────────────────────────
# KEY FIX: The original Makefile used  LDFLAGS_COMMON := -nostdlib -nostartfiles --gc-sections
# and then expanded them as  $(LDFLAGS_COMMON:%=-Wl,%)
# which turned -nostdlib → -Wl,-nostdlib  (OK)
# but also     --gc-sections → -Wl,--gc-sections  (OK)
# AND          -nostartfiles → -Wl,-nostartfiles  (WRONG for aarch64-elf-ld)
#
# The correct split:
#   • -nostdlib and -nostartfiles are GCC driver flags → pass them BARE
#   • --gc-sections is a raw linker flag → pass via -Wl,
# ─────────────────────────────────────────────────────────────────────────────
GCC_LD_FLAGS  := -nostdlib -nostartfiles          # GCC driver flags — bare

LINKER_FLAGS  := -Wl,--gc-sections               # Raw ld flags — via -Wl,

DEPFLAGS = -MT $@ -MMD -MP -MF $(@:.o=.d)

# ── Default target ────────────────────────────────────────────────────────────
.PHONY: all clean arm64 arm32 x86_64 riscv64 run help

ifndef ARCH
all: arm64 arm32 x86_64 riscv64
	$(Q)echo ""
	$(Q)echo "  ✓  All bootloaders built"
	$(Q)echo "     ARM64 : $(ARM64_BIN)"
	$(Q)echo "     ARM32 : $(ARM32_BIN)"
	$(Q)echo "     x86_64: $(X86_BIN)"
	$(Q)echo "     riscv64: $(RISCV_BIN)"
else ifeq ($(ARCH),arm64)
all: arm64
else ifeq ($(ARCH),arm32)
all: arm32
else ifeq ($(ARCH),x86_64)
all: x86_64
else ifeq ($(ARCH),riscv64)
all: riscv64
endif

# ── ARM64 build ───────────────────────────────────────────────────────────────
ARM64_CFLAGS := $(CFLAGS_COMMON) $(ARM64_ARCH) -D__aarch64__

# Object file lists — arch sources go into arch-specific subdirectory
ARM64_ARCH_OBJS := \
    $(BUILDDIR)/arm64/arch/arm64/uiox_boot_entry_arm64.o \
    $(BUILDDIR)/arm64/arch/arm64/uiox_boot_hw_arm64.o
ARM64_COMMON_OBJS := \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(COMMON_SRCS))
ARM64_OBJS := $(ARM64_ARCH_OBJS) $(ARM64_COMMON_OBJS)

arm64: $(ARM64_BIN)
	$(Q)echo "  ✓  ARM64: $(ARM64_BIN)"

$(ARM64_BIN): $(ARM64_ELF)
	$(Q)$(ARM64_OBJCOPY) -O binary $< $@

# Link: all flags go through GCC frontend — no raw ld invocation
$(ARM64_ELF): $(ARM64_OBJS) $(ARM64_LD) | $(BUILDDIR)/arm64/arch/arm64
	$(LOGLD)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) \
	    -T$(ARM64_LD) \
	    $(GCC_LD_FLAGS) \
	    $(LINKER_FLAGS) \
	    -Wl,-Map=$(ARM64_MAP) \
	    $(ARM64_OBJS) -o $@

# Compile arch-specific .S and .c files
$(BUILDDIR)/arm64/arch/arm64/%.o: $(SRCDIR)/arch/arm64/%.S \
        | $(BUILDDIR)/arm64/arch/arm64
	$(LOGAS)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64/arch/arm64/%.o: $(SRCDIR)/arch/arm64/%.c \
        | $(BUILDDIR)/arm64/arch/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile common .c files
$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm64/arch/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

# ── ARM32 build ───────────────────────────────────────────────────────────────
ARM32_CFLAGS := $(CFLAGS_COMMON) $(ARM32_ARCH) -D__arm__

ARM32_ARCH_OBJS := \
    $(BUILDDIR)/arm32/arch/arm32/uiox_boot_entry_arm32.o \
    $(BUILDDIR)/arm32/arch/arm32/uiox_boot_hw_arm32.o
ARM32_COMMON_OBJS := \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(COMMON_SRCS))
ARM32_OBJS := $(ARM32_ARCH_OBJS) $(ARM32_COMMON_OBJS)

arm32: $(ARM32_BIN)
	$(Q)echo "  ✓  ARM32: $(ARM32_BIN)"

$(ARM32_BIN): $(ARM32_ELF)
	$(Q)$(ARM32_OBJCOPY) -O binary $< $@

$(ARM32_ELF): $(ARM32_OBJS) $(ARM32_LD) | $(BUILDDIR)/arm32/arch/arm32
	$(LOGLD)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) \
	    -T$(ARM32_LD) \
	    $(GCC_LD_FLAGS) \
	    $(LINKER_FLAGS) \
	    -Wl,-Map=$(ARM32_MAP) \
	    $(ARM32_OBJS) -o $@

$(BUILDDIR)/arm32/arch/arm32/%.o: $(SRCDIR)/arch/arm32/%.S \
        | $(BUILDDIR)/arm32/arch/arm32
	$(LOGAS)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/arch/arm32/%.o: $(SRCDIR)/arch/arm32/%.c \
        | $(BUILDDIR)/arm32/arch/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm32/arch/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

# ── x86_64 build ──────────────────────────────────────────────────────────────
X86_CFLAGS := $(CFLAGS_COMMON) $(X86_ARCH) \
              -mno-red-zone -mcmodel=kernel \
              -D__x86_64__

X86_ARCH_OBJS := \
    $(BUILDDIR)/x86_64/arch/x86_64/uiox_boot_entry_x86.o \
    $(BUILDDIR)/x86_64/arch/x86_64/uiox_boot_hw_x86.o
X86_COMMON_OBJS := \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(COMMON_SRCS))
X86_OBJS := $(X86_ARCH_OBJS) $(X86_COMMON_OBJS)

x86_64: $(X86_BIN)
	$(Q)echo "  ✓  x86_64: $(X86_BIN)"

$(X86_BIN): $(X86_ELF)
	$(Q)$(X86_OBJCOPY) -O binary $< $@

$(X86_ELF): $(X86_OBJS) $(X86_LD) | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGLD)
	$(Q)$(X86_CC) $(X86_CFLAGS) \
	    -T$(X86_LD) \
	    $(GCC_LD_FLAGS) \
	    $(LINKER_FLAGS) \
	    -Wl,-Map=$(X86_MAP) \
	    $(X86_OBJS) -o $@

$(BUILDDIR)/x86_64/arch/x86_64/%.o: $(SRCDIR)/arch/x86_64/%.S \
        | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGAS)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/arch/x86_64/%.o: $(SRCDIR)/arch/x86_64/%.c \
        | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

# =============================================================================
# ── RISC-V RV64GC build ───────────────────────────────────────────────────────
# <<< NEW — mirrors the ARM64/ARM32/x86 blocks exactly >>>
# =============================================================================
RISCV_CFLAGS := $(CFLAGS_COMMON) $(RISCV_ARCH) -D__riscv -D__riscv_xlen=64

RISCV_ARCH_OBJS := \
    $(BUILDDIR)/riscv64/arch/riscv64/uiox_boot_entry_riscv64.o \
    $(BUILDDIR)/riscv64/arch/riscv64/uiox_boot_hw_riscv64.o
RISCV_COMMON_OBJS := \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/riscv64/%.o,$(COMMON_SRCS))
RISCV_OBJS := $(RISCV_ARCH_OBJS) $(RISCV_COMMON_OBJS)

riscv64: $(RISCV_BIN)
	$(Q)echo "  ✓  RISC-V RV64GC: $(RISCV_BIN)"

$(RISCV_BIN): $(RISCV_ELF)
	$(Q)$(RISCV_OBJCOPY) -O binary $< $@

$(RISCV_ELF): $(RISCV_OBJS) $(RISCV_LD) | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGLD)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) \
	    -T$(RISCV_LD) \
	    $(GCC_LD_FLAGS) \
	    $(LINKER_FLAGS) \
	    -Wl,-Map=$(RISCV_MAP) \
	    $(RISCV_OBJS) -o $@

$(BUILDDIR)/riscv64/arch/riscv64/%.o: $(SRCDIR)/arch/riscv64/%.S \
        | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGAS)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64/arch/riscv64/%.o: $(SRCDIR)/arch/riscv64/%.c \
        | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGCC)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGCC)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) $(DEPFLAGS) -c $< -o $@

# ── Directory creation (order-only prerequisites) ─────────────────────────────
$(BUILDDIR)/arm64/arch/arm64:
	$(Q)mkdir -p $@

$(BUILDDIR)/arm32/arch/arm32:
	$(Q)mkdir -p $@

$(BUILDDIR)/x86_64/arch/x86_64:
	$(Q)mkdir -p $@

$(BUILDDIR)/riscv64/arch/riscv64:           # <<< NEW
	$(Q)mkdir -p $@
# ── Dependency auto-include ───────────────────────────────────────────────────
-include $(ARM64_OBJS:.o=.d)
-include $(ARM32_OBJS:.o=.d)
-include $(X86_OBJS:.o=.d)
-include $(RISCV_OBJS:.o=.d)               # <<< NEW

# ── QEMU run ──────────────────────────────────────────────────────────────────
run:
ifeq ($(ARCH),arm64)
	$(ARM64_QEMU)
else ifeq ($(ARCH),arm32)
	$(ARM32_QEMU)
else ifeq ($(ARCH),x86_64)
	$(X86_QEMU)
else ifeq ($(ARCH),riscv64)
	$(RISCV_QEMU)
else
	@echo "Usage: make run ARCH=arm64|arm32|x86_64|riscv64"
	@exit 1
endif

# ── Help ──────────────────────────────────────────────────────────────────────
help:
	@echo "UIOX Bootloader Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make all                 Build all three architectures"
	@echo "  make ARCH=arm64          Build ARM64 only"
	@echo "  make ARCH=arm32          Build ARM32 only"
	@echo "  make ARCH=x86_64         Build x86-64 only"
	@echo "  make ARCH=riscv64        Build riscv64 only"
	@echo "  make run ARCH=arm64      Build + QEMU run (arm64/arm32/x86_64/riscv64)"
	@echo "  make clean               Remove build directory"
	@echo "  make V=1 ARCH=arm64      Verbose build"
	@echo ""
	@echo "macOS Homebrew setup:"
	@echo "  brew install aarch64-elf-gcc x86_64-elf-gcc qemu"

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)
