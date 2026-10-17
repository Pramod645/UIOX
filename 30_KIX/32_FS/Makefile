# =============================================================================
# 30_KIX/32_FS/Makefile  — v2.3.0
#
# Builds the four layers and one peer into one archive:
#     build/<arch>/libfs.a
#
# Usage:
#   make arm64 / arm32 / riscv64 / x86_64
#   make all
#   make clean[-<arch>]
#   make info[-<arch>]
#   make srclist          ← what the globs actually resolved to
#
# ── the four layers, all children of 32_FS ─────────────────────────────
#   00_bcache/src/*.c       block buffer cache   (Bach Ch.3)
#   01_fsa/src/*.c          inode level          (Bach Ch.4)
#   10_scfs/src/*.c         syscall bodies       (Bach Ch.5)
#   10_unfs/src/*.c         on-disk format + storage backend
#
# ── and one PEER, not a fifth layer ────────────────────────────────────  ◀ v2.3.0
#   02_jrnal/src/*.c        write-ahead log
#
#   02_jrnal is NOT above the four.  It is a mechanism the layers CALL —
#   iupdate() journals the inode block, unfs_alloc_run() the block bitmap,
#   unfs_alloc_inode_bit() the inode bitmap, dir_add() the directory
#   block, writei_at() the data block under MODE_DATA.  Every one of those
#   hooks is guarded by a NULL test on uiox_jr_ctx_for(dev), so a
#   filesystem mounted without journaling behaves exactly as before.
#
#   That is why 10_unfs/include may include uiox_kix_jrnl.h without
#   breaking the downward-only rule below: the include is upward in
#   DIRECTORY terms and lateral in terms of WHO CALLS WHOM.  The test is
#   direction of calls, not depth in the tree.
#
# ── the include graph ──────────────────────────────────────────────────
#   00_bcache  →  its own include/
#   01_fsa     →  its own include/  +  00_bcache/include
#                 +  10_unfs/include
#   10_scfs    →  its own include/  +  01_fsa + 00_bcache + 10_unfs
#   10_unfs    →  its own include/  +  00_bcache/include + 01_fsa/include
#                 +  02_jrnal/include        ◀ lateral — see above
#   02_jrnal   →  its own include/  +  01_fsa/include  (inode_block_of)
#
#   The pair 10_unfs ⇄ 02_jrnal is mutual, and neither is a violation.
#   Both headers must be self-sufficient — the mutual dependency lives in
#   the .c files only.  If either header includes the other, that is the
#   cycle to break.
#
# ── v2.3.0 changes ────────────────────────────────────────────────────
#   · 02_jrnal/src added to FS_SRCS_C and to vpath.
#   · $(MFDIR)src added to FS_SRCS_C and to vpath.  uiox_fs_init.c lives
#     there and was NEVER in the build — the globs covered four layer
#     directories only, so the file that registers everything and calls
#     uiox_jr_register() was not compiled into libfs.a at all.
#   · 02_FwHal/include added to COMMON_INCLUDES.  Every journal .c
#     declares `extern void uiox_fw_printf(...)`, and that is where the
#     definition is (02_FwHal/include/uiox_fw.h:73).  Without the path,
#     all three journal sources fail at the first call.
#
# ── v2.2.0 changes ────────────────────────────────────────────────────
#   · 10_unfs/src/*.c added, and 10_unfs moved to the FRONT.
#   · 10_scfs gained 10_unfs/include.
#
# =============================================================================

# ── Roots ─────────────────────────────────────────────────────────────
MFDIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(abspath $(MFDIR)../common)

BCDIR    := $(MFDIR)00_bcache
FSA_DIR  := $(MFDIR)01_fsa
SCFS_DIR := $(MFDIR)10_scfs
UNFS_DIR := $(MFDIR)10_unfs
JRNL_DIR := $(MFDIR)02_jrnal
FWHAL    := $(abspath $(MFDIR)../../02_FwHal)
DRVBUFDIR := $(MFDIR)00_drvbuff

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
# v2.1.0: unfs_format.h lives in 10_unfs/include, so that directory must
# be searched.  v2.2.0 leaves this list alone — the layer's sources are
# now globbed in, and they resolve through the same -I set.

