# =============================================================================
# 02_FwHal/Makefile  — v1.3.0
#
# Builds only the device source files for enabled devices.
# Always-on infrastructure (I2C, SPI, UART, GPIO, clock, timer, …) is
# compiled unconditionally.  Device files are conditional on ENABLE_*.
#
# Outputs: build/<arch>/libuiox_fw_<arch>.a
#
# v1.3.0 — storage-class sources are now conditionally compiled:
#   · uiox_fw_sd.c    is added for ENABLE_SDMC=yes   (flag already passed
#     by the umbrella; previously the file existed but was never built).
#   · uiox_fw_nvme.c  is added for ENABLE_NVME=yes   (same).
#   · Both files sit above the umbrella's SDMC/NVME device list; before
#     this change every build linked a kernel with no SD/NVMe firmware
#     symbols at all.
#   NOTE — uiox_fw_sd.c's read/write entry points still return
#   UIOX_FW_OK without transferring data.  Wiring the file in does NOT
#   make those paths real; they must return a failure code until the
#   transfer layer exists, or the driver will report success on a buffer
#   it never filled.  Fix the returns BEFORE relying on this build.
#
# v1.2.1 — two fixes:
#   · The ENABLE_RTC block named uiox_fw_rtc.c, which does not exist.  The
#     RTC source is uiox_fw_ramrtc.c and it is already in the unconditional
#     SRCS block above, so the conditional block was both stale and
#     duplicate.  Removed.
#   · ENABLE_RTC defaulted to `no`, which meant a plain `make arm64` built
#     the archive without the RTC branch ever being reachable.  Defaulted
#     to `yes` to match every other device flag.
#
# @version 1.3.0  @date 2026-10-25
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
ENABLE_SDMC     ?= yes
ENABLE_NVME     ?= yes

_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror          \
  -ffreestanding -fno-builtin             \
  -fno-stack-protector                    \
  -ffunction-sections -fdata-sections     \
  -nostdinc -Os -g                        \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)           \
  $(DEVICE_DEFS)                          \
  -isystem $(shell $(CC) -print-file-name=include) \
  -I$(MFDIR)../10_BSP/03_SoC/include \
  -I$(MFDIR)../01_uBoot/include       \
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
  $(SRCDIR)/uiox_fw_virtio.c  \
  $(SRCDIR)/uiox_fw_none.c  \
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

# ── Storage classes (2026-10-25) ──────────────────────────────────────────────
# The umbrella passes ENABLE_SDMC / ENABLE_NVME in $(ENABLE_FLAGS) and the
# kernel's storage layer (uiox_fw_stor_get) expects these drivers to exist,
# but no block here added the sources — so the archive shipped without them
# and the link resolved the storage symbols from elsewhere (or not at all).
#
# Only files whose stubs are honest belong here.  uiox_fw_sd.c is included
# because the file exists as a compile unit and its probe path is real; its
# transfer paths are NOT, and must be corrected to return a failure code
# before anything depends on them.  uiox_fw_sata.c and uiox_fw_net.c are
# deliberately left out: SATA has no enable flag in the umbrella's device
# list, and net duplicates what ENABLE_ETH/ENABLE_WIFI already cover.
ifeq ($(ENABLE_SDMC),yes)
  SRCS += $(SRCDIR)/uiox_fw_sd.c
endif
ifeq ($(ENABLE_NVME),yes)
  SRCS += $(SRCDIR)/uiox_fw_nvme.c
endif

# ── RTC ───────────────────────────────────────────────────────────────────────
# uiox_fw_ramrtc.c is in the unconditional SRCS block above: the RTC is a
# driver (DS1307 + MC146818), not a device gated on ENABLE_*.  The old
# `ENABLE_RTC` block here named uiox_fw_rtc.c, a file that does not exist —
# which made the archive fail with "No rule to make target uiox_fw_rtc.o"
# whenever ENABLE_RTC resolved to yes.  That block is gone.
#
# Keeping ENABLE_RTC as a recognised flag: the umbrella passes it in
# $(ENABLE_FLAGS), and an unrecognised variable is harmless, but a stale
# SRCS line is not.
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
	    ENABLE_EMMC=$(ENABLE_EMMC)      \
	    ENABLE_SDMC=$(ENABLE_SDMC)      \
	    ENABLE_NVME=$(ENABLE_NVME)

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
