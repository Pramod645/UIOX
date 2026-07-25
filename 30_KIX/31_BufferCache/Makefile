# =============================================================================
# 30_KIX/31_BufferCache/Makefile  — v1.3.0
#
# UIOX Buffer Cache — master Makefile (Linux + macOS).
# Builds libbcache.a for the selected architecture.
#
# Directory layout (verified from live repo — main branch):
#   31_BufferCache/
#   ├── 00_FileBuff/    include/  src/bcache.c
#   └── 01_DrvBuff/     WiFi  als  bms  bt  camera  chg  emmc  eth  fan
#                       gpu  hdmi  keyboard  mic  monitor  mouse  pmic
#                       rtc  speaker  tb4  thermal  touchpwd  usb
#
# v1.1.0: added all 20_DriverInterfaces peripheral subdirs to COMMON_INCLUDES
#         so buffer headers can include their matching HAL headers directly.
# v1.2.0: added bt, camera, usb subdirs to 01_DrvBuff coverage
# v1.3.0: added all remaining 01_DrvBuff subdirs (chg, emmc, eth, fan, gpu,
#         hdmi, keyboard, mic, mouse, pmic, rtc, speaker, tb4, thermal)
# @version 1.3.0  @date 2026-07-25
# =============================================================================

MFDIR    := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(abspath $(MFDIR)../common)

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

# ── HAL headers from 20_DriverInterfaces (every peripheral subdir) ────────────
HALTOP  = $(abspath $(MFDIR)../20_DriverInterfaces/include)
HALBT   = $(abspath $(MFDIR)../20_DriverInterfaces/01_Com/bt)
HALETH  = $(abspath $(MFDIR)../20_DriverInterfaces/01_Com/eth)
HALHDMI = $(abspath $(MFDIR)../20_DriverInterfaces/01_Com/hdmi)
HALTB4  = $(abspath $(MFDIR)../20_DriverInterfaces/01_Com/tb4)
HALWIFI = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/WiFi)
HALCAM  = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/camera)
HALMIC  = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/mic)
HALTHM  = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/thermal)
HALTPWD = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/touchpwd)
HALBMS  = $(abspath $(MFDIR)../20_DriverInterfaces/03_NonSensors/bms)
HALCHG  = $(abspath $(MFDIR)../20_DriverInterfaces/03_NonSensors/chg)
HALEMMC = $(abspath $(MFDIR)../20_DriverInterfaces/03_NonSensors/emmc)
HALFAN  = $(abspath $(MFDIR)../20_DriverInterfaces/03_NonSensors/fan)
HALPMIC = $(abspath $(MFDIR)../20_DriverInterfaces/03_NonSensors/pmic)
HALRTC  = $(abspath $(MFDIR)../20_DriverInterfaces/03_NonSensors/rtc)
HALSPK  = $(abspath $(MFDIR)../20_DriverInterfaces/03_NonSensors/speaker)
# HALALS / HALKBD / HALMOU were missing — uiox_als_hw.h etc. live here
HALALS  = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/als)
HALKBD  = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/keyboard)
HALMOU  = $(abspath $(MFDIR)../20_DriverInterfaces/02_Sensors/mouse)
HALGPU  = $(abspath $(MFDIR)../20_DriverInterfaces/01_Com/gpu)
HALMON  = $(abspath $(MFDIR)../20_DriverInterfaces/01_Com/monitor)
# ── HAL headers from 20_DriverInterfaces (every peripheral subdir) ────────────

# 01_Com
HALUSB  = $(abspath $(MFDIR)../20_DriverInterfaces/01_Com/usb)



COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector \
  -ffunction-sections -fdata-sections -nostdinc -Os -g

COMMON_INCLUDES := \
  -I$(COMMONDIR)                           \
  -I$(HALTOP)                              \
  -I$(HALBT)                               \
  -I$(HALETH)                              \
  -I$(HALHDMI)                             \
  -I$(HALTB4)                              \
  -I$(HALWIFI)                             \
  -I$(HALCAM)                              \
  -I$(HALMIC)                              \
  -I$(HALTHM)                              \
  -I$(HALTPWD)                             \
  -I$(HALBMS)                              \
  -I$(HALCHG)                              \
  -I$(HALEMMC)                             \
  -I$(HALFAN)                              \
  -I$(HALPMIC)                             \
  -I$(HALRTC)                              \
  -I$(HALSPK)                              \
  -I$(HALALS)                              \
  -I$(HALKBD)                              \
  -I$(HALMOU)                              \
  -I$(HALGPU)                              \
  -I$(HALMON)                              \
  -I$(HALUSB)                              \
  -I$(MFDIR)00_FileBuff/include            \
  -I$(MFDIR)01_DrvBuff/WiFi               \
  -I$(MFDIR)01_DrvBuff/als                \
  -I$(MFDIR)01_DrvBuff/bms                \
  -I$(MFDIR)01_DrvBuff/bt                 \
  -I$(MFDIR)01_DrvBuff/camera             \
  -I$(MFDIR)01_DrvBuff/chg                \
  -I$(MFDIR)01_DrvBuff/emmc               \
  -I$(MFDIR)01_DrvBuff/eth                \
  -I$(MFDIR)01_DrvBuff/fan                \
  -I$(MFDIR)01_DrvBuff/gpu                \
  -I$(MFDIR)01_DrvBuff/hdmi               \
  -I$(MFDIR)01_DrvBuff/keyboard           \
  -I$(MFDIR)01_DrvBuff/mic                \
  -I$(MFDIR)01_DrvBuff/monitor            \
  -I$(MFDIR)01_DrvBuff/mouse              \
  -I$(MFDIR)01_DrvBuff/pmic               \
  -I$(MFDIR)01_DrvBuff/rtc                \
  -I$(MFDIR)01_DrvBuff/speaker            \
  -I$(MFDIR)01_DrvBuff/tb4                \
  -I$(MFDIR)01_DrvBuff/thermal            \
  -I$(MFDIR)01_DrvBuff/touchpwd           \
  -I$(MFDIR)01_DrvBuff/usb

