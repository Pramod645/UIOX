# =============================================================================
# 30_KIX/33_PCS/Makefile  — v3.0.0
#
# UIOX Process-Control Subsystem — master Makefile (Linux + macOS).
# Builds libpcs.a for the selected architecture.
#
#   make arm64 | arm32 | riscv64 | x86_64
#   make all                              # all four
#   make clean[-arm64|-arm32|-riscv64|-x86_64]
#   make info[-arm64|-arm32|-riscv64|-x86_64]
#
# Freestanding build: -nostdinc enforced.  System headers are replaced by
# 33_PCS/include/uiox_klibc.h.
#
# ── v3.0.0 CHANGES ───────────────────────────────────────────────────────────
# The layer was renamed and re-scoped since v2.0.0, so this Makefile follows:
#
#   40_procStruct  ->  40_psa     (PSA — Process system Architecture)
#                      files: uiox_kix_psa_*.c / .h
#
#   50_scps        ->  50_scpcs  (SCPCS — System Call Process Control System)
#                      files: uiox_kix_scpcs_*.c
#
# The two directories are the whole process layer, and this Makefile builds
# BOTH into libpcs.a — 40_psa the algorithms, 50_scpcs the syscall boundary
# above them.
#
# v2.0.0 also listed files under 33_PCS/src/ that this version does not
# assume: uiox_proc_init.c, uiox_sys_call_init.c, uiox_uaccess.c,
# uiox_syscall.c, uiox_sys_fd.c, uiox_sys_mmap.c, uiox_ioctl_soc.c.
# If any of those exist in your tree, uncomment its line in PCS_SRCS_C below.
# They are commented rather than listed because a missing source is a hard
# make error, and this file should build on what is actually present.
#
# @version 3.0.0  @date 2026-09-29
# =============================================================================

# ── Makefile-relative base ────────────────────────────────────────────────────
# All paths anchored here so make works correctly from any CWD.
MFDIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(abspath $(MFDIR)../common)

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

# ── BSP paths (anchored to MFDIR, not CWD) ───────────────────────────────────
BSPDIR ?= $(abspath $(MFDIR)../../10_BSP)
SOCINC  = $(BSPDIR)/03_SoC/include

# ── The two layer directory names, in ONE place ──────────────────────────────
# If either is renamed again, these two lines are the whole change.
PSADIR   := $(MFDIR)40_psa
SCPCSDIR := $(MFDIR)50_scpcs

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
# Both layer include dirs are here because each needs the other's headers:
# 50_scpcs/../include/uiox_kix_scpcs.h includes uiox_kix_psa_process.h, and
# 40_psa sources include their own four headers.
COMMON_INCLUDES := \
  -I$(COMMONDIR)                    \
  -I$(MFDIR)include                 \
  -I$(PSADIR)/include               \
  -I$(SCPCSDIR)/include             \
  -I$(MFDIR)01_schedular/include    \
  -I$(MFDIR)02_MemMngnt/include     \
  -I$(SOCINC)

# ── Source files ──────────────────────────────────────────────────────────────
#
# 40_psa  — the algorithms.  Four sources, matching its four headers.
PCS_SRCS_C := \
  $(PSADIR)/src/uiox_kix_psa_process.c       \
  $(PSADIR)/src/uiox_kix_psa_region.c        \
  $(PSADIR)/src/uiox_kix_psa_context.c       \
  $(PSADIR)/src/uiox_kix_psa_sleep_wakeup.c

#
# 50_scpcs — the syscall boundary.  A wildcard rather than 25 named lines,
# because this layer is deliberately one function per file and listing them
# means editing this Makefile every time one is added.  The wildcard picks up
# all of them, and still works if the set is ever regrouped.
PCS_SRCS_C += $(wildcard $(SCPCSDIR)/src/uiox_kix_scpcs_*.c)

#
# Sources elsewhere in 33_PCS that were part of the old 17-syscall layout.
# Commented, because this file must build on what exists.  Uncomment any that
# are present in your tree:
#
#PCS_SRCS_C += $(MFDIR)src/uiox_proc_init.c
#PCS_SRCS_C += $(MFDIR)src/uiox_sys_call_init.c
#PCS_SRCS_C += $(MFDIR)src/uiox_uaccess.c
#PCS_SRCS_C += $(MFDIR)src/uiox_syscall.c
#PCS_SRCS_C += $(MFDIR)src/uiox_sys_fd.c
#PCS_SRCS_C += $(MFDIR)src/uiox_sys_mmap.c
#PCS_SRCS_C += $(MFDIR)src/uiox_ioctl_soc.c

