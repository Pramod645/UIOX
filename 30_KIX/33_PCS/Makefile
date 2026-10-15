# =============================================================================
# 30_KIX/33_PCS/Makefile  — v2.0.0
#
# Builds the PCS layers and their peers into one archive:
#     build/<arch>/libpcs.a
#
# Usage:
#   make arm64 / arm32 / riscv64 / x86_64
#   make all
#   make clean[-<arch>]
#   make info[-<arch>]
#   make srclist          ← what the globs actually resolved to
#
# ── the layers, all children of 33_PCS ─────────────────────────────────
#   00_IPC/src/*.c          msgq · sem · shm · socket · ptrace
#   01_schedular/src/*.c    scheduler · clock · timer · profiler · time
#   02_MemMngnt/src/*.c     page fault · swapper · phys alloc · acct
#   40_psa/src/*.c          PSA algorithms
#   50_scpcs/src/*.c        syscall boundary (SCPCS_*)
#
# ── peers, not a layer above ───────────────────────────────────────────
#   03_ksign/src/*.c        kernel signing + integrity   (moved from 50_UIX)
#   04_fboot/src/*.c        fast-boot / snapshot         (moved from 50_UIX)
#   05_sec/src/*.c          ASLR + MAC security          (moved from 50_UIX)
#   06_kpatch/src/*.c       live kernel patching         (moved from 50_UIX)
#
#   These four are mechanisms the layers CALL, not stages data passes
#   through.  Same relationship as 02_jrnal in 32_FS: the include may be
#   upward in DIRECTORY terms and lateral in terms of WHO CALLS WHOM.
#   The test is direction of calls, not depth in the tree.
#
# ── the include graph ──────────────────────────────────────────────────
#   00_IPC      →  its own include/
#   01_schedular →  its own include/  +  00_IPC/include
#   02_MemMngnt →  its own include/  +  01_schedular/include
#   40_psa      →  its own include/  +  02_MemMngnt/include
#   50_scpcs    →  its own include/  +  40_psa/include + 02_MemMngnt/include
#                   + 00_IPC/include
#   03_ksign    →  peers only need what they call into; each header must be
#   04_fboot       self-sufficient.  If two peer headers include each
#   05_sec         other, that is the cycle to break — the mutual
#   06_kpatch      dependency belongs in the .c files.
#
# ── v2.0.0 notes ──────────────────────────────────────────────────────
#   · The five PCS layers and four peers are globbed in, in dependency
#     order, then $(MFDIR)src/*.c — a wildcard that matches nothing is
#     silent in make, and 32_FS v2.3.0 found uiox_fs_init.c missing from
#     its archive exactly that way.  Run `make srclist` and check the
#     count before trusting a build.
#   · 02_FwHal/include is on the include path: PCS sources that log
#     through uiox_fw_printf() need the declaration, same as the FS
#     journal sources did.
#   · uiox_devclass.h lives in 30_DSS/include (one level up) — added so
#     the same device vocabulary the drivers use is reachable here.
# =============================================================================

# ── Roots ─────────────────────────────────────────────────────────────
MFDIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(abspath $(MFDIR)../common)
UBOOT     := $(abspath $(MFDIR)../../01_uBoot)


IPC_DIR   := $(MFDIR)00_IPC
SCHED_DIR := $(MFDIR)01_schedular
MEM_DIR   := $(MFDIR)02_MemMngnt
PSA_DIR   := $(MFDIR)40_psa
SCPCS_DIR := $(MFDIR)50_scpcs

KSIGN_DIR := $(MFDIR)03_ksign
FBOOT_DIR := $(MFDIR)04_fboot
SEC_DIR   := $(MFDIR)05_sec
KPATCH_DIR:= $(MFDIR)06_kpatch

