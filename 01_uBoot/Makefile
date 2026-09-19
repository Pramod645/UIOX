# =============================================================================
# UIOX Bootloader — Makefile  (macOS + Linux)
#
#   make                        build all four architectures
#   make ARCH=arm64             one architecture only
#   make BOARD=qemu             board variant: qemu | generic
#   make BOARD=qemu-arm64       explicit board name
#   make V=1 ARCH=arm64         verbose
#   make run ARCH=arm64         build + QEMU
#   make clean
#
# Layout it targets:
#   src/        shared sources, FLAT (8 files) + src/arch/<arch>/
#   board/      uiox_board_<variant>_<arch>.c   (one per build)
#   include/    uiox_boot_*.h
#   linker/     uiox_boot_<arch>.ld
#
# NOTE ON STYLE: every variable assignment is on its own line. Makefile
# assignments cannot be ";"-separated — a ";" starts a shell command.
# =============================================================================
# =============================================================================
# UIOX Bootloader — Makefile  (macOS + Linux)
#
#   make                        build all four architectures
#   make ARCH=arm64             one architecture only
#   make BOARD=qemu             board variant: qemu | generic
#   make V=1 ARCH=arm64         verbose
#   make run ARCH=arm64         build + QEMU
#   make clean
#
#	make                          # all four architectures, BOARD=qemu
# 	make ARCH=arm64               # one architecture
# 	make ARCH=arm32
#	make ARCH=x86_64
#	make ARCH=riscv64
#
#	make BOARD=qemu ARCH=arm64        # -> board/uiox_board_qemu_arm64.c
#	make BOARD=generic ARCH=arm64     # -> board/uiox_board_generic_arm64.c
#	make BOARD=qemu-riscv64 ARCH=riscv64   # explicit name also works
#
#	make V=1 ARCH=arm64               # verbose — shows every command
#	make BOARD=qemu ARCH=arm64        # build one target
#	make clean                        # remove build/
#	make help                         # built-in summary
#
#	make BOARD=qemu ARCH=arm64
#	make BOARD=generic ARCH=riscv64
#
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0
VERSION       := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# ── Verbosity ─────────────────────────────────────────────────────────────────
# NOTE: $(Q) must be the FIRST character of a recipe line. After a ";"
# make no longer strips "@" — it becomes a literal the shell tries to run.
V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    = @echo "  CC    $<"
  LOGAS    = @echo "  AS    $<"
  LOGLD    = @echo "  LD    $@"
  LOGCLEAN = @echo "  CLEAN"
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
BOARDDIR := board
BUILDDIR := build

MFDIR  := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SOCDIR := $(MFDIR)../10_BSP/03_SoC/include

# ── Toolchains ────────────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC      := aarch64-elf-gcc
  ARM64_OBJCOPY := aarch64-elf-objcopy
  ARM32_CC      := arm-none-eabi-gcc
  ARM32_OBJCOPY := arm-none-eabi-objcopy
  X86_CC        := x86_64-elf-gcc
  X86_OBJCOPY   := x86_64-elf-objcopy
  RISCV_CC      := riscv64-elf-gcc
  RISCV_OBJCOPY := riscv64-elf-objcopy
else
  ARM64_CC      := aarch64-linux-gnu-gcc
  ARM64_OBJCOPY := aarch64-linux-gnu-objcopy
  ARM32_CC      := arm-linux-gnueabihf-gcc
  ARM32_OBJCOPY := arm-linux-gnueabihf-objcopy
  X86_CC        := x86_64-linux-gnu-gcc
  X86_OBJCOPY   := x86_64-linux-gnu-objcopy
  RISCV_CC      := riscv64-linux-gnu-gcc
  RISCV_OBJCOPY := riscv64-linux-gnu-objcopy
endif

# ── Common sources — FLAT src/ ────────────────────────────────────────────────
COMMON_SRCS := \
    $(SRCDIR)/uiox_boot_hw.c              \
    $(SRCDIR)/uiox_boot_console.c         \
    $(SRCDIR)/uiox_boot_mem.c             \
    $(SRCDIR)/uiox_boot_unfs.c            \
    $(SRCDIR)/uiox_boot_bridge_unfs.c     \
    $(SRCDIR)/uiox_boot_verify.c          \
    $(SRCDIR)/uiox_boot_handoff.c         \
    $(SRCDIR)/uiox_boot_main.c            \
    $(SRCDIR)/uiox_boot_media.c           \
    $(SRCDIR)/uiox_boot_media_virtio.c    \
    $(SRCDIR)/uiox_boot_media_sdmmc.c     \
    $(SRCDIR)/uiox_boot_media_none.c      \
    $(SRCDIR)/uiox_boot_dt.c

