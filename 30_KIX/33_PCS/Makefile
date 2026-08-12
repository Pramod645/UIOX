# =============================================================================
# 30_KIX/33_PCS/Makefile  — v2.0.0
#
# UIOX Process-Control Subsystem — master Makefile (Linux + macOS).
# Builds libpcs.a for the selected architecture.
#
# Usage (matches 10_BSP/Makefile pattern):
#   make arm64 / arm32 / riscv64 / x86_64
#   make all                     # all four arches
#   make clean[-arm64|-arm32|-riscv64|-x86_64]
#   make info[-arm64|-arm32|-riscv64|-x86_64]
#
# Freestanding build: -nostdinc enforced.
# All system headers replaced by 33_PCS/include/uiox_klibc.h.
#
# @version 2.0.0  @date 2026-07-23
# =============================================================================

# ── Makefile-relative base ────────────────────────────────────────────────────
# All paths anchored here so make works correctly from any CWD.
MFDIR    := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(abspath $(MFDIR)../common)

# ── Host OS detection (matches 10_BSP pattern: UNAME_S) ──────────────────────
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

# ── BSP paths (anchored to MFDIR, not CWD) ───────────────────────────────────
BSPDIR ?= $(abspath $(MFDIR)../../10_BSP)
SOCINC  = $(BSPDIR)/03_SoC/include

# ── PCS include roots ─────────────────────────────────────────────────────────
PCSINC := $(MFDIR)include

# ── Common compiler flags (matches 10_BSP COMMON_CFLAGS_BASE) ────────────────
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

# ── Common include paths (all PCS-internal) ───────────────────────────────────
COMMON_INCLUDES := \
  -I$(COMMONDIR)                           \
  -I$(PCSINC)                          \
  -I$(MFDIR)40_procStruct/include      \
  -I$(MFDIR)50_scps/include            \
  -I$(MFDIR)01_schedular/include       \
  -I$(MFDIR)02_MemMngnt/include        \
  -I$(SOCINC)

# ── Source files ──────────────────────────────────────────────────────────────
PCS_SRCS_C := \
  $(MFDIR)40_procStruct/src/uiox_task.c    \
  $(MFDIR)02_MemMngnt/src/mm.c             \
  $(MFDIR)src/uiox_proc_init.c             \
  $(MFDIR)50_scps/src/uiox_sys_call.c

  # In 33_PCS/Makefile — add to SRCS list alongside existing files:
#SRCS += $(MFDIR)src/uiox_uaccess.c
#SRCS += $(MFDIR)src/uiox_syscall.c
#SRCS += $(MFDIR)src/uiox_ioctl_soc.c
#SRCS += $(MFDIR)src/uiox_mmap.c

# Add to 33_PCS/Makefile SRCS:
#SRCS += $(MFDIR)src/uiox_uaccess.c
#SRCS += $(MFDIR)src/uiox_syscall.c
#SRCS += $(MFDIR)src/uiox_sys_fd.c
#SRCS += $(MFDIR)src/uiox_sys_mmap.c
#SRCS += $(MFDIR)src/uiox_ioctl_soc.c
#SRCS += $(MFDIR)src/uiox_sys_call_init.c


