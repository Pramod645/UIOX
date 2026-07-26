# =============================================================================
# 30_KIX/Makefile  — v2.4.0
#
# UIOX Kernel Umbrella Makefile (Linux + macOS).
#
# Build chain:
#   1. 02_FwHal              → firmware HAL
#   2. 20_DriverInterfaces   → hardware abstraction
#   3. 30_DeviceDrivers      → kernel device drivers
#   4. 31_BufferCache        → buffer cache
#   5. 32_FS                 → file system
#                               (includes 02_journal, 03_netfs)
#   6. 33_PCS                → process control
#                               (includes 03_ksign, 04_fboot,
#                                         05_sec,   06_kpatch)
#   7. 34_CAS                → kernel comms / IPC services
#   8. 50_UIX/01_shell       → first user shell
#   9. uiox_kernel_main.c    → kernel entry point
#
# Subsystems moved from 50_UIX into kernel:
#   50_UIX/10_kpatch  → 33_PCS/06_kpatch   (live kernel patching)
#   50_UIX/11_netfs   → 32_FS/03_netfs     (in-kernel NFS/RPC)
#   50_UIX/12_ksign   → 33_PCS/03_ksign    (kernel signing + integrity)
#   50_UIX/13_fboot   → 33_PCS/04_fboot    (fast-boot / snapshot)
#   50_UIX/13_journal → 32_FS/02_journal   (filesystem journal)
#   50_UIX/14_sec     → 33_PCS/05_sec      (ASLR + MAC security)
#
# Usage:
#   make arm64 LINK=yes
#   make arm64 LINK=yes BSP_MODE=dynamic
#   make arm64 LINK=yes ENABLE_BT=no ENABLE_GPU=no
#   make kernel-dynamic-arm64 ENABLE_HDMI=no
#
# Device flags (default all=yes):
#   ENABLE_WIFI ENABLE_BT ENABLE_ETH ENABLE_USB ENABLE_HDMI
#   ENABLE_TB4 ENABLE_MONITOR ENABLE_GPU ENABLE_CAMERA ENABLE_MIC
#   ENABLE_KBD ENABLE_MOUSE ENABLE_ALS ENABLE_THERMAL ENABLE_TOUCHPWD
#   ENABLE_BMS ENABLE_CHG ENABLE_FAN ENABLE_PMIC ENABLE_RTC
#   ENABLE_SPEAKER ENABLE_EMMC
#
# @version 2.4.0  @date 2026-07-26
# =============================================================================

MFDIR     := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
COMMONDIR := $(MFDIR)common
LINK      ?= no
BSP_MODE  ?= static

# ── BSP_MODE ──────────────────────────────────────────────────────────────────
BSP_LIB_ARG  = $(if $(filter dynamic,$(BSP_MODE)),,$(BSPDIR)/build/$(_ARCH)/libbsp.a)
BSP_MODE_DEF = $(if $(filter dynamic,$(BSP_MODE)),-DUIOX_BSP_DYNAMIC_BOOT,)

# =============================================================================
# Device enable/disable flags
# =============================================================================
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

# -DUIOX_HAVE_<DEVICE> defines — passed to every compiler invocation
DEVICE_DEFS = \
  $(if $(filter yes,$(ENABLE_WIFI)),    -DUIOX_HAVE_WIFI,)     \
  $(if $(filter yes,$(ENABLE_BT)),      -DUIOX_HAVE_BT,)       \
  $(if $(filter yes,$(ENABLE_ETH)),     -DUIOX_HAVE_ETH,)      \
  $(if $(filter yes,$(ENABLE_USB)),     -DUIOX_HAVE_USB,)      \
  $(if $(filter yes,$(ENABLE_HDMI)),    -DUIOX_HAVE_HDMI,)     \
  $(if $(filter yes,$(ENABLE_TB4)),     -DUIOX_HAVE_TB4,)      \
  $(if $(filter yes,$(ENABLE_MONITOR)), -DUIOX_HAVE_MONITOR,)  \
  $(if $(filter yes,$(ENABLE_GPU)),     -DUIOX_HAVE_GPU,)      \
  $(if $(filter yes,$(ENABLE_CAMERA)),  -DUIOX_HAVE_CAMERA,)   \
  $(if $(filter yes,$(ENABLE_MIC)),     -DUIOX_HAVE_MIC,)      \
  $(if $(filter yes,$(ENABLE_KBD)),     -DUIOX_HAVE_KBD,)      \
  $(if $(filter yes,$(ENABLE_MOUSE)),   -DUIOX_HAVE_MOUSE,)    \
  $(if $(filter yes,$(ENABLE_ALS)),     -DUIOX_HAVE_ALS,)      \
  $(if $(filter yes,$(ENABLE_THERMAL)), -DUIOX_HAVE_THERMAL,)  \
  $(if $(filter yes,$(ENABLE_TOUCHPWD)),-DUIOX_HAVE_TOUCHPWD,) \
  $(if $(filter yes,$(ENABLE_BMS)),     -DUIOX_HAVE_BMS,)      \
  $(if $(filter yes,$(ENABLE_CHG)),     -DUIOX_HAVE_CHG,)      \
  $(if $(filter yes,$(ENABLE_FAN)),     -DUIOX_HAVE_FAN,)      \
  $(if $(filter yes,$(ENABLE_PMIC)),    -DUIOX_HAVE_PMIC,)     \
  $(if $(filter yes,$(ENABLE_RTC)),     -DUIOX_HAVE_RTC,)      \
  $(if $(filter yes,$(ENABLE_SPEAKER)), -DUIOX_HAVE_SPEAKER,)  \
  $(if $(filter yes,$(ENABLE_EMMC)),    -DUIOX_HAVE_EMMC,)

