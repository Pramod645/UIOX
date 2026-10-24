# =============================================================================
# 30_KIX/30_DSS/Makefile  — v2.1.0
#
# Builds BOTH halves of Bach's device band:
#   10_DI  hardware control — declares the ops, never performs a register write
#   20_DD  the drivers       — implement them
#   src/   the registry      — Bach's switch table (devtable, devparse, dss_init)
#
# Two archives, in dependency order:
#   build/<arch>/libdi.a   (10_DI + src)  ← built first; 20_DD includes its headers
#   build/<arch>/libdev.a  (20_DD)        ← built second
#
# v2.1.0 — DRVBUFF_INCLUDES added, and the subdirectory variables moved ABOVE
#          COMMON_CFLAGS so `-I$(DI_INCLUDES)` no longer expands to a bare `-I`.
#          The pool layer moved to 32_FS/00_drvbuff (parallel to 00_bcache).
#
# @version 2.1.0  @date 2026-10-15
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  = $(MFDIR)build

CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
DEVICE_DEFS ?=
_ARCH       ?= arm64

# ── device switches (unchanged from 20_DD v1.3.2) ─────────────────────────
ENABLE_WIFI     ?= yes
ENABLE_BT       ?= yes
ENABLE_ETH      ?= yes
ENABLE_USB      ?= yes
ENABLE_HDMI     ?= yes
ENABLE_TB4      ?= yes
ENABLE_MONITOR  ?= yes
ENABLE_GPU      ?= yes
ENABLE_CAMERA   ?= yes
ENABLE_MIC      ?= yes
ENABLE_KBD      ?= yes
ENABLE_MOUSE    ?= yes
ENABLE_ALS      ?= yes
ENABLE_THERMAL  ?= yes
ENABLE_TOUCHPWD ?= yes
ENABLE_BMS      ?= yes
ENABLE_CHG      ?= yes
ENABLE_FAN      ?= yes
ENABLE_PMIC     ?= yes
ENABLE_RTC      ?= yes
ENABLE_SPEAKER  ?= yes
ENABLE_EMMC     ?= yes
ENABLE_UART     ?= yes

_BUILD_DIR := $(BUILD_BASE)/$(_ARCH)

# ── the two layers ────────────────────────────────────────────────────────
DI_DIR  := 10_DI
DD_DIR  := 20_DD

DI_LIB  := $(_BUILD_DIR)/libdi.a
DD_LIB  := $(_BUILD_DIR)/libdev.a

# ══════════════════════════════════════════════════════════════════════════
# Subdirectory sets — defined BEFORE COMMON_CFLAGS so the -I variables are
# expanded when they are USED, not when the line is parsed.
# ══════════════════════════════════════════════════════════════════════════

# 10_DI is built whole: a hardware-control interface has no per-device switch.
DI_SUBDIRS := \
  01_Com/bt            01_Com/eth          01_Com/usb          \
  01_Com/hdmi          01_Com/tb4          01_Com/monitor      \
  01_Com/gpu           02_Sensors/WiFi     02_Sensors/camera   \
  02_Sensors/mic       02_Sensors/keyboard 02_Sensors/mouse    \
  02_Sensors/als       02_Sensors/thermal  02_Sensors/touchpwd \
  03_NonSensors/bms    03_NonSensors/chg    03_NonSensors/fan   \
  03_NonSensors/pmic   03_NonSensors/rtc    03_NonSensors/speaker \
  03_NonSensors/emmc   01_Com/uart

DI_INCLUDES := -I$(MFDIR)$(DI_DIR)/include \
               $(foreach d,$(DI_SUBDIRS),-I$(MFDIR)$(DI_DIR)/$(d))

# ── 20_DD: device-conditional (unchanged from v1.3.2) ─────────────────────
DD_SUBDIRS :=
ifeq ($(ENABLE_WIFI),yes)
  DD_SUBDIRS += 02_Sensors/WiFi
endif
ifeq ($(ENABLE_BT),yes)
  DD_SUBDIRS += 01_Com/bt
endif
ifeq ($(ENABLE_ETH),yes)
  DD_SUBDIRS += 01_Com/eth
endif
ifeq ($(ENABLE_USB),yes)
  DD_SUBDIRS += 01_Com/usb
endif
ifeq ($(ENABLE_HDMI),yes)
  DD_SUBDIRS += 01_Com/hdmi
endif
ifeq ($(ENABLE_TB4),yes)
  DD_SUBDIRS += 01_Com/tb4
endif
ifeq ($(ENABLE_MONITOR),yes)
  DD_SUBDIRS += 01_Com/monitor
endif
ifeq ($(ENABLE_CAMERA),yes)
  DD_SUBDIRS += 02_Sensors/camera
endif
ifeq ($(ENABLE_MIC),yes)
  DD_SUBDIRS += 02_Sensors/mic
endif
ifeq ($(ENABLE_KBD),yes)
  DD_SUBDIRS += 02_Sensors/keyboard
endif
ifeq ($(ENABLE_MOUSE),yes)
  DD_SUBDIRS += 02_Sensors/mouse