BOARD ?= qemu
ARCH  ?=

# ── Per-arch configuration ────────────────────────────────────────────────────
ARM64_ARCH  := -march=armv8-a -mstrict-align
ARM64_ASRCS := $(SRCDIR)/arch/arm64/uiox_boot_entry_arm64.S
ARM64_CSRCS := $(SRCDIR)/arch/arm64/uiox_boot_hw_arm64.c
ARM64_LD    := $(LDDIR)/uiox_boot_arm64.ld
ARM64_ELF   := $(BUILDDIR)/arm64/uiox_boot_arm64.elf
ARM64_BIN   := $(BUILDDIR)/arm64/uiox_boot_arm64.bin
ARM64_MAP   := $(BUILDDIR)/arm64/uiox_boot_arm64.map
ARM64_DEF   := -DARCH_ARM64
ARM64_QEMU  := qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -kernel $(ARM64_BIN)

ARM32_ARCH  := -march=armv7-a -mthumb -mfpu=vfpv3 -mfloat-abi=hard
ARM32_ASRCS := $(SRCDIR)/arch/arm32/uiox_boot_entry_arm32.S
ARM32_CSRCS := $(SRCDIR)/arch/arm32/uiox_boot_hw_arm32.c
ARM32_LD    := $(LDDIR)/uiox_boot_arm32.ld
ARM32_ELF   := $(BUILDDIR)/arm32/uiox_boot_arm32.elf
ARM32_BIN   := $(BUILDDIR)/arm32/uiox_boot_arm32.bin
ARM32_MAP   := $(BUILDDIR)/arm32/uiox_boot_arm32.map
ARM32_DEF	:= -DARCH_ARM32
ARM32_QEMU  := qemu-system-arm -M versatilepb -cpu cortex-a9 -nographic -kernel $(ARM32_BIN)

X86_ARCH  := -m64 -march=x86-64 -mno-red-zone -mcmodel=kernel
X86_ASRCS := $(SRCDIR)/arch/x86_64/uiox_boot_entry_x86.S
X86_CSRCS := $(SRCDIR)/arch/x86_64/uiox_boot_hw_x86.c
X86_LD    := $(LDDIR)/uiox_boot_x86_64.ld
X86_ELF   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.elf
X86_BIN   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.bin
X86_MAP   := $(BUILDDIR)/x86_64/uiox_boot_x86_64.map
X86_DEF   := -DARCH_X86_64
X86_QEMU  := qemu-system-x86_64 -M q35 -nographic -kernel $(X86_BIN)

RISCV_ARCH  := -march=rv64gc -mabi=lp64d -mcmodel=medany -mno-relax
RISCV_ASRCS := $(SRCDIR)/arch/riscv64/uiox_boot_entry_riscv64.S
RISCV_CSRCS := $(SRCDIR)/arch/riscv64/uiox_boot_hw_riscv64.c
RISCV_LD    := $(LDDIR)/uiox_boot_riscv64.ld
RISCV_ELF   := $(BUILDDIR)/riscv64/uiox_boot_riscv64.elf
RISCV_BIN   := $(BUILDDIR)/riscv64/uiox_boot_riscv64.bin
RISCV_MAP   := $(BUILDDIR)/riscv64/uiox_boot_riscv64.map
RISCV_DEF   := -DARCH_RISCV64
RISCV_QEMU  := qemu-system-riscv64 -machine virt -bios default -nographic -kernel $(RISCV_BIN)

# ── Flags ─────────────────────────────────────────────────────────────────────
INCLUDES := -I$(INCDIR) -I$(SOCDIR) -I$(BOARDDIR)

CFLAGS_COMMON := \
    -std=c11 -Wall -Wextra -Wpedantic                 \
    -ffreestanding -fno-builtin -fno-stack-protector  \
    -ffunction-sections -fdata-sections               \
    -Os -g                                            \
    $(INCLUDES)                                       \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR)             \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR)             \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

GCC_LD_FLAGS := -nostdlib -nostartfiles
LINKER_FLAGS := -Wl,--gc-sections
DEPFLAGS     = -MT $@ -MMD -MP -MF $(@:.o=.d)


.PHONY: all clean arm64 arm32 x86_64 riscv64 run help

ifeq ($(ARCH),)
all: arm64 arm32 x86_64 riscv64
	$(Q)echo ""
	$(Q)echo "  ✓  All bootloaders built  (BOARD=$(BOARD))"
else
all: $(ARCH)
endif