# All ENABLE_* vars forwarded as a single block to sub-makes
ENABLE_FLAGS = \
  ENABLE_WIFI=$(ENABLE_WIFI)         ENABLE_BT=$(ENABLE_BT)             \
  ENABLE_ETH=$(ENABLE_ETH)           ENABLE_USB=$(ENABLE_USB)           \
  ENABLE_HDMI=$(ENABLE_HDMI)         ENABLE_TB4=$(ENABLE_TB4)           \
  ENABLE_MONITOR=$(ENABLE_MONITOR)   ENABLE_GPU=$(ENABLE_GPU)           \
  ENABLE_CAMERA=$(ENABLE_CAMERA)     ENABLE_MIC=$(ENABLE_MIC)           \
  ENABLE_KBD=$(ENABLE_KBD)           ENABLE_MOUSE=$(ENABLE_MOUSE)       \
  ENABLE_ALS=$(ENABLE_ALS)           ENABLE_THERMAL=$(ENABLE_THERMAL)   \
  ENABLE_TOUCHPWD=$(ENABLE_TOUCHPWD) ENABLE_BMS=$(ENABLE_BMS)           \
  ENABLE_CHG=$(ENABLE_CHG)           ENABLE_FAN=$(ENABLE_FAN)           \
  ENABLE_PMIC=$(ENABLE_PMIC)         ENABLE_RTC=$(ENABLE_RTC)           \
  ENABLE_SPEAKER=$(ENABLE_SPEAKER)   ENABLE_EMMC=$(ENABLE_EMMC)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC := aarch64-elf-gcc
  ARM64_AR := aarch64-elf-ar
  ARM64_LD := aarch64-elf-ld
  ARM32_CC := arm-none-eabi-gcc
  ARM32_AR := arm-none-eabi-ar
  ARM32_LD := arm-none-eabi-ld
  RV64_CC  := riscv64-elf-gcc
  RV64_AR  := riscv64-elf-ar
  RV64_LD  := riscv64-elf-ld
  X86_CC   := x86_64-elf-gcc
  X86_AR   := x86_64-elf-ar
  X86_LD   := x86_64-elf-ld
else
  ARM64_CC := aarch64-linux-gnu-gcc
  ARM64_AR := aarch64-linux-gnu-ar
  ARM64_LD := aarch64-linux-gnu-ld
  ARM32_CC := arm-linux-gnueabihf-gcc
  ARM32_AR := arm-linux-gnueabihf-ar
  ARM32_LD := arm-linux-gnueabihf-ld
  RV64_CC  := riscv64-linux-gnu-gcc
  RV64_AR  := riscv64-linux-gnu-ar
  RV64_LD  := riscv64-linux-gnu-ld
  X86_CC   := x86_64-linux-gnu-gcc
  X86_AR   := x86_64-linux-gnu-ar
  X86_LD   := x86_64-linux-gnu-ld
endif

# ── Path roots ────────────────────────────────────────────────────────────────
BSPDIR ?= $(abspath $(MFDIR)../10_BSP)
FWHAL  := $(abspath $(MFDIR)../02_FwHal)
SOCINC  = $(BSPDIR)/03_SoC/include
UIXDIR  := $(abspath $(MFDIR)../50_UIX)

