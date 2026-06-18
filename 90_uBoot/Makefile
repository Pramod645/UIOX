# =============================================================================
# UIOX Bootloader — Makefile
# Builds three bootloader binaries: arm64, arm32, x86_64
# Usage:
#   make all              — build all three
#   make ARCH=arm64       — build ARM64 only
#   make ARCH=arm32       — build ARM32 only
#   make ARCH=x86_64      — build x86-64 only
#   make run ARCH=arm64   — build + QEMU run
#   make clean
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0
VERSION       := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC      $<"
  LOGAS    := @echo "  AS      $<"
  LOGAR    := @echo "  AR      $@"
  LOGLD    := @echo "  LD      $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q := 
  LOGCC := 
  LOGAS := 
  LOGAR := 
  LOGLD := 
  LOGCLEAN :=
endif

# ── Directories ──────────────────────────────────────────────────────────────
SRCDIR   := src
INCDIR   := include
LDDIR    := linker
BUILDDIR := build

# ── Common source files (arch-independent) ───────────────────────────────────
COMMON_SRCS := \
    $(SRCDIR)/uiox_boot_hw.c      \
    $(SRCDIR)/uiox_boot_console.c \
    $(SRCDIR)/uiox_boot_mem.c     \
    $(SRCDIR)/uiox_boot_fs.c      \
    $(SRCDIR)/uiox_boot_verify.c  \
    $(SRCDIR)/uiox_boot_handoff.c \
    $(SRCDIR)/uiox_boot_main.c

# ── ARM64 ────────────────────────────────────────────────────────────────────
#ARM64_CC      := aarch64-linux-gnu-gcc
ARM64_CC      := aarch64-elf-gcc
#ARM64_OBJCOPY := aarch64-linux-gnu-objcopy
ARM64_OBJCOPY := aarch64-elf-objcopy
ARM64_ARCH    := -march=armv8-a
ARM64_ASRCS   := $(SRCDIR)/arch/arm64/uiox_boot_entry_arm64.S
ARM64_CSRCS   := $(SRCDIR)/arch/arm64/uiox_boot_hw_arm64.c
ARM64_LD      := $(LDDIR)/uiox_boot_arm64.ld
ARM64_OUT_ELF := $(BUILDDIR)/arm64/uiox_boot_arm64.elf
ARM64_OUT_BIN := $(BUILDDIR)/arm64/uiox_boot_arm64.bin
ARM64_QEMU    := qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic \
                  -kernel $(ARM64_OUT_BIN)

# ── ARM32 ────────────────────────────────────────────────────────────────────
#ARM32_CC      := arm-linux-gnueabihf-gcc
ARM32_CC      := arm-linux-gnueabihf-gcc
#ARM32_OBJCOPY := arm-linux-gnueabihf-objcopy
ARM32_OBJCOPY := arm-linux-gnueabihf-objcopy
ARM32_ARCH    := -march=armv7-a -mthumb -mfpu=vfpv3 -mfloat-abi=hard
ARM32_ASRCS   := $(SRCDIR)/arch/arm32/uiox_boot_entry_arm32.S
ARM32_CSRCS   := $(SRCDIR)/arch/arm32/uiox_boot_hw_arm32.c
ARM32_LD      := $(LDDIR)/uiox_boot_arm32.ld
ARM32_OUT_ELF := $(BUILDDIR)/arm32/uiox_boot_arm32.elf
ARM32_OUT_BIN := $(BUILDDIR)/arm32/uiox_boot_arm32.bin
ARM32_QEMU    := qemu-system-arm -M versatilepb -cpu cortex-a9 -nographic \
                  -kernel $(ARM32_OUT_BIN)

# ── x86_64 ───────────────────────────────────────────────────────────────────
X86_CC        := gcc
X86_OBJCOPY   := objcopy
X86_ARCH      := -m64 -march=x86-64
X86_ASRCS     := $(SRCDIR)/arch/x86_64/uiox_boot_entry_x86.S
X86_CSRCS     := $(SRCDIR)/arch/x86_64/uiox_boot_hw_x86.c
X86_LD        := $(LDDIR)/uiox_boot_x86_64.ld
X86_OUT_ELF   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.elf
X86_OUT_BIN   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.bin
X86_QEMU      := qemu-system-x86_64 -M q35 -nographic \
                  -kernel $(X86_OUT_BIN)

# ── Common compiler flags ─────────────────────────────────────────────────────
CFLAGS_COMMON := \
    -std=c11 -Wall -Wextra -Wpedantic \
    -ffreestanding -fno-builtin \
    -fno-stack-protector \
    -ffunction-sections -fdata-sections \
    -Os \
    -I$(INCDIR) \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

LDFLAGS_COMMON := \
    -nostdlib -nostartfiles \
    --gc-sections

DEPFLAGS = -MT $@ -MMD -MP -MF $(@:.o=.d)

# ── Default target ────────────────────────────────────────────────────────────
.PHONY: all clean arm64 arm32 x86_64 run

ifndef ARCH
all: arm64 arm32 x86_64
	$(Q)echo "  ✓  All bootloaders built"
	$(Q)echo "    ARM64 : $(ARM64_OUT_BIN)"
	$(Q)echo "    ARM32 : $(ARM32_OUT_BIN)"
	$(Q)echo "    x86_64: $(X86_OUT_BIN)"
