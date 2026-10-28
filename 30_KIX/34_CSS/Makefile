# =============================================================================
# 30_KIX/34_CSS/Makefile  — v1.6.0
#
# Communication and Sensor Services — the layer a stack calls when it needs
# a DD driver.  NOT a device-driver layer and NOT a filesystem peer:
# no probe of its own, so this layer carries no device code of its own —
# it carries the stacks (proto + subsys) FOR the classes 20_DD/10_DI drive.
#
# Where a class's code lives here:
#   34_CSS/<group>/<class>/uiox_<cls>_proto.c    the class's protocol
#   34_CSS/<group>/<class>/uiox_<cls>_subsys.c   its state machine + tick
# Mirroring 03_NonSensors/emmc/, which is the shape the rest follow.
#
# Outputs: build/<arch>/libcss.a
#
# ── v1.6.0 changes ────────────────────────────────────────────────────
#   · PER-MODULE ENABLE FLAGS.  A class's stack is built only when its
#     ENABLE_<CLASS> is `yes`, the way 10_DI/Makefile gates SUBDIRS.  The
#     flag list is declared once and forwarded to the re-invoked _build by
#     the same $(.VARIABLES) derivation 10_DI v1.3.0 uses, so a new module
#     is one ?= line plus one ifeq — and the arch rule cannot be forgotten.
#   · CSS_SUBDIRS is now the single source of what is built: SRCS globs it,
#     vpath derives from it, and info prints it.  The three cannot drift.
#   · A flag with no matching directory contributes nothing and would be
#     silent; `make info` prints the count so that is visible.
#
# ── v1.5.0 (kept) ─────────────────────────────────────────────────────
#   · DRV_CLASS_DIRS added and added to COMMON_CFLAGS.  The 00_drvbuff pool
#     headers are a REAL dependency of the DD layer:
#     20_DD/01_Com/bt/uiox_bt_if.h:11 includes "uiox_bt_buf.h", and that
#     header lives in 32_FS/00_drvbuff/01_Com/bt/.  v1.4.0 derived DD and
#     DI but not this one, so the first CSS compile failed with
#     `uiox_bt_buf.h file not found`.
#
# ── v1.4.0 (kept) ─────────────────────────────────────────────────────
#   · Class include paths DERIVED, not enumerated.
#   · vpath block added, matching 32_FS.
#   · FIXED: the -include line had an unbalanced $( ), which made the whole
#     line inert and silently stopped stale-object tracking.
#
# @version 1.6.0  @date 2026-10-28
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR  := $(abspath $(MFDIR)../common)
DSSDIR     := $(abspath $(MFDIR)../30_DSS)
DRVBUFDIR  := $(abspath $(MFDIR)../32_FS/00_drvbuff)
BUILD_BASE  = $(MFDIR)build
INCDIR      = $(MFDIR)include
FWHAL       = $(abspath $(MFDIR)../../02_FwHal)
BSPDIR      = $(abspath $(MFDIR)../../10_BSP)
SOCINC      = $(BSPDIR)/03_SoC/include

CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
DEVICE_DEFS ?=
_ARCH       ?= arm64

# ── module switches — the ONE place a flag is declared ────────────────
# Matched by name to a class directory under the three groups.  Defaults
# are `yes` for everything that exists today, matching 10_DI and 20_DD.
# A class that is off is excluded from SRCS and from vpath, so its archive
# members and its include path both disappear together.
ENABLE_BT        ?= yes
ENABLE_ETH       ?= yes
ENABLE_GPU       ?= no
ENABLE_HDMI      ?= yes
ENABLE_MONITOR   ?= no
ENABLE_TB4       ?= no
ENABLE_USB       ?= yes
ENABLE_WIFI      ?= yes
ENABLE_CAMERA    ?= yes
ENABLE_MIC       ?= yes
ENABLE_TOUCHPWD  ?= yes
ENABLE_SPEAKER   ?= yes

# The names forwarded to _build.  Derived from the variables actually
# declared above, so the two cannot drift — a typo like 10_DI v1.2.1's
# `ENABLE_SPEAKER=$(ENABLE_UART)` is impossible by construction.
ENABLE_FLAGS := $(filter ENABLE_%,$(.VARIABLES))

_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