# ── kernel_main compiler flags ────────────────────────────────────────────────
# Note: 12_ksign and 13_fboot headers now come from 33_PCS/include
#       (previously pointed at 50_UIX/12_ksign and 50_UIX/13_fboot)
KERNEL_CFLAGS_BASE := \
  -I$(COMMONDIR)                              \
  -std=c11 -Wall -Wextra -Werror              \
  -ffreestanding -fno-builtin -fno-stack-protector \
  -ffunction-sections -fdata-sections -nostdinc -Os -g \
  -I$(FWHAL)/include                          \
  -I$(MFDIR)32_FS/include                     \
  -I$(MFDIR)32_FS/02_journal/include          \
  -I$(MFDIR)32_FS/03_netfs/include            \
  -I$(MFDIR)33_PCS/include                    \
  -I$(MFDIR)33_PCS/03_ksign/include           \
  -I$(MFDIR)33_PCS/04_fboot/include           \
  -I$(MFDIR)33_PCS/05_sec/include             \
  -I$(MFDIR)33_PCS/06_kpatch/include          \
  -I$(SOCINC)                                 \
  -I$(MFDIR)../01_uBoot/include               \
  -I$(BSPDIR)/include

# =============================================================================
# Per-architecture named targets
# =============================================================================

.PHONY: arm64
arm64: _do_arm64
	@echo "[KIX] arm64 done → build/arm64/"

.PHONY: _do_arm64
_do_arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all              \
	    _ARCH=arm64 CC=$(ARM64_CC) AR=$(ARM64_AR) LD=$(ARM64_LD) \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian" \
	    ARCH_CPUDEF="-D__aarch64__" ARCH_LD_EMUL=aarch64elf \
	    BSP_MODE=$(BSP_MODE) $(ENABLE_FLAGS)

.PHONY: arm32
arm32: _do_arm32
	@echo "[KIX] arm32 done → build/arm32/"

.PHONY: _do_arm32
_do_arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all                  \
	    _ARCH=arm32 CC=$(ARM32_CC) AR=$(ARM32_AR) LD=$(ARM32_LD) \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_CPUDEF="-D__arm__" ARCH_LD_EMUL=armelf \
	    BSP_MODE=$(BSP_MODE) $(ENABLE_FLAGS)

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[KIX] riscv64 done → build/riscv64/"

.PHONY: _do_riscv64
_do_riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all                   \
	    _ARCH=riscv64 CC=$(RV64_CC) AR=$(RV64_AR) LD=$(RV64_LD) \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64" ARCH_LD_EMUL=elf64lriscv \
	    BSP_MODE=$(BSP_MODE) $(ENABLE_FLAGS)

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[KIX] x86_64 done → build/x86_64/"

.PHONY: _do_x86_64
_do_x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_all                    \
	    _ARCH=x86_64 CC=$(X86_CC) AR=$(X86_AR) LD=$(X86_LD)   \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -mcmodel=kernel" \
	    ARCH_CPUDEF="-D__x86_64__" ARCH_LD_EMUL=elf_x86_64 \
	    BSP_MODE=$(BSP_MODE) $(ENABLE_FLAGS)

# =============================================================================
# Dynamic BSP convenience targets
# =============================================================================

.PHONY: bsp-dynamic-arm64 bsp-dynamic-arm32 bsp-dynamic-riscv64 bsp-dynamic-x86_64
bsp-dynamic-arm64:
	$(MAKE) -C $(BSPDIR) arm64 BUILD=dynamic
bsp-dynamic-arm32:
	$(MAKE) -C $(BSPDIR) arm32 BUILD=dynamic
bsp-dynamic-riscv64:
	$(MAKE) -C $(BSPDIR) riscv64 BUILD=dynamic
bsp-dynamic-x86_64:
	$(MAKE) -C $(BSPDIR) x86_64 BUILD=dynamic

.PHONY: kernel-dynamic-arm64 kernel-dynamic-arm32 kernel-dynamic-riscv64 kernel-dynamic-x86_64
kernel-dynamic-arm64:  bsp-dynamic-arm64
	$(MAKE) -f $(MFDIR)Makefile _do_arm64   BSP_MODE=dynamic LINK=yes $(ENABLE_FLAGS)
kernel-dynamic-arm32:  bsp-dynamic-arm32
	$(MAKE) -f $(MFDIR)Makefile _do_arm32   BSP_MODE=dynamic LINK=yes $(ENABLE_FLAGS)
kernel-dynamic-riscv64: bsp-dynamic-riscv64
	$(MAKE) -f $(MFDIR)Makefile _do_riscv64 BSP_MODE=dynamic LINK=yes $(ENABLE_FLAGS)
kernel-dynamic-x86_64: bsp-dynamic-x86_64
	$(MAKE) -f $(MFDIR)Makefile _do_x86_64  BSP_MODE=dynamic LINK=yes $(ENABLE_FLAGS)