else ifeq ($(ARCH),arm64)
all: arm64
else ifeq ($(ARCH),arm32)
all: arm32
else ifeq ($(ARCH),x86_64)
all: x86_64
endif

# ── ARM64 build ───────────────────────────────────────────────────────────────
ARM64_CFLAGS := $(CFLAGS_COMMON) $(ARM64_ARCH) -D__aarch64__
ARM64_OBJS   := \
    $(patsubst $(SRCDIR)/%.S,$(BUILDDIR)/arm64/%.o,$(ARM64_ASRCS)) \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(ARM64_CSRCS)) \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(COMMON_SRCS))

arm64: $(ARM64_OUT_BIN)
	$(Q)echo "  ARM64 bootloader: $<"

$(ARM64_OUT_BIN): $(ARM64_OUT_ELF)
	$(Q)$(ARM64_OBJCOPY) -O binary $< $@

$(ARM64_OUT_ELF): $(ARM64_OBJS) | $(BUILDDIR)/arm64
	$(LOGLD)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) -T$(ARM64_LD) \
	    $(LDFLAGS_COMMON:%=-Wl,%) \
	    -Wl,-Map=$(BUILDDIR)/arm64/uiox_boot_arm64.map \
	    $^ -o $@

$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.S | $(BUILDDIR)/arm64/arch/arm64
	$(LOGAS)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

# ── ARM32 build ───────────────────────────────────────────────────────────────
ARM32_CFLAGS := $(CFLAGS_COMMON) $(ARM32_ARCH) -D__arm__
ARM32_OBJS   := \
    $(patsubst $(SRCDIR)/%.S,$(BUILDDIR)/arm32/%.o,$(ARM32_ASRCS)) \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(ARM32_CSRCS)) \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(COMMON_SRCS))

arm32: $(ARM32_OUT_BIN)
	$(Q)echo "  ARM32 bootloader: $<"

$(ARM32_OUT_BIN): $(ARM32_OUT_ELF)
	$(Q)$(ARM32_OBJCOPY) -O binary $< $@

$(ARM32_OUT_ELF): $(ARM32_OBJS) | $(BUILDDIR)/arm32
	$(LOGLD)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) -T$(ARM32_LD) \
	    $(LDFLAGS_COMMON:%=-Wl,%) \
	    -Wl,-Map=$(BUILDDIR)/arm32/uiox_boot_arm32.map \
	    $^ -o $@

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.S | $(BUILDDIR)/arm32/arch/arm32
	$(LOGAS)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

# ── x86_64 build ──────────────────────────────────────────────────────────────
X86_CFLAGS := $(CFLAGS_COMMON) $(X86_ARCH) -mno-red-zone -mcmodel=kernel
X86_OBJS   := \
    $(patsubst $(SRCDIR)/%.S,$(BUILDDIR)/x86_64/%.o,$(X86_ASRCS)) \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(X86_CSRCS)) \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(COMMON_SRCS))

x86_64: $(X86_OUT_BIN)
	$(Q)echo "  x86_64 bootloader: $<"

$(X86_OUT_BIN): $(X86_OUT_ELF)
	$(Q)$(X86_OBJCOPY) -O binary $< $@

$(X86_OUT_ELF): $(X86_OBJS) | $(BUILDDIR)/x86_64
	$(LOGLD)
	$(Q)$(X86_CC) $(X86_CFLAGS) -T$(X86_LD) \
	    $(LDFLAGS_COMMON:%=-Wl,%) \
	    -Wl,-Map=$(BUILDDIR)/x86_64/uiox_boot_x86_64.map \
	    $^ -o $@

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.S | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGAS)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

# ── Directory creation ────────────────────────────────────────────────────────
$(BUILDDIR)/arm64 $(BUILDDIR)/arm64/arch/arm64 \
$(BUILDDIR)/arm64/arch $(BUILDDIR)/arm64/arch/arm64:
	$(Q)mkdir -p $(BUILDDIR)/arm64/arch/arm64

$(BUILDDIR)/arm32 $(BUILDDIR)/arm32/arch/arm32 \
$(BUILDDIR)/arm32/arch $(BUILDDIR)/arm32/arch/arm32:
	$(Q)mkdir -p $(BUILDDIR)/arm32/arch/arm32

$(BUILDDIR)/x86_64 $(BUILDDIR)/x86_64/arch/x86_64 \
$(BUILDDIR)/x86_64/arch $(BUILDDIR)/x86_64/arch/x86_64:
	$(Q)mkdir -p $(BUILDDIR)/x86_64/arch/x86_64

# ── Dependency includes ───────────────────────────────────────────────────────
-include $(ARM64_OBJS:.o=.d)
-include $(ARM32_OBJS:.o=.d)
-include $(X86_OBJS:.o=.d)

# ── QEMU run ──────────────────────────────────────────────────────────────────
run:
ifeq ($(ARCH),arm64)
	$(Q)$(ARM64_QEMU)
else ifeq ($(ARCH),arm32)
	$(Q)$(ARM32_QEMU)
else ifeq ($(ARCH),x86_64)
	$(Q)$(X86_QEMU)
else
	$(Q)echo "Usage: make run ARCH=arm64|arm32|x86_64"; exit 1
endif

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)
