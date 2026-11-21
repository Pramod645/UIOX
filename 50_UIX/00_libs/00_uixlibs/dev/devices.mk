# =============================================================================
# 50_UIX/00_libs/00_uixlibs/dev/devices.mk
#
# Device layer sources, listed explicitly.  Included by the master Makefile
# AFTER $(MFDIR) is set — this file appends to LIB_SRCS_C and defines the
# per-subdirectory include paths, and defines no targets of its own.
#
# WHY NOT A RECURSIVE WILDCARD:
#   $(wildcard $(MFDIR)dev/*/*.c) would pick up any .c that lands in a
#   subdirectory, including one not meant for the archive.  A file left
#   out of an explicit list fails at link, which is where you want to
#   find it; a file silently included fails somewhere else entirely.
#
# LAYOUT
#   dev/*.c          the six shapes + the path helper — always built
#   dev/<class>/*.c  one directory per device class, one line each below
# =============================================================================

# ── the six shapes and the path helper ──────────────────────────────────────
DEV_SRCS := \
  $(MFDIR)dev/uiox_dev_path.c    \
  $(MFDIR)dev/uiox_dev_stream.c  \
  $(MFDIR)dev/uiox_dev_block.c   \
  $(MFDIR)dev/uiox_dev_event.c   \
  $(MFDIR)dev/uiox_dev_reg.c     \
  $(MFDIR)dev/uiox_dev_cmd.c     \
  $(MFDIR)dev/uiox_dev_bus.c

# ── device classes ─────────────────────────────────────────────────────────
# Each class directory holds its own commands (the UIOX_*_IOC_* numbers) and
# calls the shape it belongs to.  Add one line per class as it lands.
DEV_SRCS += \
  $(MFDIR)dev/als/uiox_dev_als.c  \
  $(MFDIR)dev/therm/uiox_dev_therm.c  \
  $(MFDIR)dev/bms/uiox_dev_bms.c  \
  $(MFDIR)dev/chg/uiox_dev_chg.c \
  $(MFDIR)dev/pmic/uiox_dev_pmic.c  \
  $(MFDIR)dev/rtc/uiox_dev_rtc.c  \
  $(MFDIR)dev/fan/uiox_dev_fan.c  \
  $(MFDIR)dev/gpu/uiox_dev_gpu.c  \
  $(MFDIR)dev/kbd/uiox_dev_kbd.c  \
  $(MFDIR)dev/mouse/uiox_dev_mouse.c  \
  $(MFDIR)dev/tpwd/uiox_dev_tpwd.c  \
  $(MFDIR)dev/emmc/uiox_dev_emmc.c  \
  $(MFDIR)dev/nvme/uiox_dev_nvme.c  \
  $(MFDIR)dev/sata/uiox_dev_sata.c  \
  $(MFDIR)dev/eth/uiox_connectivity.c  \
  $(MFDIR)dev/WiFi/uiox_wifi_device.c \
  $(MFDIR)dev/bt/uiox_bt_device.c \
  $(MFDIR)dev/mic/uiox_mic_device.c \
  $(MFDIR)dev/spk/uiox_spk_device.c \
  $(MFDIR)dev/cam/uiox_cam_device.c \
  $(MFDIR)dev/hdmi/uiox_hdmi_device.c \
  $(MFDIR)dev/monitor/uiox_mon_device.c


# ── include paths ──────────────────────────────────────────────────────────
# The shapes include each other by relative path ("../uiox_dev_reg.h"), so
# the class directories need their own -I as well as dev/ itself.
DEV_INCS := \
  $(MFDIR)dev     \
  $(MFDIR)dev/als \
  $(MFDIR)dev/therm \
  $(MFDIR)dev/bms \
  $(MFDIR)dev/chg \
  $(MFDIR)dev/pmic \
  $(MFDIR)dev/rtc \
  $(MFDIR)dev/fan \
  $(MFDIR)dev/gpu \
  $(MFDIR)dev/kbd \
  $(MFDIR)dev/mouse \
  $(MFDIR)dev/tpwd  \
  $(MFDIR)dev/emmc  \
  $(MFDIR)dev/nvme  \
  $(MFDIR)dev/sata  \
  $(MFDIR)dev/eth  \
  $(MFDIR)dev/WiFi \
  $(MFDIR)dev/bt \
  $(MFDIR)dev/mic \
  $(MFDIR)dev/spk \
  $(MFDIR)dev/cam \
  $(MFDIR)dev/hdmi \
  $(MFDIR)dev/monitor

# ── exported to the master Makefile ────────────────────────────────────────
LIB_SRCS_C += $(DEV_SRCS)
LIB_DIRS   += $(DEV_INCS)
