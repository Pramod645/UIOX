# =============================================================================
# 30_KIX/20_DriverInterfaces/Makefile  — v1.2.1
#
# Hardware Abstraction Layer — builds only subdirectories for enabled devices.
# Outputs: build/<arch>/libhw.a
# @version 1.2.1  @date 2026-07-26
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  = $(MFDIR)build

CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
DEVICE_DEFS ?=
_ARCH       ?= arm64

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
  -I$(MFDIR)../02_FwHal/include           \
  -I$(MFDIR)../common

# ── Device-conditional subdirectories ────────────────────────────────────────
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

SRCS := $(foreach d,$(SUBDIRS),$(wildcard $(MFDIR)$(d)/*.c))
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
	    ENABLE_WIFI=$(ENABLE_WIFI)      \
	    ENABLE_BT=$(ENABLE_BT)          \
	    ENABLE_ETH=$(ENABLE_ETH)        \
	    ENABLE_USB=$(ENABLE_USB)        \
	    ENABLE_HDMI=$(ENABLE_HDMI)      \
	    ENABLE_TB4=$(ENABLE_TB4)        \
	    ENABLE_MONITOR=$(ENABLE_MONITOR)\
	    ENABLE_GPU=$(ENABLE_GPU)        \
	    ENABLE_CAMERA=$(ENABLE_CAMERA)  \
	    ENABLE_MIC=$(ENABLE_MIC)        \
	    ENABLE_KBD=$(ENABLE_KBD)        \
	    ENABLE_MOUSE=$(ENABLE_MOUSE)    \
	    ENABLE_ALS=$(ENABLE_ALS)        \
	    ENABLE_THERMAL=$(ENABLE_THERMAL)\
	    ENABLE_TOUCHPWD=$(ENABLE_TOUCHPWD)\
	    ENABLE_BMS=$(ENABLE_BMS)        \
	    ENABLE_CHG=$(ENABLE_CHG)        \
	    ENABLE_FAN=$(ENABLE_FAN)        \
	    ENABLE_PMIC=$(ENABLE_PMIC)      \
	    ENABLE_RTC=$(ENABLE_RTC)        \
	    ENABLE_SPEAKER=$(ENABLE_SPEAKER)\
	    ENABLE_EMMC=$(ENABLE_EMMC)

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
	    -I$(MFDIR)../02_FwHal/$(dir $*) \
	    -MMD -MP -c $< -o $@

-include $(wildcard $(BUILD_BASE)/$(_ARCH)/*.d)

.PHONY: clean
clean:
	rm -rf $(BUILD_BASE)