FWHAL     := $(abspath $(MFDIR)../../02_FwHal)
SCIXDIR   := $(abspath $(MFDIR)../../40_SCIX)
DSSDIR    := $(abspath $(MFDIR)..)

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
  -I$(IPC_DIR)/include    \
  -I$(SCHED_DIR)/include  \
  -I$(MEM_DIR)/include    \
  -I$(PSA_DIR)/include    \
  -I$(SCPCS_DIR)/include  \
  -I$(KSIGN_DIR)/include  \
  -I$(FBOOT_DIR)/include  \
  -I$(SEC_DIR)/include    \
  -I$(KPATCH_DIR)/include \
  -I$(DSSDIR)/include     \
  -I$(FWHAL)/include      \
  -I$(SCIXDIR)/include    \
  -I$(UBOOT)/include      \
  -I$(SOCINC)

# ── the layers, in dependency order ───────────────────────────────────
# 00_IPC defines the message/queue vocabulary the scheduler wakes on;
# 01_schedular supplies the sleep/wake primitives the memory layer needs;
# 02_MemMngnt owns the page maps 40_psa scores; 50_scpcs is the boundary
# that routes into all of them.
PCS_SRCS_C :=
PCS_SRCS_C += $(wildcard $(KSIGN_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(FBOOT_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(SEC_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(KPATCH_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(IPC_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(SCHED_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(MEM_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(PSA_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(SCPCS_DIR)/src/*.c)
PCS_SRCS_C += $(wildcard $(MFDIR)src/*.c)

vpath %.c \
  $(KSIGN_DIR)/src  \
  $(FBOOT_DIR)/src  \
  $(SEC_DIR)/src    \
  $(KPATCH_DIR)/src \
  $(IPC_DIR)/src    \
  $(SCHED_DIR)/src  \
  $(MEM_DIR)/src    \
  $(PSA_DIR)/src    \
  $(SCPCS_DIR)/src  \
  $(MFDIR)src

ARFLAGS := rcs

# ── arch entry points ─────────────────────────────────────────────────
.PHONY: arm64
arm64: _do_arm64
	@echo "[PCS] arm64 done → build/arm64/"
.PHONY: _do_arm64
_do_arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build               \
	    _ARCH=arm64 CC=$(ARM64_CC) AR=$(ARM64_AR)    \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" \
	    ARCH_CPUDEF="-D__aarch64__"

.PHONY: arm32
arm32: _do_arm32
	@echo "[PCS] arm32 done → build/arm32/"
.PHONY: _do_arm32
_do_arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                     \
	    _ARCH=arm32 CC=$(ARM32_CC) AR=$(ARM32_AR)          \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_CPUDEF="-D__arm__"

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[PCS] riscv64 done → build/riscv64/"
.PHONY: _do_riscv64
_do_riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                       \
	    _ARCH=riscv64 CC=$(RV64_CC) AR=$(RV64_AR)            \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[PCS] x86_64 done → build/x86_64/"
.PHONY: _do_x86_64
_do_x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                        \
	    _ARCH=x86_64 CC=$(X86_CC) AR=$(X86_AR)                \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__"

_BUILD_DIR = $(MFDIR)build/$(_ARCH)
_ARCH_INC  = $(BSPDIR)/10_Arch/$(_ARCH)/include

_CFLAGS_ALL = $(COMMON_CFLAGS)   \
              $(COMMON_INCLUDES)  \
              -I$(_ARCH_INC)      \
              $(ARCH_CFLAGS)      \
              $(ARCH_CPUDEF)

_OBJS    = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(PCS_SRCS_C))
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

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[PCS] all builds complete"

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
	@echo "  IPC_DIR   = $(IPC_DIR)"
	@echo "  SCHED_DIR = $(SCHED_DIR)"
	@echo "  MEM_DIR   = $(MEM_DIR)"
	@echo "  PSA_DIR   = $(PSA_DIR)"
	@echo "  SCPCS_DIR = $(SCPCS_DIR)"
info-arm32:   ; @echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC)"
info-riscv64: ; @echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC)"
info-x86_64:  ; @echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC)"

# ── srclist — every file the globs resolved to ────────────────────────
# A wildcard that matches nothing produces NO error in make; the build
# simply links fewer objects.  This target makes an empty layer visible.
# Run it before trusting a build.
.PHONY: srclist
srclist:
	@echo "[PCS] sources to build:"
	@for f in $(PCS_SRCS_C); do echo "  $$f"; done
	@echo "[PCS] total: $(words $(PCS_SRCS_C))"