PCS_SRCS_C += $(wildcard $(MFDIR)01_schedular/src/*.c)
PCS_SRCS_C += $(wildcard $(MFDIR)02_MemMngnt/src/uiox_timer*.c)

# ── vpath — compiler finds sources by basename (matches 10_BSP pattern) ──────
vpath %.c \
  $(MFDIR)40_procStruct/src \
  $(MFDIR)02_MemMngnt/src   \
  $(MFDIR)01_schedular/src  \
  $(MFDIR)50_scps/src       \
  $(MFDIR)src

ARFLAGS := rcs

# =============================================================================
# Per-architecture named targets  (same pattern as 10_BSP/Makefile)
# =============================================================================

.PHONY: arm64
arm64: _do_arm64
	@echo "[PCS] arm64 done → build/arm64/"

.PHONY: _do_arm64
_do_arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-aarch64-linux-gnu"; \
	      echo "  macOS: brew install aarch64-elf-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build       \
	    _ARCH=arm64                           \
	    CC=$(ARM64_CC) AR=$(ARM64_AR)         \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" \
	    ARCH_CPUDEF="-D__aarch64__"

.PHONY: arm32
arm32: _do_arm32
	@echo "[PCS] arm32 done → build/arm32/"

.PHONY: _do_arm32
_do_arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-arm-linux-gnueabihf"; \
	      echo "  macOS: brew install arm-none-eabi-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build              \
	    _ARCH=arm32                                  \
	    CC=$(ARM32_CC) AR=$(ARM32_AR)               \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_CPUDEF="-D__arm__"

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[PCS] riscv64 done → build/riscv64/"

.PHONY: _do_riscv64
_do_riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-riscv64-linux-gnu"; \
	      echo "  macOS: brew install riscv64-elf-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                    \
	    _ARCH=riscv64                                      \
	    CC=$(RV64_CC) AR=$(RV64_AR)                       \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[PCS] x86_64 done → build/x86_64/"

.PHONY: _do_x86_64
_do_x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; \
	      echo "  Linux: sudo apt install gcc-x86-64-linux-gnu"; \
	      echo "  macOS: brew install x86_64-elf-gcc"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                         \
	    _ARCH=x86_64                                            \
	    CC=$(X86_CC) AR=$(X86_AR)                              \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__"

# =============================================================================
# Internal _build target
# =============================================================================
_BUILD_DIR = $(MFDIR)build/$(_ARCH)

_ARCH_INC   = $(BSPDIR)/10_Arch/$(_ARCH)/include

_CFLAGS_ALL = $(COMMON_CFLAGS)    \
              $(COMMON_INCLUDES)   \
              -I$(_ARCH_INC)       \
              $(ARCH_CFLAGS)       \
              $(ARCH_CPUDEF)

_OBJS = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(PCS_SRCS_C))

DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
-include $(_OBJS:.o=.d)

.PHONY: _build
_build: $(_BUILD_DIR)/libpcs.a
	@echo "[PCS] _build done: $(_ARCH)"

$(_BUILD_DIR)/libpcs.a: $(_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(_CFLAGS_ALL) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

# =============================================================================
# Convenience targets  (matches 10_BSP pattern)
# =============================================================================

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[PCS] all builds complete"

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
# 30_KIX/33_PCS/Makefile  — v1.3.0
# Added: uaccess, syscall, fd, mmap, ioctl, sys_call_init
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
  -I$(MFDIR)../32_FS/include              \
  -I$(MFDIR)../32_FS/02_journal           \
  -I$(MFDIR)../31_BufferCache/00_FileBuff/include \
  -I$(MFDIR)../02_FwHal/include           \
  -I$(MFDIR)00_IPC/include                \
  -I$(MFDIR)50_scps/include               \
  -I$(MFDIR)40_procStruct/include

SRCS := \
  $(MFDIR)src/uiox_proc_init.c           \
  $(MFDIR)src/uiox_sys_call_init.c       \
  $(MFDIR)src/uiox_uaccess.c             \
  $(MFDIR)src/uiox_syscall.c             \
  $(MFDIR)src/uiox_sys_fd.c              \
  $(MFDIR)src/uiox_sys_mmap.c            \
  $(MFDIR)src/uiox_ioctl_soc.c           \
  $(MFDIR)02_MemMngnt/src/mm.c           \
  $(MFDIR)02_MemMngnt/src/page_fault.c   \
  $(MFDIR)01_schedular/src/sched.c       \
  $(MFDIR)01_schedular/src/timer.c       \
  $(MFDIR)00_IPC/src/msg.c               \
  $(MFDIR)00_IPC/src/shm.c               \
  $(MFDIR)00_IPC/src/sem.c               \
  $(MFDIR)50_scps/src/fork.c             \
  $(MFDIR)50_scps/src/exec.c             \
  $(MFDIR)50_scps/src/exit_wait.c        \
  $(MFDIR)50_scps/src/signal.c           \
  $(MFDIR)03_ksign/src/uiox_ksign.c      \
  $(MFDIR)04_fboot/src/uiox_fboot.c      \
  $(MFDIR)05_sec/src/uiox_sec.c          \
  $(MFDIR)06_kpatch/src/uiox_kpatch.c

OBJS := $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(SRCS))
LIB  := $(_BUILD_DIR)/libpcs.a

.PHONY: arm64 arm32 riscv64 x86_64
arm64 arm32 riscv64 x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build \
	    _ARCH=$@                       \
	    CC="$(CC)" AR="$(AR)"          \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)"   \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)"

.PHONY: _build
_build: $(LIB)
	@echo "[PCS] $(_BUILD_DIR)/libpcs.a"

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