# =============================================================================
# ARM64
# =============================================================================
ARM64_BOARD_SRC := $(BOARDDIR)/uiox_board_$(BOARD)_arm64.c
ARM64_CFLAGS    := $(CFLAGS_COMMON) $(ARM64_ARCH) $(ARM64_DEF)

ARM64_OBJS := \
    $(BUILDDIR)/arm64/arch/arm64/uiox_boot_entry_arm64.o \
    $(BUILDDIR)/arm64/arch/arm64/uiox_boot_hw_arm64.o    \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(COMMON_SRCS))

ifneq ($(wildcard $(ARM64_BOARD_SRC)),)
ARM64_OBJS += $(BUILDDIR)/arm64/board/$(notdir $(ARM64_BOARD_SRC:.c=.o))
endif

arm64: $(ARM64_BIN)

$(ARM64_BIN): $(ARM64_ELF)
	$(Q)$(ARM64_OBJCOPY) -O binary $< $@
	@echo "  ✓  ARM64: $(ARM64_BIN)"

$(ARM64_ELF): $(ARM64_OBJS) $(ARM64_LD) | $(BUILDDIR)/arm64/arch/arm64
	$(LOGLD)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) -T$(ARM64_LD) $(GCC_LD_FLAGS) $(LINKER_FLAGS) \
	    -Wl,-Map=$(ARM64_MAP) $(ARM64_OBJS) -o $@

$(BUILDDIR)/arm64/arch/arm64/%.o: $(SRCDIR)/arch/arm64/%.S | $(BUILDDIR)/arm64/arch/arm64
	$(LOGAS)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) -c $< -o $@

$(BUILDDIR)/arm64/arch/arm64/%.o: $(SRCDIR)/arch/arm64/%.c | $(BUILDDIR)/arm64/arch/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm64/arch/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64/board/%.o: $(BOARDDIR)/%.c | $(BUILDDIR)/arm64/board
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64/board:
	$(Q)mkdir -p $@

-include $(ARM64_OBJS:.o=.d)

# =============================================================================
# ARM32
# =============================================================================
ARM32_BOARD_SRC := $(BOARDDIR)/uiox_board_$(BOARD)_arm32.c
ARM32_CFLAGS    := $(CFLAGS_COMMON) $(ARM32_ARCH) $(ARM32_DEF)

ARM32_OBJS := \
    $(BUILDDIR)/arm32/arch/arm32/uiox_boot_entry_arm32.o \
    $(BUILDDIR)/arm32/arch/arm32/uiox_boot_hw_arm32.o    \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(COMMON_SRCS))

ifneq ($(wildcard $(ARM32_BOARD_SRC)),)
ARM32_OBJS += $(BUILDDIR)/arm32/board/$(notdir $(ARM32_BOARD_SRC:.c=.o))
endif

arm32: $(ARM32_BIN)

$(ARM32_BIN): $(ARM32_ELF)
	$(Q)$(ARM32_OBJCOPY) -O binary $< $@
	@echo "  ✓  ARM32: $(ARM32_BIN)"

$(ARM32_ELF): $(ARM32_OBJS) $(ARM32_LD) | $(BUILDDIR)/arm32/arch/arm32
	$(LOGLD)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) -T$(ARM32_LD) $(GCC_LD_FLAGS) $(LINKER_FLAGS) \
	    -Wl,-Map=$(ARM32_MAP) $(ARM32_OBJS) -o $@

$(BUILDDIR)/arm32/arch/arm32/%.o: $(SRCDIR)/arch/arm32/%.S | $(BUILDDIR)/arm32/arch/arm32
	$(LOGAS)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/arch/arm32/%.o: $(SRCDIR)/arch/arm32/%.c | $(BUILDDIR)/arm32/arch/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm32/arch/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/board/%.o: $(BOARDDIR)/%.c | $(BUILDDIR)/arm32/board
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32/board:
	$(Q)mkdir -p $@

-include $(ARM32_OBJS:.o=.d)

# =============================================================================
# x86_64
# =============================================================================
X86_BOARD_SRC := $(BOARDDIR)/uiox_board_$(BOARD)_x86_64.c
X86_CFLAGS    := $(CFLAGS_COMMON) $(X86_ARCH) $(X86_DEF)

X86_OBJS := \
    $(BUILDDIR)/x86_64/arch/x86_64/uiox_boot_entry_x86.o \
    $(BUILDDIR)/x86_64/arch/x86_64/uiox_boot_hw_x86.o    \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(COMMON_SRCS))

ifneq ($(wildcard $(X86_BOARD_SRC)),)
X86_OBJS += $(BUILDDIR)/x86_64/board/$(notdir $(X86_BOARD_SRC:.c=.o))
endif