# ── module → directory map ────────────────────────────────────────────
# One row per module:  <group>/<class>  <flag-suffix>
# The flag name is ENABLE_<upper(class)>; the directory is the same word
# as the class's own file prefix (uiox_<cls>_proto.c).  Keeping the pair
# in one table means SRCS, vpath and the ifeq gates cannot disagree.
#
# 01_Com
CSS_bt        := 01_Com/bt
CSS_eth       := 01_Com/eth
CSS_gpu       := 01_Com/gpu
CSS_hdmi      := 01_Com/hdmi
CSS_monitor   := 01_Com/monitor
CSS_tb4       := 01_Com/tb4
CSS_usb       := 01_Com/usb
# 02_Sensors
CSS_WiFi      := 02_Sensors/WiFi
CSS_camera    := 02_Sensors/camera
CSS_mic       := 02_Sensors/mic
CSS_touchpwd  := 02_Sensors/touchpwd
# 03_NonSensors
CSS_speaker   := 03_NonSensors/speaker

# ── the module list, gated by its flag ────────────────────────────────
# Each ifeq names one module.  A directory whose flag is `no` is absent
# from CSS_SUBDIRS, so it leaves SRCS and the -I set in the same step —
# the property that makes these gates safe to flip.
CSS_SUBDIRS :=

ifeq ($(ENABLE_BT),yes)
  CSS_SUBDIRS += $(CSS_bt)
endif
ifeq ($(ENABLE_ETH),yes)
  CSS_SUBDIRS += $(CSS_eth)
endif
ifeq ($(ENABLE_GPU),yes)
  CSS_SUBDIRS += $(CSS_gpu)
endif
ifeq ($(ENABLE_HDMI),yes)
  CSS_SUBDIRS += $(CSS_hdmi)
endif
ifeq ($(ENABLE_MONITOR),yes)
  CSS_SUBDIRS += $(CSS_monitor)
endif
ifeq ($(ENABLE_TB4),yes)
  CSS_SUBDIRS += $(CSS_tb4)
endif
ifeq ($(ENABLE_USB),yes)
  CSS_SUBDIRS += $(CSS_usb)
endif
ifeq ($(ENABLE_WIFI),yes)
  CSS_SUBDIRS += $(CSS_WiFi)
endif
ifeq ($(ENABLE_CAMERA),yes)
  CSS_SUBDIRS += $(CSS_camera)
endif
ifeq ($(ENABLE_MIC),yes)
  CSS_SUBDIRS += $(CSS_mic)
endif
ifeq ($(ENABLE_TOUCHPWD),yes)
  CSS_SUBDIRS += $(CSS_touchpwd)
endif
ifeq ($(ENABLE_SPEAKER),yes)
  CSS_SUBDIRS += $(CSS_speaker)
endif

# ── the three sides of every class — derived, not enumerated ──────────
#   DD  30_DSS/20_DD/<group>/<cls>/uiox_<cls>_if.h    the class interface
#   DI  30_DSS/10_DI/<group>/<cls>/uiox_<cls>_hw.h    the register layer
#   DRV 32_FS/00_drvbuff/<group>/<cls>/uiox_<cls>_buf.h   the pool header
#
# The DRV side is not optional: 20_DD/01_Com/bt/uiox_bt_if.h includes
# uiox_bt_buf.h, and that header lives under 00_drvbuff.
#
# These cover EVERY class on disk, not only the enabled ones: a CSS stack
# for one class may name another class's header (eth's may name a phy's),
# and a missing include path is a hard error.  Only 10_DI and 20_DD are
# derived this widely — the CSS side is gated by CSS_SUBDIRS above.
DD_CLASS_DIRS  := $(wildcard $(DSSDIR)/20_DD/01_Com/*/)     \
                  $(wildcard $(DSSDIR)/20_DD/02_Sensors/*/) \
                  $(wildcard $(DSSDIR)/20_DD/03_NonSensors/*/)
DI_CLASS_DIRS  := $(wildcard $(DSSDIR)/10_DI/01_Com/*/)     \
                  $(wildcard $(DSSDIR)/10_DI/02_Sensors/*/) \
                  $(wildcard $(DSSDIR)/10_DI/03_NonSensors/*/)