#
# Supporting subsystems the process layer calls into.  Same rule: commented
# unless confirmed present.  A wildcard is used where the whole directory is
# wanted, so a missing directory expands to nothing instead of failing.
#
#PCS_SRCS_C += $(MFDIR)02_MemMngnt/src/mm.c
#PCS_SRCS_C += $(MFDIR)02_MemMngnt/src/page_fault.c
#PCS_SRCS_C += $(MFDIR)src/uiox_task.c
#
PCS_SRCS_C += $(wildcard $(MFDIR)01_schedular/src/*.c)

# ── Object mapping ────────────────────────────────────────────────────────────
# Objects mirror the source path under build/<arch>/, so
#   40_psa/src/uiox_kix_psa_region.c  ->  build/arm64/40_psa/src/uiox_kix_psa_region.o
# The directory part is preserved by the pattern rule below.
BUILD_BASE  = $(MFDIR)build

# ── vpath ─────────────────────────────────────────────────────────────────────
# NOT used in this version.  v2.0.0 used vpath with basenames, which cannot
# work once two directories hold same-named files — and the layers here are
# named apart precisely to avoid that.  Keeping the full path in _OBJS makes
# the object layout self-describing and the failure messages readable.
vpath %.c $(PSADIR)/src $(SCPCSDIR)/src

ARFLAGS := rcs

# =============================================================================
# Per-architecture named targets
# =============================================================================

.PHONY: arm64
arm64: _do_arm64
	@echo "[PCS] arm64 done -> build/arm64/"

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
	@echo "[PCS] arm32 done -> build/arm32/"

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
	@echo "[PCS] riscv64 done -> build/riscv64/"

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

# x86_64: -mno-sse / -mno-sse2 ban floating point, because the return path
# uses SSE registers.  Any source using float will fail here — the same rule
# that forced the thermal-sensor rewrite.  Kept, because this arch is the one
# that catches it.
.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[PCS] x86_64 done -> build/x86_64/"

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
_ARCH     ?= arm64
_BUILD_DIR = $(BUILD_BASE)/$(_ARCH)
_ARCH_INC  = $(BSPDIR)/10_Arch/$(_ARCH)/include

_CFLAGS_ALL = $(COMMON_CFLAGS)    \
              $(COMMON_INCLUDES)  \
              -I$(_ARCH_INC)      \
              $(ARCH_CFLAGS)      \
              $(ARCH_CPUDEF)

# Objects keep their layer path under the arch build dir, so two layers with
# same-named files cannot collide and a .o name tells you where it came from.
_OBJS = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(PCS_SRCS_C))

DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
-include $(_OBJS:.o=.d)

.PHONY: _build
_build: $(_BUILD_DIR)/libpcs.a
	@echo "[PCS] _build done: $(_ARCH)  ($(words $(_OBJS)) objects)"

$(_BUILD_DIR)/libpcs.a: $(_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

# The pattern keeps the full relative path, which is what lets the two layers
# coexist.  -I of the source's own directory is added per file so a source can
# include its neighbour the way uiox_kix_scpcs.h includes uiox_kix_psa_process.h.
$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(_CFLAGS_ALL) -I$(dir $<) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

# =============================================================================
# Convenience targets
# =============================================================================

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo ""
	@echo "[PCS] all builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; rm -rf $(MFDIR)build/arm64
clean-arm32:   ; rm -rf $(MFDIR)build/arm32
clean-riscv64: ; rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; rm -rf $(MFDIR)build/x86_64
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] all build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64 info
info-arm64:
	@echo "HOST=$(UNAME_S) ARCH=arm64   CC=$(ARM64_CC)"
info-arm32:
	@echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC)"
info-riscv64:
	@echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC)"
info-x86_64:
	@echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC)"

# ── info — what this Makefile will actually build ────────────────────────────
# The one target worth running first, because it prints the file list rather
# than assuming it.  If a path is wrong, you see it here instead of at link.
.PHONY: info
info:
	@echo "PSADIR   = $(PSADIR)"
	@echo "SCPCSDIR = $(SCPCSDIR)"
	@echo ""
	@echo "40_psa sources:"
	@for f in $(PSADIR)/src/uiox_kix_psa_*.c ; do \
	    [ -f "$$f" ] && echo "  OK   $$f" || echo "  MISS $$f" ; done
	@echo ""
	@echo "50_scpcs sources:"
	@for f in $(wildcard $(SCPCSDIR)/src/uiox_kix_scpcs_*.c) ; do \
	    [ -f "$$f" ] && echo "  OK   $$f" || echo "  MISS $$f" ; done
	@echo ""
	@echo "object count: $(words $(_OBJS))"
