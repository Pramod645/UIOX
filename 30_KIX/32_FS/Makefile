# =============================================================================
# 30_KIX/32_FS/Makefile  — v2.0.0
#
# Builds the three layers into one archive:  build/<arch>/libfs.a
#
# Usage:
#   make arm64 / arm32 / riscv64 / x86_64
#   make all
#   make clean[-<arch>]
#   make info[-<arch>]
#   make srclist          ← what the globs actually resolved to
#
# ── the three layers, all children of 32_FS ────────────────────────────
#   00_buffcache/src/*.c    block buffer cache   (Bach Ch.3)
#   01_fsa/src/*.c          inode level          (Bach Ch.4)
#   10_scfs/src/*.c         syscall bodies       (Bach Ch.5)
#
# Not built here, though present in the tree:
#   02_jrnal/    the journal — separate concern, built when wired
#   03_netfs/    9P client   — not part of the on-disk path
#   10_unfs/     storage backend — sits UNDER 01_fsa once integrated
#   $(MFDIR)src/ retired uiox_fs_init.c lived here
#
# ── the include graph, which points DOWNWARD only ──────────────────────
#   00_buffcache  →  its own include/
#   01_fsa        →  its own include/  +  00_buffcache/include
#   10_scfs       →  its own include/  +  01_fsa/include + 00_buffcache/include
#
# No layer includes the one above it.  An upward -I means an inverted
# dependency, and this Makefile will not hide it.
#
# ── v2.0.0 changes from the tree's previous version ────────────────────
#   · 00_buffcache (lower-case c) added to SRCS and vpath — its sources
#     are in a FLAT src/, not buffers/src/
#   · $(MFDIR)src/ glob dropped — it picked up the retired uiox_fs_init.c
#   · 00_buffcache/include added to the include path so 01_fsa's
#     buffer.h → bcache.h resolves
#   · no extra compile rule: 00_buffcache is inside $(MFDIR), so the
#     generic pattern rule already matches it
#   · srclist target added — a silent empty glob is the failure mode this
#     guards against
#
# @version 2.0.0  @date 2026-09-25
# =============================================================================

MFDIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(abspath $(MFDIR)../common)

# ── the three generated layers ────────────────────────────────────────
BCDIR    := $(MFDIR)00_buffcache
FSA_DIR  := $(MFDIR)01_fsa
SCFS_DIR := $(MFDIR)10_scfs

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
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

BSPDIR ?= $(abspath $(MFDIR)../../10_BSP)
SOCINC  = $(BSPDIR)/03_SoC/include

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

# -nostdinc: <stdio.h> is unreachable on purpose.  uiox_klibc.h (in
# ../common) supplies printf/memset/memcpy as macros over uiox_*; the BSP
# SoC stdio header supplies early_puts.  An undeclared call is -Werror.
COMMON_INCLUDES := \
  -I$(COMMONDIR)          \
  -I$(MFDIR)include       \
  -I$(BCDIR)/include      \
  -I$(FSA_DIR)/include    \
  -I$(SCFS_DIR)/include   \
  -I$(SOCINC)

# ── the three layers, in dependency order ─────────────────────────────
# 00_buffcache supplies buffers to 01_fsa, which 10_scfs calls into.
FS_SRCS_C :=
FS_SRCS_C += $(wildcard $(BCDIR)/src/*.c)
FS_SRCS_C += $(wildcard $(FSA_DIR)/src/*.c)
FS_SRCS_C += $(wildcard $(SCFS_DIR)/src/*.c)

vpath %.c \
  $(BCDIR)/src    \
  $(FSA_DIR)/src  \
  $(SCFS_DIR)/src

ARFLAGS := rcs

# ── arch entry points ─────────────────────────────────────────────────
.PHONY: arm64
arm64: _do_arm64
	@echo "[FS] arm64 done → build/arm64/"
.PHONY: _do_arm64
_do_arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build              \
	    _ARCH=arm64 CC=$(ARM64_CC) AR=$(ARM64_AR)   \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" \
	    ARCH_CPUDEF="-D__aarch64__"

.PHONY: arm32
arm32: _do_arm32
	@echo "[FS] arm32 done → build/arm32/"
.PHONY: _do_arm32
_do_arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                    \
	    _ARCH=arm32 CC=$(ARM32_CC) AR=$(ARM32_AR)         \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_CPUDEF="-D__arm__"

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[FS] riscv64 done → build/riscv64/"
.PHONY: _do_riscv64
_do_riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                          \
	    _ARCH=riscv64 CC=$(RV64_CC) AR=$(RV64_AR)               \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[FS] x86_64 done → build/x86_64/"
.PHONY: _do_x86_64
_do_x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                               \
	    _ARCH=x86_64 CC=$(X86_CC) AR=$(X86_AR)                     \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__"

_BUILD_DIR = $(MFDIR)build/$(_ARCH)
_ARCH_INC  = $(BSPDIR)/10_Arch/$(_ARCH)/include

_CFLAGS_ALL = $(COMMON_CFLAGS)   \
              $(COMMON_INCLUDES)  \
              -I$(_ARCH_INC)      \
              $(ARCH_CFLAGS)      \
              $(ARCH_CPUDEF)

_OBJS    = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(FS_SRCS_C))
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
-include $(_OBJS:.o=.d)

.PHONY: _build
_build: $(_BUILD_DIR)/libfs.a
	@echo "[FS] _build done: $(_ARCH)"

$(_BUILD_DIR)/libfs.a: $(_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(_CFLAGS_ALL) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[FS] all builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; @rm -rf $(MFDIR)build/arm64
clean-arm32:   ; @rm -rf $(MFDIR)build/arm32
clean-riscv64: ; @rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; @rm -rf $(MFDIR)build/x86_64
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] all build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:
	@echo "HOST=$(UNAME_S) ARCH=arm64  CC=$(ARM64_CC)"
	@echo "  BCDIR    = $(BCDIR)"
	@echo "  FSA_DIR  = $(FSA_DIR)"
	@echo "  SCFS_DIR = $(SCFS_DIR)"
info-arm32:   ; @echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC)"
info-riscv64: ; @echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC)"
info-x86_64:  ; @echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC)"

# ── srclist — every file the globs resolved to ────────────────────────
# A wildcard that matches nothing produces NO error in make; the build
# simply links fewer objects.  This target makes an empty layer visible.
.PHONY: srclist
srclist:
	@echo "[FS] sources to build:"
	@for f in $(FS_SRCS_C); do echo "  $$f"; done
	@echo "[FS] total: $(words $(FS_SRCS_C))"