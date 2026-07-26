# =============================================================================
# 30_KIX/30_DeviceDrivers/Makefile  — v1.3.2
#
# Builds only the device driver subdirectories for enabled devices.
# GPU: buffer-only — no DeviceDriver entry (handled by 31_BufferCache).
# Outputs: build/<arch>/libdev.a
# @version 1.3.2  @date 2026-07-26
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

COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror           \
  -ffreestanding -fno-builtin              \
  -fno-stack-protector                     \
  -ffunction-sections -fdata-sections      \
  -nostdinc -Os -g                         \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)            \
  $(DEVICE_DEFS)                           \
  -I$(MFDIR)../common                      \
  -I$(MFDIR)../20_DriverInterfaces/include \
  -I$(MFDIR)../31_BufferCache/include      \
  -I$(MFDIR)../02_FwHal/include            \
  -I$(MFDIR)../20_DriverInterfaces/01_Com/bt         \
  -I$(MFDIR)../20_DriverInterfaces/01_Com/eth        \
  -I$(MFDIR)../20_DriverInterfaces/01_Com/usb        \
  -I$(MFDIR)../20_DriverInterfaces/01_Com/hdmi       \
  -I$(MFDIR)../20_DriverInterfaces/01_Com/tb4        \
  -I$(MFDIR)../20_DriverInterfaces/01_Com/monitor    \
  -I$(MFDIR)../20_DriverInterfaces/01_Com/gpu        \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/WiFi       \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/camera     \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/mic        \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/keyboard   \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/mouse      \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/als        \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/thermal    \
  -I$(MFDIR)../20_DriverInterfaces/02_Sensors/touchpwd   \
  -I$(MFDIR)../20_DriverInterfaces/03_NonSensors/bms     \
  -I$(MFDIR)../20_DriverInterfaces/03_NonSensors/chg     \
  -I$(MFDIR)../20_DriverInterfaces/03_NonSensors/fan     \
  -I$(MFDIR)../20_DriverInterfaces/03_NonSensors/pmic    \
  -I$(MFDIR)../20_DriverInterfaces/03_NonSensors/rtc     \
  -I$(MFDIR)../20_DriverInterfaces/03_NonSensors/speaker \
  -I$(MFDIR)../20_DriverInterfaces/03_NonSensors/emmc    \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/WiFI        \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/bt          \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/eth         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/usb         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/hdmi        \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/tb4         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/monitor     \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/gpu         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/camera      \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/mic         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/keyboard    \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/mouse       \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/als         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/thermal     \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/touchpwd    \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/bms         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/chg         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/fan         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/pmic        \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/rtc         \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/speaker     \
  -I$(MFDIR)../31_BufferCache/01_DrvBuff/emmc

# ── Device-conditional subdirectories ────────────────────────────────────────
SUBDIRS :=
ifeq ($(ENABLE_WIFI),yes)
  SUBDIRS += 02_Sensors/WiFI
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
LIB  := $(_BUILD_DIR)/libdev.a

# =============================================================================
# Arch entry points — re-invoke with _ARCH=$@ so $(_BUILD_DIR) is defined.
# =============================================================================
.PHONY: arm64 arm32 riscv64 x86_64

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
	    ENABLE_EMMC=$(ENABLE_EMMC)

.PHONY: _build
_build: $(LIB)
	@echo "[DEV] $(_BUILD_DIR)/libdev.a  ($(words $(SUBDIRS)) subsystems)"
	@echo "[DEV]  $(SUBDIRS)"

$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(COMMON_CFLAGS) \
	    -I$(MFDIR)$(dir $*) \
	    -MMD -MP -c $< -o $@

-include $(wildcard $(BUILD_BASE)/$(_ARCH)/*.d)

.PHONY: clean
clean:
	rm -rf $(BUILD_BASE)