# =============================================================================
# _build_all
# =============================================================================
_BUILD_DIR = $(MFDIR)build/$(_ARCH)
_ARCH_INC  = $(BSPDIR)/10_Arch/$(_ARCH)/include

.PHONY: _build_all
_build_all: _build_subsystems _build_kernel_main
ifeq ($(LINK),yes)
_build_all: _link_kernel
endif

.PHONY: _build_subsystems
_build_subsystems:
	@echo "[KIX] building subsystems for $(_ARCH) (BSP_MODE=$(BSP_MODE))..."
	@echo "[KIX] WIFI=$(ENABLE_WIFI) BT=$(ENABLE_BT) ETH=$(ENABLE_ETH) USB=$(ENABLE_USB) HDMI=$(ENABLE_HDMI) TB4=$(ENABLE_TB4) MON=$(ENABLE_MONITOR) GPU=$(ENABLE_GPU)"
	@echo "[KIX] CAM=$(ENABLE_CAMERA) MIC=$(ENABLE_MIC) KBD=$(ENABLE_KBD) MOUSE=$(ENABLE_MOUSE) ALS=$(ENABLE_ALS) THERMAL=$(ENABLE_THERMAL) TPWD=$(ENABLE_TOUCHPWD)"
	@echo "[KIX] BMS=$(ENABLE_BMS) CHG=$(ENABLE_CHG) FAN=$(ENABLE_FAN) PMIC=$(ENABLE_PMIC) RTC=$(ENABLE_RTC) SPK=$(ENABLE_SPEAKER) EMMC=$(ENABLE_EMMC)"
	# 1. 02_FwHal
	+$(MAKE) -C $(FWHAL) CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH) 2>/dev/null || echo "[KIX] 02_FwHal: no Makefile or skipped"
	# 2. 20_DriverInterfaces
	+$(MAKE) -C $(MFDIR)20_DriverInterfaces CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    DEVICE_DEFS="$(DEVICE_DEFS)" $(ENABLE_FLAGS) $(_ARCH)
	# 3. 30_DeviceDrivers
	+$(MAKE) -C $(MFDIR)30_DeviceDrivers CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    DEVICE_DEFS="$(DEVICE_DEFS)" $(ENABLE_FLAGS) $(_ARCH)
	# 4. 31_BufferCache
	+$(MAKE) -C $(MFDIR)31_BufferCache CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    DEVICE_DEFS="$(DEVICE_DEFS)" $(ENABLE_FLAGS) $(_ARCH)
	# 5. 32_FS  (includes 02_journal, 03_netfs — built by 32_FS/Makefile)
	+$(MAKE) -C $(MFDIR)32_FS CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" $(_ARCH)
	# 6. 33_PCS  (includes 03_ksign, 04_fboot, 05_sec, 06_kpatch — built by 33_PCS/Makefile)
	+$(MAKE) -C $(MFDIR)33_PCS CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" $(_ARCH)
	# 7. 34_CAS
	+$(MAKE) -C $(MFDIR)34_CAS CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    DEVICE_DEFS="$(DEVICE_DEFS)" $(ENABLE_FLAGS) \
	    $(_ARCH) 2>/dev/null || echo "[KIX] 34_CAS: skipped"
	# 8. 50_UIX/01_shell  (only remaining 50_UIX build step)
	+$(MAKE) -C $(UIXDIR)/01_shell CC="$(CC)" AR="$(AR)" \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CPUDEF="$(ARCH_CPUDEF)" \
	    $(_ARCH) 2>/dev/null || echo "[KIX] 50_UIX/01_shell: skipped"

.PHONY: _build_kernel_main
_build_kernel_main:
	@mkdir -p $(_BUILD_DIR)
	$(CC) $(KERNEL_CFLAGS_BASE) $(ARCH_CFLAGS) $(ARCH_CPUDEF) \
	    $(BSP_MODE_DEF)                                        \
	    $(DEVICE_DEFS)                                         \
	    -I$(_ARCH_INC)                                         \
	    -c $(MFDIR)uiox_kernel_main.c                          \
	    -o $(_BUILD_DIR)/uiox_kernel_main.o
	@echo "  [CC]  uiox_kernel_main.c  (BSP_MODE=$(BSP_MODE))"

