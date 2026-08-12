# =============================================================================
# 30_KIX/32_FS/Makefile  — v1.2.0
#
# UIOX File-System Subsystem — master Makefile (Linux + macOS).
# Builds libfs.a for the selected architecture.
#
# Usage (matches 33_PCS / 10_BSP pattern):
#   make arm64 / arm32 / riscv64 / x86_64
#   make all
#   make clean[-arm64|-arm32|-riscv64|-x86_64]
#   make info[-arm64|-arm32|-riscv64|-x86_64]
#
# v1.2.0: COMMONDIR added — uiox_klibc.h now lives in 30_KIX/common/
#         instead of a per-subsystem copy.
# @version 1.2.0  @date 2026-07-25
# =============================================================================

MFDIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(abspath $(MFDIR)../common)

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

BSPDIR ?= $(abspath $(MFDIR)../../10_BSP)
SOCINC  = $(BSPDIR)/03_SoC/include
FSINC  := $(MFDIR)include

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

COMMON_INCLUDES := \
  -I$(COMMONDIR)                    \
  -I$(FSINC)                        \
  -I$(MFDIR)01_fsa/include          \
  -I$(MFDIR)10_scfs/include         \
  -I$(SOCINC)

FS_SRCS_C :=
FS_SRCS_C += $(wildcard $(MFDIR)src/*.c)
FS_SRCS_C += $(wildcard $(MFDIR)01_fsa/src/*.c)
FS_SRCS_C += $(wildcard $(MFDIR)10_scfs/src/*.c)

# Add to 32_FS/Makefile SRCS:
#SRCS += $(MFDIR)src/uiox_fs_init.c
#SRCS += $(MFDIR)01_fsa/vfs.c
#SRCS += $(MFDIR)10_scfs/scfs.c
#SRCS += $(MFDIR)02_journal/uiox_journal.c


vpath %.c \
  $(MFDIR)src          \
  $(MFDIR)01_fsa/src   \
  $(MFDIR)10_scfs/src

ARFLAGS := rcs

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





# =============================================================================
# 30_KIX/32_FS/Makefile  — v1.2.0
# Added: VFS, SCFS, journal
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  = $(MFDIR)build

CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
_ARCH       ?= arm64

_BUILD_DIR := $(BUILD_BASE)/$(_ARCH)

COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror          \
  -ffreestanding -fno-builtin             \
  -fno-stack-protector                    \
  -ffunction-sections -fdata-sections     \
  -nostdinc -Os -g                        \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)           \
  -I$(MFDIR)include                       \
  -I$(MFDIR)../common                     \
  -I$(MFDIR)../33_PCS/include             \
  -I$(MFDIR)../31_BufferCache/00_FileBuff/include \
  -I$(MFDIR)02_journal                    \
  -I$(MFDIR)../02_FwHal/include

SRCS := \
  $(MFDIR)src/uiox_fs_init.c             \
  $(MFDIR)01_fsa/vfs.c                   \
  $(MFDIR)10_scfs/scfs.c                 \
  $(MFDIR)02_journal/uiox_journal.c

OBJS := $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(SRCS))
LIB  := $(_BUILD_DIR)/libfs.a

.PHONY: arm64 arm32 riscv64 x86_64
arm64 arm32 riscv64 x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build \
	    _ARCH=$@                       \
	    CC="$(CC)" AR="$(AR)"          \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)"   \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)"

.PHONY: _build
_build: $(LIB)
	@echo "[FS] $(_BUILD_DIR)/libfs.a"

$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(COMMON_CFLAGS) -I$(dir $<) -MMD -MP -c $< -o $@

-include $(wildcard $(BUILD_BASE)/$(_ARCH)/*.d)

.PHONY: clean
clean:
	rm -rf $(BUILD_BASE)
