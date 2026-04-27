# =============================================================
# build_config/arm32.mk
#
# ARM32 (ARMv7-A) build configuration.
#
# Cross-compiler: arm-linux-gnueabihf-gcc
# Target:         ARMv7-A, Thumb-2, hard-float, ELF32
# =============================================================

ARCH          := arm32
ARCH_DIR      := arm32
TRIPLE        := arm-linux-gnueabihf

CC            := $(TRIPLE)-gcc
LD            := $(TRIPLE)-ld
OBJCOPY       := $(TRIPLE)-objcopy
OBJDUMP       := $(TRIPLE)-objdump
AR            := $(TRIPLE)-ar
SIZE          := $(TRIPLE)-size

# ── Architecture-specific CFLAGS ─────────────────────────────
ARCH_CFLAGS   := -march=armv7-a           \
                 -mtune=cortex-a9         \
                 -mfpu=vfpv3-d16          \
                 -mfloat-abi=hard         \
                 -mthumb                  \
                 -DARCH_ARM32             \
                 -DUIOX_ARCH_ARM32=1      \
                 -Iarch/arm32/include

include build_config/common.mk

CFLAGS        := $(COMMON_CFLAGS) $(ARCH_CFLAGS) $(COMMON_INCS)
LDFLAGS       := $(COMMON_LDFLAGS)

# ── Output paths ──────────────────────────────────────────────
OBJ_DIR       := obj/arm32
LIB_DIR       := lib/arm32
TARGET_ELF    := uiox_arm32.elf
TARGET_BIN    := uiox_arm32.bin
STATIC_LIB    := $(LIB_DIR)/libuiox_arm32.a
SHARED_LIB    := $(LIB_DIR)/libuiox_arm32.so

FS_OBJS       := $(patsubst $(UIOX_ROOT)/uiox_fs/src/%.c, \
                             $(OBJ_DIR)/%.o, $(FS_SRCS))
DEV_OBJS      := $(patsubst $(UIOX_ROOT)/uiox_dev/src/%.c, \
                             $(OBJ_DIR)/%.o, $(DEV_SRCS))
HW_OBJS       := $(patsubst $(UIOX_ROOT)/uiox_hw/src/%.c, \
                             $(OBJ_DIR)/%.o, $(HW_SRCS))
ARCH_OBJS     := $(OBJ_DIR)/arch_init.o
MAIN_OBJ      := $(OBJ_DIR)/main.o
ALL_OBJS      := $(FS_OBJS) $(DEV_OBJS) $(HW_OBJS) \
                 $(ARCH_OBJS) $(MAIN_OBJ)

.PHONY: arm32 arm32_static arm32_shared arm32_bin arm32_clean

arm32: dirs $(TARGET_ELF) arm32_static arm32_shared
	@$(SIZE) $(TARGET_ELF) 2>/dev/null || true
	@echo "=== ARM32 build complete ==="

$(TARGET_ELF): $(ALL_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@echo "Linked: $@"

arm32_bin: $(TARGET_ELF)
	$(OBJCOPY) -O binary $(TARGET_ELF) $(TARGET_BIN)
	@echo "Binary image: $(TARGET_BIN)"

arm32_static: $(STATIC_LIB)
$(STATIC_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Static lib: $@"

arm32_shared: $(SHARED_LIB)
$(SHARED_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC $(LDFLAGS) -o $@ $^
	@echo "Shared lib: $@"

arm32_clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET_ELF) $(TARGET_BIN)

arm32_run: arm32_bin
	qemu-system-arm \
	  -M versatilepb -cpu cortex-a9 -m 64M \
	  -nographic \
	  -kernel $(TARGET_BIN) \
	  -serial mon:stdio