x86_64: $(X86_BIN)

$(X86_BIN): $(X86_ELF)
	$(Q)$(X86_OBJCOPY) -O binary $< $@
	@echo "  ✓  x86_64: $(X86_BIN)"

$(X86_ELF): $(X86_OBJS) $(X86_LD) | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGLD)
	$(Q)$(X86_CC) $(X86_CFLAGS) -T$(X86_LD) $(GCC_LD_FLAGS) $(LINKER_FLAGS) \
	    -Wl,-Map=$(X86_MAP) $(X86_OBJS) -o $@

$(BUILDDIR)/x86_64/arch/x86_64/%.o: $(SRCDIR)/arch/x86_64/%.S | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGAS)
	$(Q)$(X86_CC) $(X86_CFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/arch/x86_64/%.o: $(SRCDIR)/arch/x86_64/%.c | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/x86_64/arch/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/board/%.o: $(BOARDDIR)/%.c | $(BUILDDIR)/x86_64/board
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64/board:
	$(Q)mkdir -p $@

-include $(X86_OBJS:.o=.d)

# =============================================================================
# RISC-V 64
# =============================================================================
RISCV_BOARD_SRC := $(BOARDDIR)/uiox_board_$(BOARD)_riscv64.c
RISCV_CFLAGS    := $(CFLAGS_COMMON) $(RISCV_ARCH) $(RISCV_DEF) -D__riscv_xlen=64

RISCV_OBJS := \
    $(BUILDDIR)/riscv64/arch/riscv64/uiox_boot_entry_riscv64.o \
    $(BUILDDIR)/riscv64/arch/riscv64/uiox_boot_hw_riscv64.o    \
    $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/riscv64/%.o,$(COMMON_SRCS))

ifneq ($(wildcard $(RISCV_BOARD_SRC)),)
RISCV_OBJS += $(BUILDDIR)/riscv64/board/$(notdir $(RISCV_BOARD_SRC:.c=.o))
endif

riscv64: $(RISCV_BIN)

$(RISCV_BIN): $(RISCV_ELF)
	$(Q)$(RISCV_OBJCOPY) -O binary $< $@
	@echo "  ✓  RISC-V RV64GC: $(RISCV_BIN)"

$(RISCV_ELF): $(RISCV_OBJS) $(RISCV_LD) | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGLD)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) -T$(RISCV_LD) $(GCC_LD_FLAGS) $(LINKER_FLAGS) \
	    -Wl,-Map=$(RISCV_MAP) $(RISCV_OBJS) -o $@

$(BUILDDIR)/riscv64/arch/riscv64/%.o: $(SRCDIR)/arch/riscv64/%.S | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGAS)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64/arch/riscv64/%.o: $(SRCDIR)/arch/riscv64/%.c | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGCC)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/riscv64/arch/riscv64
	$(LOGCC)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64/board/%.o: $(BOARDDIR)/%.c | $(BUILDDIR)/riscv64/board
	$(LOGCC)
	$(Q)$(RISCV_CC) $(RISCV_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64/board:
	$(Q)mkdir -p $@

-include $(RISCV_OBJS:.o=.d)

# ── Directory creation ────────────────────────────────────────────────────────
$(BUILDDIR)/arm64/arch/arm64:
	$(Q)mkdir -p $@

$(BUILDDIR)/arm32/arch/arm32:
	$(Q)mkdir -p $@

$(BUILDDIR)/x86_64/arch/x86_64:
	$(Q)mkdir -p $@

$(BUILDDIR)/riscv64/arch/riscv64:
	$(Q)mkdir -p $@

# ── QEMU run ──────────────────────────────────────────────────────────────────
run:
ifeq ($(ARCH),arm64)
	$(Q)$(ARM64_QEMU)
else ifeq ($(ARCH),arm32)
	$(Q)$(ARM32_QEMU)
else ifeq ($(ARCH),x86_64)
	$(Q)$(X86_QEMU)
else ifeq ($(ARCH),riscv64)
	$(Q)$(RISCV_QEMU)
else
	$(Q)echo "Usage: make run ARCH=arm64|arm32|x86_64|riscv64"
	$(Q)exit 1
endif

help:
	@echo "UIOX Bootloader Build System"
	@echo "  make                       build all four architectures"
	@echo "  make ARCH=arm64            build ARM64 only"
	@echo "  make BOARD=qemu            board variant (qemu | generic)"
	@echo "  make run ARCH=arm64        build + QEMU"
	@echo "  make clean                 remove build/"

clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)
