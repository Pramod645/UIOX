# =============================================================
# UIOX top-level Makefile
# Builds the UIOX OS kernel for ARM64, ARM32, and x86_64.
#
# Usage:
#   make                   build all three architectures
#   make ARCH=arm64        build ARM64 only
#   make ARCH=arm32        build ARM32 only
#   make ARCH=x86_64       build x86_64 only
#   make run               run x86_64 binary natively
#   make qemu_arm64        boot ARM64 image in QEMU
#   make qemu_arm32        boot ARM32 image in QEMU
#   make qemu_x86_64       boot x86_64 image in QEMU
#   make check_tools       verify all cross-compilers are present
#   make clean             remove all build artefacts
#   make help              print this message
#
# Cross-compilers required:
#   ARM64  : aarch64-linux-gnu-gcc
#   ARM32  : arm-linux-gnueabihf-gcc
#   x86_64 : gcc  (native on x86_64 Linux/macOS)
# =============================================================
#To install aarch64-elf-gcc ✔, run:
#  brew install aarch64-elf-gcc 
# --- Always keep these targets phony -------------------------
.PHONY: all arm64 arm32 x86_64 run \
        qemu_arm64 qemu_arm32 qemu_x86_64 \
        check_tools clean help

# --- Repository root (absolute, works from any sub-dir) ------
UIOX_ROOT := $(CURDIR)

# --- Build output directories --------------------------------
BUILD_DIR := $(UIOX_ROOT)/build
OBJ_DIR   := $(BUILD_DIR)/obj
BIN_DIR   := $(BUILD_DIR)/bin
MAP_DIR   := $(BUILD_DIR)/map

# --- Helper macros -------------------------------------------
comma := ,
space :=
space +=
MKDIR_P := mkdir -p

# --- Default: build all architectures ------------------------
ifeq ($(ARCH),)
all: arm64 arm32 x86_64
	@echo ""
	@echo "============================================"
	@echo " UIOX build complete — all architectures"
	@echo "============================================"
	@ls -lh $(BIN_DIR)/
else
all: $(ARCH)
endif

# =============================================================
# ARM64 build
# =============================================================
include build_config/tools.mk

ARM64_ARCH_FLAGS := \
    -march=armv8-a \
    -mtune=cortex-a53 \
    -mabi=lp64 \
    -mstrict-align \
    -fno-pic \
    -fno-pie

ARM64_DEFS := \
    -DUIOX_ARCH_ARM64=1 \
    -DUIOX_BITS=64 \
    -DUIOX_ENDIAN_LITTLE=1

ARM64_INCLUDES := \
    -I$(UIOX_ROOT)/10_Arch/arm64/include \
    -I$(UIOX_ROOT)/20_DriverInterfaces/include \
    -I$(UIOX_ROOT)/30_DeviceDrivers/include \
    -I$(UIOX_ROOT)/31_BufferCache/include \
    -I$(UIOX_ROOT)/32_FileSystem \
    -I$(UIOX_ROOT)/33_ProcessControlSubsystem \
    -I$(UIOX_ROOT)/40_SystemCallInterface \
    -I$(UIOX_ROOT)/50_UIX/00_libs \
    -I$(UIOX_ROOT)

ARM64_WARN := \
    -Wall -Wextra -Wshadow \
    -Wstrict-prototypes \
    -Wmissing-prototypes \
    -Wno-unused-parameter

ARM64_KERN := \
    -ffreestanding -fno-builtin \
    -fno-stack-protector -fno-common \
    -nostdlib -std=c99

ARM64_CFLAGS := \
    $(ARM64_ARCH_FLAGS) \
    $(ARM64_DEFS) \
    $(ARM64_INCLUDES) \
    $(ARM64_WARN) \
    $(ARM64_KERN) \
    -O2 -g3

ARM64_LDFLAGS := \
    -T $(UIOX_ROOT)/linker/uiox_arm64.ld \
    -Map=$(MAP_DIR)/uiox_arm64.map \
    --no-undefined \
    --build-id=none \
    --gc-sections \
    -nostdlib \
    -static