endif
ifeq ($(ENABLE_ALS),yes)
  DD_SUBDIRS += 02_Sensors/als
endif
ifeq ($(ENABLE_THERMAL),yes)
  DD_SUBDIRS += 02_Sensors/thermal
endif
ifeq ($(ENABLE_TOUCHPWD),yes)
  DD_SUBDIRS += 02_Sensors/touchpwd
endif
ifeq ($(ENABLE_BMS),yes)
  DD_SUBDIRS += 03_NonSensors/bms
endif
ifeq ($(ENABLE_CHG),yes)
  DD_SUBDIRS += 03_NonSensors/chg
endif
ifeq ($(ENABLE_FAN),yes)
  DD_SUBDIRS += 03_NonSensors/fan
endif
ifeq ($(ENABLE_PMIC),yes)
  DD_SUBDIRS += 03_NonSensors/pmic
endif
ifeq ($(ENABLE_RTC),yes)
  DD_SUBDIRS += 03_NonSensors/rtc
endif
ifeq ($(ENABLE_SPEAKER),yes)
  DD_SUBDIRS += 03_NonSensors/speaker
endif
ifeq ($(ENABLE_EMMC),yes)
  DD_SUBDIRS += 03_NonSensors/emmc
endif
ifeq ($(ENABLE_UART),yes)
  DD_SUBDIRS += 01_Com/uart
endif

# ══════════════════════════════════════════════════════════════════════════
# 32_FS/00_drvbuff — the driver-side pools
#
# A SIBLING of 00_bcache, in the file subsystem.  Bach draws ONE buffer
# box between the file subsystem and the drivers; UIOX needs two, because
# its classes have two buffer shapes:
#
#   00_bcache    keyed (dev, blkno), evictable     — family 1 BLOCK
#   00_drvbuff   unkeyed, in-flight ownership      — families 2, 3, 5, 6
#
# The driver already reaches 32_FS — that is what bcache_plat_read_block()
# is — so this is the same edge extended, not a new direction.
#
# NOT a mirror of DD_SUBDIRS: family 4 (als thermal bms chg fan pmic rtc)
# reads a register on demand and has nothing in flight to hold.
# ══════════════════════════════════════════════════════════════════════════
DRVBUFF_INCLUDES := \
  -I$(MFDIR)../32_FS/00_drvbuff/01_Com/bt             \
  -I$(MFDIR)../32_FS/00_drvbuff/01_Com/eth            \
  -I$(MFDIR)../32_FS/00_drvbuff/01_Com/hdmi           \
  -I$(MFDIR)../32_FS/00_drvbuff/01_Com/monitor        \
  -I$(MFDIR)../32_FS/00_drvbuff/01_Com/tb4            \
  -I$(MFDIR)../32_FS/00_drvbuff/01_Com/usb            \
  -I$(MFDIR)../32_FS/00_drvbuff/02_Sensors/WiFi       \
  -I$(MFDIR)../32_FS/00_drvbuff/02_Sensors/camera     \
  -I$(MFDIR)../32_FS/00_drvbuff/02_Sensors/keyboard   \
  -I$(MFDIR)../32_FS/00_drvbuff/02_Sensors/mic        \
  -I$(MFDIR)../32_FS/00_drvbuff/02_Sensors/mouse      \
  -I$(MFDIR)../32_FS/00_drvbuff/02_Sensors/touchpwd   \
  -I$(MFDIR)../32_FS/00_drvbuff/03_NonSensors/emmc    \
  -I$(MFDIR)../32_FS/00_drvbuff/03_NonSensors/speaker

# ── include paths ─────────────────────────────────────────────────────────
# 10_DI first: 20_DD includes the interface headers and must see them.
COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror           \
  -ffreestanding -fno-builtin              \
  -fno-stack-protector                     \
  -ffunction-sections -fdata-sections      \
  -nostdinc -Os -g                         \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)            \
  $(DEVICE_DEFS)                           \
  -I$(MFDIR)../common                      \
  -I$(MFDIR)include                        \
  -I$(MFDIR)$(DI_DIR)/include              \
  -I$(MFDIR)../32_FS/00_bcache/include     \
  -I$(MFDIR)../../02_FwHal/include         \
  -I$(MFDIR)../../10_BSP/03_SoC/include    \
  -I$(MFDIR)$(DD_DIR)                      \
  $(DI_INCLUDES)                           \
  $(DRVBUFF_INCLUDES)