.PHONY: _link_kernel
_link_kernel:
	@echo "[KIX] Linking $(_BUILD_DIR)/uiox_kernel.elf  (BSP_MODE=$(BSP_MODE))..."
	$(LD) -m $(ARCH_LD_EMUL)                                       \
	    -T $(MFDIR)linker/kernel_$(_ARCH).ld                       \
	    -Map=$(_BUILD_DIR)/uiox_kernel.map                         \
	    --gc-sections                                              \
	    $(_BUILD_DIR)/uiox_kernel_main.o                           \
	    $(wildcard $(FWHAL)/build/$(_ARCH)/*.a)                    \
	    $(wildcard $(MFDIR)20_DriverInterfaces/build/$(_ARCH)/*.a) \
	    $(wildcard $(MFDIR)30_DeviceDrivers/build/$(_ARCH)/*.a)    \
	    $(wildcard $(MFDIR)31_BufferCache/build/$(_ARCH)/*.a)      \
	    $(wildcard $(MFDIR)32_FS/build/$(_ARCH)/*.a)               \
	    $(wildcard $(MFDIR)33_PCS/build/$(_ARCH)/*.a)              \
	    $(wildcard $(MFDIR)34_CAS/build/$(_ARCH)/*.a)              \
	    $(wildcard $(UIXDIR)/01_shell/build/$(_ARCH)/*.a)          \
	    $(BSP_LIB_ARG)                                             \
	    -o $(_BUILD_DIR)/uiox_kernel.elf
	@echo "[KIX] Kernel ELF → $(_BUILD_DIR)/uiox_kernel.elf"
ifeq ($(BSP_MODE),dynamic)
	@echo "[KIX] Dynamic BSP → $(BSPDIR)/build/$(_ARCH)/uiox_bsp.bin"
endif

# =============================================================================
# Convenience targets
# =============================================================================

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[KIX] all builds complete"

.PHONY: all-dynamic
all-dynamic: kernel-dynamic-arm64 kernel-dynamic-arm32 \
             kernel-dynamic-riscv64 kernel-dynamic-x86_64
	@echo "" && echo "[KIX] all dynamic builds complete"

.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; rm -rf $(MFDIR)build/arm64
clean-arm32:   ; rm -rf $(MFDIR)build/arm32
clean-riscv64: ; rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; rm -rf $(MFDIR)build/x86_64

.PHONY: clean-subsys
clean-subsys:
	@for d in $(FWHAL) $(MFDIR)20_DriverInterfaces $(MFDIR)30_DeviceDrivers \
	           $(MFDIR)31_BufferCache $(MFDIR)32_FS $(MFDIR)33_PCS \
	           $(MFDIR)34_CAS $(UIXDIR)/01_shell; do \
	    [ -f $$d/Makefile ] && $(MAKE) -C $$d clean || true; \
	done

.PHONY: clean
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean-subsys
	@echo "[CLN] all kernel build outputs removed"

.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64
info-arm64:
	@echo "HOST=$(UNAME_S)  ARCH=arm64    CC=$(ARM64_CC)  BSP_MODE=$(BSP_MODE)  LINK=$(LINK)"
	@echo "  WIFI=$(ENABLE_WIFI) BT=$(ENABLE_BT) ETH=$(ENABLE_ETH) USB=$(ENABLE_USB) HDMI=$(ENABLE_HDMI) GPU=$(ENABLE_GPU) EMMC=$(ENABLE_EMMC)"
info-arm32:
	@echo "HOST=$(UNAME_S)  ARCH=arm32    CC=$(ARM32_CC)  BSP_MODE=$(BSP_MODE)  LINK=$(LINK)"
	@echo "  WIFI=$(ENABLE_WIFI) BT=$(ENABLE_BT) ETH=$(ENABLE_ETH) USB=$(ENABLE_USB) HDMI=$(ENABLE_HDMI) GPU=$(ENABLE_GPU) EMMC=$(ENABLE_EMMC)"
info-riscv64:
	@echo "HOST=$(UNAME_S)  ARCH=riscv64  CC=$(RV64_CC)   BSP_MODE=$(BSP_MODE)  LINK=$(LINK)"
	@echo "  WIFI=$(ENABLE_WIFI) BT=$(ENABLE_BT) ETH=$(ENABLE_ETH) USB=$(ENABLE_USB) HDMI=$(ENABLE_HDMI) GPU=$(ENABLE_GPU) EMMC=$(ENABLE_EMMC)"
info-x86_64:
	@echo "HOST=$(UNAME_S)  ARCH=x86_64   CC=$(X86_CC)    BSP_MODE=$(BSP_MODE)  LINK=$(LINK)"
	@echo "  WIFI=$(ENABLE_WIFI) BT=$(ENABLE_BT) ETH=$(ENABLE_ETH) USB=$(ENABLE_USB) HDMI=$(ENABLE_HDMI) GPU=$(ENABLE_GPU) EMMC=$(ENABLE_EMMC)"