DRV_CLASS_DIRS := $(wildcard $(DRVBUFDIR)/01_Com/*/)        \
                  $(wildcard $(DRVBUFDIR)/02_Sensors/*/)    \
                  $(wildcard $(DRVBUFDIR)/03_NonSensors/*/)

# The enabled class directories, for the CSS side only.
CSS_CLASS_DIRS := $(addprefix $(MFDIR),$(CSS_SUBDIRS)/)

# -nostdinc with klibc supplying printf/memset/memcpy and the errno defines.
COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror          \
  -ffreestanding -fno-builtin             \
  -fno-stack-protector                    \
  -ffunction-sections -fdata-sections     \
  -nostdinc -Os -g                        \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)           \
  $(DEVICE_DEFS)                          \
  -I$(INCDIR)                             \
  -I$(COMMONDIR)                          \
  -I$(DSSDIR)/include                     \
  -I$(DSSDIR)/10_DI/include               \
  -I$(MFDIR)../33_PCS/include             \
  -I$(MFDIR)../33_PCS/40_psa/include      \
  -I$(DRVBUFDIR)/include                  \
  -I$(FWHAL)/include                      \
  -I$(SOCINC)                             \
  $(addprefix -I,$(CSS_CLASS_DIRS))       \
  $(addprefix -I,$(DD_CLASS_DIRS))        \
  $(addprefix -I,$(DI_CLASS_DIRS))        \
  $(addprefix -I,$(DRV_CLASS_DIRS))

# ── sources — only the enabled modules ────────────────────────────────
# $*/*.c is TWO levels, matching 03_NonSensors/emmc/uiox_emmc_proto.c —
# one wildcard level per group, not a recursive glob.  Iterating over
# CSS_SUBDIRS rather than globbing the groups is what makes a flag bite.
SRCS := $(foreach d,$(CSS_SUBDIRS),$(wildcard $(MFDIR)$(d)/*.c))
SRCS += $(wildcard $(MFDIR)src/*.c)

OBJS := $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(SRCS))
LIB  := $(_BUILD_DIR)/libcss.a

# vpath keyed on the same list, so a disabled module's .c is neither
# compiled nor found by the implicit rule.
vpath %.c \
  $(MFDIR)src \
  $(addprefix $(MFDIR),$(CSS_SUBDIRS))

ARFLAGS := rcs

# =============================================================================
# Arch entry points — re-invoke with _ARCH=$@ so $(_BUILD_DIR) is defined.
# =============================================================================
.PHONY: arm64 arm32 riscv64 x86_64

arm64 arm32 riscv64 x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build  \
	    _ARCH=$@                        \
	    CC="$(CC)" AR="$(AR)"           \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)"    \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)"    \
	    DEVICE_DEFS="$(DEVICE_DEFS)"    \
	    $(foreach f,$(ENABLE_FLAGS),$(f)=$($(f)))

.PHONY: _build
_build: $(LIB)
	@echo "[CSS] $(_BUILD_DIR)/libcss.a  ($(words $(SRCS)) sources, $(words $(CSS_SUBDIRS)) modules)"
	@echo "[CSS]  $(CSS_SUBDIRS)"

# ── the archive ───────────────────────────────────────────────────────
# An empty OBJS is a no-op, not an error.  `ar rcs` with no members prints
# its usage and exits non-zero, which would make "every module disabled" or
# "this layer has no code yet" look like a broken build.
$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	@if [ -z "$(OBJS)" ]; then \
	    echo "  [AR]  skipped — no CSS modules enabled"; \
	    touch $@; \
	  else \
	    $(AR) $(ARFLAGS) $@ $(OBJS); \
	    echo "  [AR]  $@"; \
	  fi

# Keys on $(MFDIR)%.c so it matches what SRCS globbed.
$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(COMMON_CFLAGS) -MMD -MP -c $< -o $@

-include $(wildcard $(_BUILD_DIR)/*/*/*.d) $(wildcard $(_BUILD_DIR)/*/*.d)

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	@rm -rf $(BUILD_BASE)
	@echo "[CLN] 34_CSS build outputs removed"

# ── srclist — what the globs actually resolved to ─────────────────────
.PHONY: srclist
srclist:
	@echo "[CSS] sources to build:"
	@for f in $(SRCS); do echo "  $$f"; done
	@echo "[CSS] total: $(words $(SRCS))"

# ── info — flags, modules, sources ────────────────────────────────────
# A flag whose class has no directory contributes nothing silently; the
# module and source counts make that visible, the same reason 32_FS
# carries srclist.
.PHONY: info
info:
	@echo "CSS        : $(MFDIR)"
	@echo "arch       : $(_ARCH)  CC=$(CC)"
	@echo "modules    : $(words $(CSS_SUBDIRS))"
	@echo "subdirs    : $(CSS_SUBDIRS)"
	@echo "sources    : $(words $(SRCS)) -> $(LIB)"
	@echo "class dirs : $(words $(CSS_CLASS_DIRS)) CSS + $(words $(DD_CLASS_DIRS)) DD + $(words $(DI_CLASS_DIRS)) DI + $(words $(DRV_CLASS_DIRS)) DRV"
	@echo "flags      : $(foreach f,$(ENABLE_FLAGS),$(f)=$($(f)) )"
	@echo "srclist    : make srclist"