# ── sources ───────────────────────────────────────────────────────────────
# 10_DI builds whole — a hardware-control interface has no per-device switch.
# 30_DSS/include and 30_DSS/src (the registry) build into libdi.a as well,
# because uiox_devclass.h/uiox_devtable.c ARE the switch table both halves need.
DI_SRCS := $(wildcard $(MFDIR)$(DI_DIR)/*.c) \
           $(wildcard $(MFDIR)$(DI_DIR)/*/[0-9]*/*.c) \
           $(wildcard $(MFDIR)src/*.c)

DD_SRCS := $(foreach d,$(DD_SUBDIRS),$(wildcard $(MFDIR)$(DD_DIR)/$(d)/*.c))

DI_OBJS := $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(DI_SRCS))
DD_OBJS := $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(DD_SRCS))

# =============================================================================
# Arch entry points
# =============================================================================

ARCHES := arm64 arm32 riscv64 x86_64

.PHONY: all
all: $(ARCHES)

# Optional: `make all-report` prints what each arch produced.
.PHONY: all-report
all-report: $(ARCHES)
	@for a in $(ARCHES); do \
	    printf '%-8s ' "$$a"; \
	    ls -1 $(BUILD_BASE)/$$a/*.a 2>/dev/null | tr '\n' ' '; \
	    echo; \
	done

arm64 arm32 riscv64 x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build      \
	    _ARCH=$@                            \
	    CC="$(CC)" AR="$(AR)"               \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)"        \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)"        \
	    DEVICE_DEFS="$(DEVICE_DEFS)"        \
	    ENABLE_WIFI=$(ENABLE_WIFI)          \
	    ENABLE_BT=$(ENABLE_BT)              \
	    ENABLE_ETH=$(ENABLE_ETH)            \
	    ENABLE_USB=$(ENABLE_USB)            \
	    ENABLE_HDMI=$(ENABLE_HDMI)          \
	    ENABLE_TB4=$(ENABLE_TB4)            \
	    ENABLE_MONITOR=$(ENABLE_MONITOR)    \
	    ENABLE_GPU=$(ENABLE_GPU)            \
	    ENABLE_CAMERA=$(ENABLE_CAMERA)      \
	    ENABLE_MIC=$(ENABLE_MIC)            \
	    ENABLE_KBD=$(ENABLE_KBD)            \
	    ENABLE_MOUSE=$(ENABLE_MOUSE)        \
	    ENABLE_ALS=$(ENABLE_ALS)            \
	    ENABLE_THERMAL=$(ENABLE_THERMAL)    \
	    ENABLE_TOUCHPWD=$(ENABLE_TOUCHPWD)  \
	    ENABLE_BMS=$(ENABLE_BMS)            \
	    ENABLE_CHG=$(ENABLE_CHG)            \
	    ENABLE_FAN=$(ENABLE_FAN)            \
	    ENABLE_PMIC=$(ENABLE_PMIC)          \
	    ENABLE_RTC=$(ENABLE_RTC)            \
	    ENABLE_SPEAKER=$(ENABLE_SPEAKER)    \
		ENABLE_SPEAKER=$(ENABLE_UART)       \
	    ENABLE_EMMC=$(ENABLE_EMMC)

# ── the build: 10_DI first, then 20_DD, because 20_DD includes 10_DI's headers
.PHONY: _build
_build: $(DI_LIB) $(DD_LIB)
	@echo "[DSS] 10_DI  $(_BUILD_DIR)/libdi.a   ($(words $(DI_SRCS)) sources)"
	@echo "[DSS] 20_DD  $(_BUILD_DIR)/libdev.a  ($(words $(DD_SUBDIRS)) subsystems)"

$(DI_LIB): $(DI_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	@echo "[AR]  $@"

$(DD_LIB): $(DD_OBJS) | $(DI_LIB)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	@echo "[AR]  $@"

# ── pattern rules: one per layer, so $* resolves correctly ────────────────
$(_BUILD_DIR)/$(DI_DIR)/%.o: $(MFDIR)$(DI_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC-DI]  $<"
	$(CC) $(COMMON_CFLAGS) \
	    -I$(MFDIR)$(dir $*) \
	    -MMD -MP -c $< -o $@

$(_BUILD_DIR)/src/%.o: $(MFDIR)src/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC-REG]  $<"
	$(CC) $(COMMON_CFLAGS) \
	    -MMD -MP -c $< -o $@

$(_BUILD_DIR)/$(DD_DIR)/%.o: $(MFDIR)$(DD_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC-DD]  $<"
	$(CC) $(COMMON_CFLAGS) \
	    -I$(MFDIR)$(DD_DIR)/$(dir $*) \
	    -MMD -MP -c $< -o $@

-include $(wildcard $(BUILD_BASE)/$(_ARCH)/*.d)

.PHONY: clean
clean:
	rm -rf $(BUILD_BASE)

# ── introspection, so the umbrella Makefile can ask what to link ──────────
.PHONY: info srclist
info:
	@echo "DSS        : $(MFDIR)"
	@echo "10_DI      : $(words $(DI_SRCS)) sources -> $(DI_LIB)"
	@echo "20_DD      : $(words $(DD_SUBDIRS)) subsystems, $(words $(DD_SRCS)) sources -> $(DD_LIB)"
	@echo "drvbuff -I : $(words $(DRVBUFF_INCLUDES)) entries"
	@echo "subdirs    : $(DD_SUBDIRS)"

srclist:
	@echo "$(DI_SRCS)" | tr ' ' '\n'
	@echo "$(DD_SRCS)" | tr ' ' '\n'