ARM64_SRCS := \
    $(UIOX_ROOT)/main.c \
    $(UIOX_ROOT)/10_Arch/arm64/src/arch_init.c \
    $(UIOX_ROOT)/10_Arch/arm64/src/arm64_decode.c \
    $(UIOX_ROOT)/10_Arch/arm64/src/arm64_execute.c \
    $(UIOX_ROOT)/10_Arch/arm64/src/arm64_memory.c \
    $(UIOX_ROOT)/10_Arch/arm64/src/arm64_exceptions.c \
    $(wildcard $(UIOX_ROOT)/20_DriverInterfaces/src/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/src/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/00_character/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/01_block/*.c) \
    $(wildcard $(UIOX_ROOT)/31_BufferCache/src/*.c) \
    $(wildcard $(UIOX_ROOT)/32_FileSystem/01_fsa/*.c) \
    $(wildcard $(UIOX_ROOT)/32_FileSystem/10_scfs/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/00_inter-process-communication/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/01_schedular/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/02_memory-managment/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/40_procStruct/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/50_scps/*.c) \
    $(wildcard $(UIOX_ROOT)/40_SystemCallInterface/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/00_libs/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/01_shell/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/20_uios/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/21_apps/*.c)

ARM64_OBJS := $(patsubst $(UIOX_ROOT)/%.c, \
                  $(OBJ_DIR)/arm64/%.o, $(ARM64_SRCS))

ARM64_ELF := $(BIN_DIR)/uiox_arm64.elf
ARM64_BIN := $(BIN_DIR)/uiox_arm64.bin
ARM64_HEX := $(BIN_DIR)/uiox_arm64.hex
ARM64_LST := $(BIN_DIR)/uiox_arm64.lst

$(OBJ_DIR)/arm64/%.o: $(UIOX_ROOT)/%.c
	@$(MKDIR_P) $(dir $@)
	@echo "  [arm64] CC  $<"
	$(ARM64_CC) $(ARM64_CFLAGS) -c -o $@ $<

$(ARM64_ELF): $(ARM64_OBJS)
	@$(MKDIR_P) $(BIN_DIR) $(MAP_DIR)
	@echo "  [arm64] LD  $@"
	$(ARM64_LD) $(ARM64_LDFLAGS) -o $@ $^
	$(ARM64_SIZE) $@

$(ARM64_BIN): $(ARM64_ELF)
	$(ARM64_OBJCOPY) -O binary $< $@
	@echo "  [arm64] BIN $@"

$(ARM64_HEX): $(ARM64_ELF)
	$(ARM64_OBJCOPY) -O ihex $< $@
	@echo "  [arm64] HEX $@"

$(ARM64_LST): $(ARM64_ELF)
	$(ARM64_OBJDUMP) -d -S $< > $@
	@echo "  [arm64] LST $@"

arm64: $(ARM64_ELF) $(ARM64_BIN) $(ARM64_HEX) $(ARM64_LST)
	@echo "[arm64] Build DONE → $(ARM64_ELF)"

# =============================================================
# ARM32 build
# =============================================================
ARM32_ARCH_FLAGS := \
    -march=armv7-a \
    -mtune=cortex-a9 \
    -mfpu=vfpv3-d16 \
    -mfloat-abi=hard \
    -marm \
    -fno-pic \
    -fno-pie

ARM32_DEFS := \
    -DUIOX_ARCH_ARM32=1 \
    -DUIOX_BITS=32 \
    -DUIOX_ENDIAN_LITTLE=1

ARM32_INCLUDES := \
    -I$(UIOX_ROOT)/10_Arch/arm32/include \
    -I$(UIOX_ROOT)/20_DriverInterfaces/include \
    -I$(UIOX_ROOT)/30_DeviceDrivers/include \
    -I$(UIOX_ROOT)/31_BufferCache/include \
    -I$(UIOX_ROOT)/32_FileSystem \
    -I$(UIOX_ROOT)/33_ProcessControlSubsystem \
    -I$(UIOX_ROOT)/40_SystemCallInterface \
    -I$(UIOX_ROOT)/50_UIX/00_libs \
    -I$(UIOX_ROOT)

ARM32_CFLAGS := \
    $(ARM32_ARCH_FLAGS) \
    $(ARM32_DEFS) \
    $(ARM32_INCLUDES) \
    -Wall -Wextra -Wstrict-prototypes \
    -Wmissing-prototypes -Wno-unused-parameter \
    -ffreestanding -fno-builtin \
    -fno-stack-protector -fno-common \
    -nostdlib -std=c99 \
    -O2 -g3

ARM32_LDFLAGS := \
    -T $(UIOX_ROOT)/linker/uiox_arm32.ld \
    -Map=$(MAP_DIR)/uiox_arm32.map \
    --no-undefined \
    --build-id=none \
    --gc-sections \
    -nostdlib \
    -static

ARM32_SRCS := \
    $(UIOX_ROOT)/main.c \
    $(UIOX_ROOT)/10_Arch/arm32/src/arch_init.c \
    $(UIOX_ROOT)/10_Arch/arm32/src/arm_decode.c \
    $(UIOX_ROOT)/10_Arch/arm32/src/arm_execute.c \
    $(UIOX_ROOT)/10_Arch/arm32/src/arm_memory.c \
    $(UIOX_ROOT)/10_Arch/arm32/src/arm_exceptions.c \
    $(wildcard $(UIOX_ROOT)/20_DriverInterfaces/src/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/src/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/00_character/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/01_block/*.c) \
    $(wildcard $(UIOX_ROOT)/31_BufferCache/src/*.c) \
    $(wildcard $(UIOX_ROOT)/32_FileSystem/01_fsa/*.c) \
    $(wildcard $(UIOX_ROOT)/32_FileSystem/10_scfs/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/00_inter-process-communication/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/01_schedular/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/02_memory-managment/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/40_procStruct/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/50_scps/*.c) \
    $(wildcard $(UIOX_ROOT)/40_SystemCallInterface/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/00_libs/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/01_shell/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/20_uios/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/21_apps/*.c)

ARM32_OBJS := $(patsubst $(UIOX_ROOT)/%.c, \
                  $(OBJ_DIR)/arm32/%.o, $(ARM32_SRCS))

ARM32_ELF := $(BIN_DIR)/uiox_arm32.elf
ARM32_BIN := $(BIN_DIR)/uiox_arm32.bin
ARM32_HEX := $(BIN_DIR)/uiox_arm32.hex
ARM32_LST := $(BIN_DIR)/uiox_arm32.lst

$(OBJ_DIR)/arm32/%.o: $(UIOX_ROOT)/%.c
	@$(MKDIR_P) $(dir $@)
	@echo "  [arm32] CC  $<"
	$(ARM32_CC) $(ARM32_CFLAGS) -c -o $@ $<

$(ARM32_ELF): $(ARM32_OBJS)
	@$(MKDIR_P) $(BIN_DIR) $(MAP_DIR)
	@echo "  [arm32] LD  $@"
	$(ARM32_LD) $(ARM32_LDFLAGS) -o $@ $^
	$(ARM32_SIZE) $@

$(ARM32_BIN): $(ARM32_ELF)
	$(ARM32_OBJCOPY) -O binary $< $@

$(ARM32_HEX): $(ARM32_ELF)
	$(ARM32_OBJCOPY) -O ihex $< $@

$(ARM32_LST): $(ARM32_ELF)
	$(ARM32_OBJDUMP) -d -S $< > $@

arm32: $(ARM32_ELF) $(ARM32_BIN) $(ARM32_HEX) $(ARM32_LST)
	@echo "[arm32] Build DONE → $(ARM32_ELF)"

# =============================================================
# x86_64 build
# =============================================================
X86_ARCH_FLAGS := \
    -m64 \
    -march=x86-64 \
    -mtune=generic \
    -mno-red-zone \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mcmodel=kernel \
    -fno-pic \
    -fno-pie

X86_DEFS := \
    -DUIOX_ARCH_X86_64=1 \
    -DUIOX_BITS=64 \
    -DUIOX_ENDIAN_LITTLE=1

X86_INCLUDES := \
    -I$(UIOX_ROOT)/10_Arch/x86_64/include \
    -I$(UIOX_ROOT)/20_DriverInterfaces/include \
    -I$(UIOX_ROOT)/30_DeviceDrivers/include \
    -I$(UIOX_ROOT)/31_BufferCache/include \
    -I$(UIOX_ROOT)/32_FileSystem \
    -I$(UIOX_ROOT)/33_ProcessControlSubsystem \
    -I$(UIOX_ROOT)/40_SystemCallInterface \
    -I$(UIOX_ROOT)/50_UIX/00_libs \
    -I$(UIOX_ROOT)

X86_CFLAGS := \
    $(X86_ARCH_FLAGS) \
    $(X86_DEFS) \
    $(X86_INCLUDES) \
    -Wall -Wextra -Wstrict-prototypes \
    -Wmissing-prototypes -Wno-unused-parameter \
    -ffreestanding -fno-builtin \
    -fno-stack-protector -fno-common \
    -nostdlib -std=c99 \
    -O2 -g3

X86_LDFLAGS := \
    -T $(UIOX_ROOT)/linker/uiox_x86_64.ld \
    -Map=$(MAP_DIR)/uiox_x86_64.map \
    --no-undefined \
    --build-id=none \
    --gc-sections \
    -nostdlib \
    -static \
    -z max-page-size=0x1000

X86_SRCS := \
    $(UIOX_ROOT)/main.c \
    $(UIOX_ROOT)/10_Arch/x86_64/src/arch_init.c \
    $(UIOX_ROOT)/10_Arch/x86_64/src/x86_decode.c \
    $(UIOX_ROOT)/10_Arch/x86_64/src/x86_execute.c \
    $(UIOX_ROOT)/10_Arch/x86_64/src/x86_memory.c \
    $(UIOX_ROOT)/10_Arch/x86_64/src/x86_exceptions.c \
    $(UIOX_ROOT)/10_Arch/x86_64/src/x86_msr.c \
    $(wildcard $(UIOX_ROOT)/20_DriverInterfaces/src/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/src/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/00_character/*.c) \
    $(wildcard $(UIOX_ROOT)/30_DeviceDrivers/01_block/*.c) \
    $(wildcard $(UIOX_ROOT)/31_BufferCache/src/*.c) \
    $(wildcard $(UIOX_ROOT)/32_FileSystem/01_fsa/*.c) \
    $(wildcard $(UIOX_ROOT)/32_FileSystem/10_scfs/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/00_inter-process-communication/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/01_schedular/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/02_memory-managment/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/40_procStruct/*.c) \
    $(wildcard $(UIOX_ROOT)/33_ProcessControlSubsystem/50_scps/*.c) \
    $(wildcard $(UIOX_ROOT)/40_SystemCallInterface/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/00_libs/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/01_shell/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/20_uios/*.c) \
    $(wildcard $(UIOX_ROOT)/50_UIX/21_apps/*.c)

X86_OBJS := $(patsubst $(UIOX_ROOT)/%.c, \
                $(OBJ_DIR)/x86_64/%.o, $(X86_SRCS))

X86_ELF := $(BIN_DIR)/uiox_x86_64.elf
X86_BIN := $(BIN_DIR)/uiox_x86_64.bin
X86_HEX := $(BIN_DIR)/uiox_x86_64.hex
X86_LST := $(BIN_DIR)/uiox_x86_64.lst

$(OBJ_DIR)/x86_64/%.o: $(UIOX_ROOT)/%.c
	@$(MKDIR_P) $(dir $@)
	@echo "  [x86_64] CC  $<"
	$(X86_CC) $(X86_CFLAGS) -c -o $@ $<

$(X86_ELF): $(X86_OBJS)
	@$(MKDIR_P) $(BIN_DIR) $(MAP_DIR)
	@echo "  [x86_64] LD  $@"
	$(X86_LD) $(X86_LDFLAGS) -o $@ $^
	$(X86_SIZE) $@

$(X86_BIN): $(X86_ELF)
	$(X86_OBJCOPY) -O binary $< $@

$(X86_HEX): $(X86_ELF)
	$(X86_OBJCOPY) -O ihex $< $@

$(X86_LST): $(X86_ELF)
	$(X86_OBJDUMP) -d -S $< > $@

x86_64: $(X86_ELF) $(X86_BIN) $(X86_HEX) $(X86_LST)
	@echo "[x86_64] Build DONE → $(X86_ELF)"

# =============================================================
# QEMU launch targets
# =============================================================
QEMU_ARM64  := qemu-system-aarch64
QEMU_ARM32  := qemu-system-arm
QEMU_X86    := qemu-system-x86_64

qemu_arm64: $(ARM64_ELF)
	$(QEMU_ARM64) \
	    -machine virt \
	    -cpu cortex-a53 \
	    -m 64M \
	    -nographic \
	    -serial mon:stdio \
	    -kernel $(ARM64_ELF)

qemu_arm32: $(ARM32_ELF)
	$(QEMU_ARM32) \
	    -machine versatilepb \
	    -cpu arm926 \
	    -m 16M \
	    -nographic \
	    -serial mon:stdio \
	    -kernel $(ARM32_ELF)

qemu_x86_64: $(X86_ELF)
	$(QEMU_X86) \
	    -machine q35 \
	    -cpu qemu64 \
	    -m 64M \
	    -nographic \
	    -serial mon:stdio \
	    -kernel $(X86_ELF)

# =============================================================
# Run native x86_64 binary directly
# =============================================================
run: $(X86_ELF)
	@echo "[run] Executing native x86_64 UIOX kernel..."
	$(X86_ELF)

# =============================================================
# Toolchain check
# =============================================================
check_tools:
	@echo "=== UIOX Toolchain Check ==="
	@echo -n "ARM64  : $(ARM64_CC)  -> "; \
	    which $(ARM64_CC) > /dev/null 2>&1 && echo "OK" || echo "MISSING"
	@echo -n "ARM32  : $(ARM32_CC)  -> "; \
	    which $(ARM32_CC) > /dev/null 2>&1 && echo "OK" || echo "MISSING"
	@echo -n "x86_64 : $(X86_CC)   -> "; \
	    which $(X86_CC)   > /dev/null 2>&1 && echo "OK" || echo "MISSING"
	@echo "==========================="

# =============================================================
# Clean
# =============================================================
clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean done."

# =============================================================
# Help
# =============================================================
help:
	@echo ""
	@echo "  UIOX Build System"
	@echo ""
	@echo "  make                  build ARM64 + ARM32 + x86_64"
	@echo "  make ARCH=arm64       build ARM64 only"
	@echo "  make ARCH=arm32       build ARM32 only"
	@echo "  make ARCH=x86_64      build x86_64 only"
	@echo "  make run              run native x86_64 ELF"
	@echo "  make qemu_arm64       boot ARM64 in QEMU"
	@echo "  make qemu_arm32       boot ARM32 in QEMU"
	@echo "  make qemu_x86_64      boot x86_64 in QEMU"
	@echo "  make check_tools      verify cross-compilers"
	@echo "  make clean            remove build/ directory"
	@echo "  make help             this message"
	@echo ""
	@echo "  Cross-compilers:"
	@echo "    ARM64  : aarch64-linux-gnu-gcc"
	@echo "    ARM32  : arm-linux-gnueabihf-gcc"
	@echo "    x86_64 : gcc (native)"
	@echo ""
	@echo "  Install on Ubuntu/Debian:"
	@echo "    sudo apt install gcc \\"
	@echo "      gcc-aarch64-linux-gnu \\"
	@echo "      gcc-arm-linux-gnueabihf \\"
	@echo "      binutils-aarch64-linux-gnu \\"
	@echo "      binutils-arm-linux-gnueabihf \\"
	@echo "      q
	@echo "    sudo apt install gcc \\"
	@echo "      gcc-aarch64-linux-gnu \\"
	@echo "      gcc-arm-linux-gnueabihf \\"
	@echo "      binutils-aarch64-linux-gnu \\"
	@echo "      binutils-arm-linux-gnueabihf \\"
	@echo "      qemu-system-arm \\"
	@echo "      qemu-system-x86"
	@echo ""
	@echo "  Install on macOS (Homebrew):"
	@echo "    brew install aarch64-elf-gcc arm-none-eabi-gcc gcc qemu"
	@echo ""