# ── 10_DI class directories ───────────────────────────────────────────
# 00_drvbuff's pool headers include the driver class headers directly
# (uiox_bt_buf.h → uiox_bt_hw.h, and ten more), so the class dirs must be
# on the search path.  The wildcard covers every class under 10_DI and
# picks up new ones without editing this list.
#DI_CLASS_DIRS := $(wildcard $(MFDIR)../30_DSS/10_DI/*/*/)
DI_CLASS_DIRS := $(wildcard $(MFDIR)../30_DSS/10_DI/01_Com/*/) \
                 $(wildcard $(MFDIR)../30_DSS/10_DI/02_Sensors/*/) \
                 $(wildcard $(MFDIR)../30_DSS/10_DI/03_NonSensors/*/)


COMMON_INCLUDES := \
  -I$(COMMONDIR)          \
  -I$(MFDIR)include       \
  -I$(MFDIR)../30_DSS/include \
  -I$(BCDIR)/include      \
  -I$(FSA_DIR)/include    \
  -I$(SCFS_DIR)/include   \
  -I$(UNFS_DIR)/include   \
  -I$(JRNL_DIR)/include   \
  -I$(FWHAL)/include      \
  -I$(DRVBUFDIR)/include  \
  -I$(MFDIR)../30_DSS/10_DI/include \
  $(addprefix -I,$(DI_CLASS_DIRS)) \
  -I$(SOCINC)

# ── the four layers, in dependency order ──────────────────────────────
# 10_unfs defines the on-disk format and its storage entry points;
# 00_buffcache supplies buffers to 01_fsa, which 10_scfs calls into.
FS_SRCS_C :=
FS_SRCS_C += $(wildcard $(UNFS_DIR)/src/*.c)    # v2.2.0 — was header-only
FS_SRCS_C += $(wildcard $(BCDIR)/src/*.c)
FS_SRCS_C += $(wildcard $(FSA_DIR)/src/*.c)
FS_SRCS_C += $(wildcard $(SCFS_DIR)/src/*.c)
FS_SRCS_C += $(wildcard $(JRNL_DIR)/src/*.c)          # ◀ v2.3.0
FS_SRCS_C += $(wildcard $(MFDIR)src/*.c)              # ◀ v2.3.0
FS_SRCS_C += $(wildcard $(DRVBUFDIR)/*/*/*.c)       # ◀ the pools
# ── the bcache platform pair — link exactly ONE ───────────────────────
# $(wildcard $(BCDIR)/src/*.c) above pulls in BOTH bcache_plat_dram.c
# (the DRAM stand-in) and bcache_plat_dev.c (the live registry path).
# Each defines all four hooks, so ar accepts both and the LINKER picks
# one arbitrarily.  These lines make the choice explicit.
#FS_SRCS_C := $(filter-out $(BCDIR)/src/bcache_plat_dev.c, $(FS_SRCS_C))
FS_SRCS_C := $(filter-out $(BCDIR)/src/bcache_plat_dram.c, $(FS_SRCS_C))


vpath %.c \
  $(UNFS_DIR)/src \
  $(BCDIR)/src    \
  $(FSA_DIR)/src  \
  $(SCFS_DIR)/src \
  $(JRNL_DIR)/src \
  $(MFDIR)src     \
  $(DRVBUFDIR)/01_Com/bt        \
  $(DRVBUFDIR)/01_Com/eth       \
  $(DRVBUFDIR)/01_Com/gpu       \
  $(DRVBUFDIR)/01_Com/hdmi      \
  $(DRVBUFDIR)/01_Com/monitor   \
  $(DRVBUFDIR)/01_Com/tb4       \
  $(DRVBUFDIR)/01_Com/usb       \
  $(DRVBUFDIR)/02_Sensors/WiFi      \
  $(DRVBUFDIR)/02_Sensors/camera    \
  $(DRVBUFDIR)/02_Sensors/keyboard  \
  $(DRVBUFDIR)/02_Sensors/mic       \
  $(DRVBUFDIR)/02_Sensors/mouse     \
  $(DRVBUFDIR)/02_Sensors/touchpwd  \
  $(DRVBUFDIR)/03_NonSensors/emmc   \
  $(DRVBUFDIR)/03_NonSensors/speaker

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
	@echo "  UNFS_DIR = $(UNFS_DIR)"
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
