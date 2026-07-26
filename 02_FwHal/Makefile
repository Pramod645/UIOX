# =============================================================================
# 02_FwHal/Makefile  — v1.2.0
#
# Builds only the device source files for enabled devices.
# Always-on infrastructure (I2C, SPI, UART, GPIO, clock, timer, …) is
# compiled unconditionally.  Device files are conditional on ENABLE_*.
#
# Outputs: build/<arch>/libuiox_fw_<arch>.a
# @version 1.2.0  @date 2026-07-26
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  = $(MFDIR)build
SRCDIR      = $(MFDIR)src
INCDIR      = $(MFDIR)include

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

_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror          \
  -ffreestanding -fno-builtin             \
  -fno-stack-protector                    \
  -ffunction-sections -fdata-sections     \
  -nostdinc -Os -g                        \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)           \
  $(DEVICE_DEFS)                          \
  -I$(INCDIR)

# ── Always-on infrastructure ──────────────────────────────────────────────────
SRCS := \
  $(SRCDIR)/uiox_fw_main.c    \
  $(SRCDIR)/uiox_fw_hw.c      \
  $(SRCDIR)/uiox_fw_clock.c   \
  $(SRCDIR)/uiox_fw_gpio.c    \
  $(SRCDIR)/uiox_fw_i2c.c     \
  $(SRCDIR)/uiox_fw_spi.c     \
  $(SRCDIR)/uiox_fw_uart.c    \
  $(SRCDIR)/uiox_fw_timer.c   \
  $(SRCDIR)/uiox_fw_devsw.c   \
  $(SRCDIR)/uiox_fw_wdt.c     \
  $(SRCDIR)/uiox_fw_pcie.c    \
  $(SRCDIR)/uiox_fw_storage.c \
  $(SRCDIR)/uiox_fw_ramrtc.c  \
  $(SRCDIR)/uiox_fw_sensor.c

# ── Device-conditional sources ────────────────────────────────────────────────
ifeq ($(ENABLE_WIFI),yes)
  SRCS += $(SRCDIR)/uiox_fw_wifi.c
endif
ifeq ($(ENABLE_BT),yes)
  SRCS += $(SRCDIR)/uiox_fw_bt.c
endif
ifeq ($(ENABLE_ETH),yes)
  SRCS += $(SRCDIR)/uiox_fw_eth.c
endif
ifeq ($(ENABLE_USB),yes)
  SRCS += $(SRCDIR)/uiox_fw_usb.c
endif
ifeq ($(ENABLE_HDMI),yes)
  SRCS += $(SRCDIR)/uiox_fw_hdmi.c
endif
ifeq ($(ENABLE_TB4),yes)
  SRCS += $(SRCDIR)/uiox_fw_tb4.c
endif
ifeq ($(ENABLE_MONITOR),yes)
  SRCS += $(SRCDIR)/uiox_fw_monitor.c
endif
ifeq ($(ENABLE_GPU),yes)
  SRCS += $(SRCDIR)/uiox_fw_gpu.c
endif
ifeq ($(ENABLE_CAMERA),yes)
  SRCS += $(SRCDIR)/uiox_fw_camera.c
endif
ifeq ($(ENABLE_MIC),yes)
  SRCS += $(SRCDIR)/uiox_fw_mic.c
endif
ifeq ($(ENABLE_KBD),yes)
  SRCS += $(SRCDIR)/uiox_fw_keyboard.c
endif
ifeq ($(ENABLE_MOUSE),yes)
  SRCS += $(SRCDIR)/uiox_fw_mouse.c
endif
ifeq ($(ENABLE_ALS),yes)
  SRCS += $(SRCDIR)/uiox_fw_als.c
endif
ifeq ($(ENABLE_THERMAL),yes)
  SRCS += $(SRCDIR)/uiox_fw_thermal.c
endif
ifeq ($(ENABLE_TOUCHPWD),yes)
  SRCS += $(SRCDIR)/uiox_fw_touchpwd.c
endif
ifeq ($(ENABLE_BMS),yes)
  SRCS += $(SRCDIR)/uiox_fw_bms.c
endif
ifeq ($(ENABLE_CHG),yes)
  SRCS += $(SRCDIR)/uiox_fw_charger.c
endif
ifeq ($(ENABLE_FAN),yes)
  SRCS += $(SRCDIR)/uiox_fw_fan.c
endif
ifeq ($(ENABLE_PMIC),yes)
  SRCS += $(SRCDIR)/uiox_fw_pmic.c
endif
ifeq ($(ENABLE_RTC),yes)
  SRCS += $(SRCDIR)/uiox_fw_rtc.c
endif
ifeq ($(ENABLE_SPEAKER),yes)
  SRCS += $(SRCDIR)/uiox_fw_speaker.c
endif
ifeq ($(ENABLE_EMMC),yes)
  SRCS += $(SRCDIR)/uiox_fw_emmc.c
endif

SRCS := $(sort $(SRCS))
OBJS := $(patsubst $(SRCDIR)/%.c,$(_BUILD_DIR)/%.o,$(SRCS))
LIB  := $(_BUILD_DIR)/libuiox_fw_$(_ARCH).a

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
	@echo "[FwHal] $(_BUILD_DIR)/libuiox_fw_$(_ARCH).a  ($(words $(SRCS)) files)"

$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(_BUILD_DIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(COMMON_CFLAGS) -MMD -MP -c $< -o $@

-include $(wildcard $(_BUILD_DIR)/*.d)

.PHONY: clean
clean:
	rm -rf $(BUILD_BASE)
