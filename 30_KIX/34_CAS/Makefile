# =============================================================================
# 30_KIX/34_CAS/Makefile  — v1.0.0
#
# UIOX Connected Application Services — master Makefile (Linux + macOS).
# Builds libcas.a for the selected architecture.
#
# Usage (matches 30_DeviceDrivers / 33_PCS / 32_FS / 10_BSP pattern):
#   make arm64 / arm32 / riscv64 / x86_64
#   make all
#   make clean[-arm64|-arm32|-riscv64|-x86_64]
#   make info[-arm64|-arm32|-riscv64|-x86_64]
#
# Directory layout (verified from live repo — main branch):
#   34_CAS/
#   ├── 01_Com/          bt  eth  gpu  hdmi  monitor
#   ├── 02_Sensors/      mic  mouse  thermal  touchpwd
#   └── 03_NonSecnsors/  bms  chg  emmc  fan  pmic  rtc  speaker
#
# @version 1.0.0  @date 2026-07-25
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

# Layer below this one
DEVINC = $(abspath $(MFDIR)../30_DeviceDrivers)
HALINC = $(abspath $(MFDIR)../20_DriverInterfaces/include)

COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector \
  -ffunction-sections -fdata-sections -nostdinc -Os -g

COMMON_INCLUDES := \
  -I$(COMMONDIR)                           \
  -I$(HALINC)                             \
  -I$(MFDIR)01_Com/bt                     \
  -I$(MFDIR)01_Com/eth                    \
  -I$(MFDIR)01_Com/gpu                    \
  -I$(MFDIR)01_Com/hdmi                   \
  -I$(MFDIR)01_Com/monitor                \
  -I$(MFDIR)02_Sensors/mic                \
  -I$(MFDIR)02_Sensors/mouse              \
  -I$(MFDIR)02_Sensors/thermal            \
  -I$(MFDIR)02_Sensors/touchpwd           \
  -I$(MFDIR)03_NonSensors/bms            \
  -I$(MFDIR)03_NonSensors/chg            \
  -I$(MFDIR)03_NonSensors/emmc           \
  -I$(MFDIR)03_NonSensors/fan            \
  -I$(MFDIR)03_NonSensors/pmic           \
  -I$(MFDIR)03_NonSensors/rtc            \
  -I$(MFDIR)03_NonSensors/speaker

CAS_SRCS_C :=
# 01_Com
CAS_SRCS_C += $(wildcard $(MFDIR)01_Com/bt/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)01_Com/eth/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)01_Com/gpu/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)01_Com/hdmi/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)01_Com/monitor/*.c)
# 02_Sensors
CAS_SRCS_C += $(wildcard $(MFDIR)02_Sensors/mic/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)02_Sensors/mouse/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)02_Sensors/thermal/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)02_Sensors/touchpwd/*.c)
# 03_NonSecnsors
CAS_SRCS_C += $(wildcard $(MFDIR)03_NonSensors/bms/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)03_NonSensors/chg/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)03_NonSensors/emmc/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)03_NonSensors/fan/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)03_NonSensors/pmic/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)03_NonSensors/rtc/*.c)
CAS_SRCS_C += $(wildcard $(MFDIR)03_NonSensors/speaker/*.c)

vpath %.c \
  $(MFDIR)01_Com/bt           $(MFDIR)01_Com/eth          \
  $(MFDIR)01_Com/gpu          $(MFDIR)01_Com/hdmi         \
  $(MFDIR)01_Com/monitor      $(MFDIR)02_Sensors/mic      \
  $(MFDIR)02_Sensors/mouse    $(MFDIR)02_Sensors/thermal  \
  $(MFDIR)02_Sensors/touchpwd \
  $(MFDIR)03_NonSensors/bms  $(MFDIR)03_NonSensors/chg  \
  $(MFDIR)03_NonSensors/emmc $(MFDIR)03_NonSensors/fan  \
  $(MFDIR)03_NonSensors/pmic $(MFDIR)03_NonSensors/rtc  \
  $(MFDIR)03_NonSensors/speaker

ARFLAGS := rcs

.PHONY: arm64
arm64: _do_arm64
	@echo "[CAS] arm64 done"
.PHONY: _do_arm64
_do_arm64:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=arm64 CC=$(ARM64_CC) AR=$(ARM64_AR) \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" ARCH_CPUDEF="-D__aarch64__"

.PHONY: arm32
arm32: _do_arm32
	@echo "[CAS] arm32 done"
.PHONY: _do_arm32
_do_arm32:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=arm32 CC=$(ARM32_CC) AR=$(ARM32_AR) \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" ARCH_CPUDEF="-D__arm__"

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[CAS] riscv64 done"
.PHONY: _do_riscv64
_do_riscv64:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=riscv64 CC=$(RV64_CC) AR=$(RV64_AR) \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[CAS] x86_64 done"
.PHONY: _do_x86_64
_do_x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=x86_64 CC=$(X86_CC) AR=$(X86_AR) \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__"

_BUILD_DIR = $(MFDIR)build/$(_ARCH)
_CFLAGS_ALL = $(COMMON_CFLAGS) $(COMMON_INCLUDES) $(ARCH_CFLAGS) $(ARCH_CPUDEF)
_OBJS    = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(CAS_SRCS_C))
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
-include $(_OBJS:.o=.d)

.PHONY: _build
_build: $(_BUILD_DIR)/libcas.a
	@echo "[CAS] _build done: $(_ARCH)"

$(_BUILD_DIR)/libcas.a: $(_OBJS)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(_CFLAGS_ALL) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "[CAS] all builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; rm -rf $(MFDIR)build/arm64
clean-arm32:   ; rm -rf $(MFDIR)build/arm32
clean-riscv64: ; rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; rm -rf $(MFDIR)build/x86_64
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] CAS build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:   ; @echo "HOST=$(UNAME_S) ARCH=arm64   CC=$(ARM64_CC)"
info-arm32:   ; @echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC)"
info-riscv64: ; @echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC)"
info-x86_64:  ; @echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC)"