BC_SRCS_C :=
BC_SRCS_C += $(wildcard $(MFDIR)00_FileBuff/src/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/WiFi/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/als/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/bms/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/bt/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/camera/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/chg/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/emmc/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/eth/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/fan/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/gpu/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/hdmi/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/keyboard/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/mic/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/monitor/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/mouse/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/pmic/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/rtc/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/speaker/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/tb4/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/thermal/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/touchpwd/*.c)
BC_SRCS_C += $(wildcard $(MFDIR)01_DrvBuff/usb/*.c)

vpath %.c \
  $(MFDIR)00_FileBuff/src      \
  $(MFDIR)01_DrvBuff/WiFi      \
  $(MFDIR)01_DrvBuff/als       \
  $(MFDIR)01_DrvBuff/bms       \
  $(MFDIR)01_DrvBuff/bt        \
  $(MFDIR)01_DrvBuff/camera    \
  $(MFDIR)01_DrvBuff/chg       \
  $(MFDIR)01_DrvBuff/emmc      \
  $(MFDIR)01_DrvBuff/eth       \
  $(MFDIR)01_DrvBuff/fan       \
  $(MFDIR)01_DrvBuff/gpu       \
  $(MFDIR)01_DrvBuff/hdmi      \
  $(MFDIR)01_DrvBuff/keyboard  \
  $(MFDIR)01_DrvBuff/mic       \
  $(MFDIR)01_DrvBuff/monitor   \
  $(MFDIR)01_DrvBuff/mouse     \
  $(MFDIR)01_DrvBuff/pmic      \
  $(MFDIR)01_DrvBuff/rtc       \
  $(MFDIR)01_DrvBuff/speaker   \
  $(MFDIR)01_DrvBuff/tb4       \
  $(MFDIR)01_DrvBuff/thermal   \
  $(MFDIR)01_DrvBuff/touchpwd  \
  $(MFDIR)01_DrvBuff/usb

ARFLAGS := rcs

.PHONY: arm64
arm64: _do_arm64
	@echo "[BCACHE] arm64 done"
.PHONY: _do_arm64
_do_arm64:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=arm64 CC=$(ARM64_CC) AR=$(ARM64_AR) \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" ARCH_CPUDEF="-D__aarch64__"

.PHONY: arm32
arm32: _do_arm32
	@echo "[BCACHE] arm32 done"
.PHONY: _do_arm32
_do_arm32:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=arm32 CC=$(ARM32_CC) AR=$(ARM32_AR) \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" ARCH_CPUDEF="-D__arm__"

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[BCACHE] riscv64 done"
.PHONY: _do_riscv64
_do_riscv64:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=riscv64 CC=$(RV64_CC) AR=$(RV64_AR) \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[BCACHE] x86_64 done"
.PHONY: _do_x86_64
_do_x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=x86_64 CC=$(X86_CC) AR=$(X86_AR) \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__"

_BUILD_DIR = $(MFDIR)build/$(_ARCH)
_CFLAGS_ALL = $(COMMON_CFLAGS) $(COMMON_INCLUDES) $(ARCH_CFLAGS) $(ARCH_CPUDEF)
_OBJS    = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(BC_SRCS_C))
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
-include $(_OBJS:.o=.d)

.PHONY: _build
_build: $(_BUILD_DIR)/libbcache.a
	@echo "[BCACHE] _build done: $(_ARCH)"

$(_BUILD_DIR)/libbcache.a: $(_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(_CFLAGS_ALL) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "[BCACHE] all builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; rm -rf $(MFDIR)build/arm64
clean-arm32:   ; rm -rf $(MFDIR)build/arm32
clean-riscv64: ; rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; rm -rf $(MFDIR)build/x86_64
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] BufferCache build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:   ; @echo "HOST=$(UNAME_S) ARCH=arm64   CC=$(ARM64_CC)"
info-arm32:   ; @echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC)"
info-riscv64: ; @echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC)"
info-x86_64:  ; @echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC)"
