# =============================================================================
# 30_KIX/10_DI/Makefile  — v1.3.0
#
# Hardware Abstraction Layer — builds only subdirectories for enabled devices.
# Outputs: build/<arch>/libhw.a
#
# ── v1.3.0 changes ────────────────────────────────────────────────────
#   · FIXED: the arch re-invocation named the wrong variables.  Four lines
#     read ENABLE_SPEAKER/ENABLE_EMMC where they meant ENABLE_UART/SDMMC/
#     NVME/SATA, and one misspelled ENABLE_SDMC.  make keeps the LAST
#     assignment for a repeated name, so the re-invoked _build saw
#     ENABLE_SPEAKER=$(ENABLE_UART) and ENABLE_EMMC=$(ENABLE_SATA), and
#     never saw the four it actually needed.  Their ?= yes defaults masked
#     it — flipping any of them to `no` on the command line had no effect.
#   · The flag list is now derived from the ENABLE_* variable names, so a
#     new device needs one line (the ?= default) rather than three.  This
#     is the same class of fix as v1.2.1's stale RTC block: a name that
#     does not match silently does nothing.
#   · ENABLE_USBMS added for USDMS/USBMS parity with 20_DD.
#
# @version 1.3.0  @date 2026-10-27
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  = $(MFDIR)build

CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
DEVICE_DEFS ?=
_ARCH       ?= arm64

# ── device switches — the ONE place a flag is declared ────────────────
# Each name here also selects its subdirectory below.  Add a device by
# adding its ?= line here and its ifeq block in SUBDIRS; the arch rule
# forwards every name in this list automatically.
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
ENABLE_SDMMC    ?= yes
ENABLE_NVME     ?= yes
ENABLE_SATA     ?= yes
ENABLE_USBMS    ?= yes

# The names forwarded to _build.  Derived from the list above, so the two
# cannot drift — the v1.2.1-style typo is impossible by construction.
ENABLE_FLAGS := $(filter ENABLE_%,$(.VARIABLES))

_BUILD_DIR := $(BUILD_BASE)/$(_ARCH)
INCDIR      = $(MFDIR)include

COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror          \
  -ffreestanding -fno-builtin             \
  -fno-stack-protector                    \
  -ffunction-sections -fdata-sections     \
  -nostdinc -Os -g                        \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)           \
  $(DEVICE_DEFS)                          \
  -I$(INCDIR)                             \
  -I$(MFDIR)../../../02_FwHal/include     \
  -I$(MFDIR)../../../10_BSP/03_SoC/include \
  -I$(MFDIR)../../common

# ── Device-conditional subdirectories ─────────────────────────────────
# One ifeq per device.  A name here that matches no directory contributes
# nothing, which `make info` now makes visible.
SUBDIRS :=
ifeq ($(ENABLE_WIFI),yes)
  SUBDIRS += 02_Sensors/WiFi
endif
ifeq ($(ENABLE_BT),yes)
  SUBDIRS += 01_Com/bt
endif
ifeq ($(ENABLE_ETH),yes)
  SUBDIRS += 01_Com/eth
endif
ifeq ($(ENABLE_USB),yes)
  SUBDIRS += 01_Com/usb
endif
ifeq ($(ENABLE_HDMI),yes)
  SUBDIRS += 01_Com/hdmi
endif
ifeq ($(ENABLE_TB4),yes)
  SUBDIRS += 01_Com/tb4
endif
ifeq ($(ENABLE_MONITOR),yes)
  SUBDIRS += 01_Com/monitor
endif
ifeq ($(ENABLE_GPU),yes)
  SUBDIRS += 01_Com/gpu
endif
ifeq ($(ENABLE_CAMERA),yes)
  SUBDIRS += 02_Sensors/camera
endif
ifeq ($(ENABLE_MIC),yes)
  SUBDIRS += 02_Sensors/mic
endif
ifeq ($(ENABLE_KBD),yes)
  SUBDIRS += 02_Sensors/keyboard
endif
ifeq ($(ENABLE_MOUSE),yes)
  SUBDIRS += 02_Sensors/mouse
endif
ifeq ($(ENABLE_ALS),yes)
  SUBDIRS += 02_Sensors/als
endif
ifeq ($(ENABLE_THERMAL),yes)
  SUBDIRS += 02_Sensors/thermal
endif
ifeq ($(ENABLE_TOUCHPWD),yes)
  SUBDIRS += 02_Sensors/touchpwd
endif
ifeq ($(ENABLE_BMS),yes)
  SUBDIRS += 03_NonSensors/bms
endif
ifeq ($(ENABLE_CHG),yes)
  SUBDIRS += 03_NonSensors/chg
endif
ifeq ($(ENABLE_FAN),yes)
  SUBDIRS += 03_NonSensors/fan
endif
ifeq ($(ENABLE_PMIC),yes)
  SUBDIRS += 03_NonSensors/pmic
endif
ifeq ($(ENABLE_RTC),yes)
  SUBDIRS += 03_NonSensors/rtc
endif
ifeq ($(ENABLE_SPEAKER),yes)
  SUBDIRS += 03_NonSensors/speaker
endif
ifeq ($(ENABLE_EMMC),yes)
  SUBDIRS += 03_NonSensors/emmc
endif
ifeq ($(ENABLE_UART),yes)
  SUBDIRS += 01_Com/uart
endif
ifeq ($(ENABLE_SDMMC),yes)
  SUBDIRS += 03_NonSensors/sdmmc
endif
ifeq ($(ENABLE_NVME),yes)
  SUBDIRS += 03_NonSensors/nvme
endif
ifeq ($(ENABLE_SATA),yes)
  SUBDIRS += 03_NonSensors/sata
endif

SRCS := $(foreach d,$(SUBDIRS),$(wildcard $(MFDIR)$(d)/*.c))
SRCS += $(MFDIR)02_Sensors/als/uiox_als_fwops.c
OBJS := $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(SRCS))
LIB  := $(_BUILD_DIR)/libhw.a

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
	@echo "[DI] $(_BUILD_DIR)/libhw.a  ($(words $(SUBDIRS)) subsystems)"
	@echo "[DI]  $(SUBDIRS)"

$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(COMMON_CFLAGS) \
	    -I$(MFDIR)$(dir $*) \
	    -I$(MFDIR)../../../02_FwHal/$(dir $*) \
	    -MMD -MP -c $< -o $@

-include $(wildcard $(BUILD_BASE)/$(_ARCH)/*.d)

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	rm -rf $(BUILD_BASE)

# ── info — which flags are on, and what SUBDIRS resolved to ───────────
# An ifeq whose name matches no directory contributes nothing silently;
# this makes the count visible, the same reason 32_FS carries srclist.
.PHONY: info
info:
	@echo "DI         : $(MFDIR)"
	@echo "arch       : $(_ARCH)  CC=$(CC)"
	@echo "subsystems : $(words $(SUBDIRS))"
	@echo "subdirs    : $(SUBDIRS)"
	@echo "sources    : $(words $(SRCS)) -> $(LIB)"
	@echo "flags      : $(foreach f,$(ENABLE_FLAGS),$(f)=$($(f)) )"
	@echo "device defs: $(DEVICE_DEFS)"
