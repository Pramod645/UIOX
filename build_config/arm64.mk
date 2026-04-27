# =============================================================
# build_config/arm64.mk
#
# ARM64 (AArch64) build configuration.
#
# Cross-compiler: aarch64-linux-gnu-gcc
# Target:         ARMv8-A, little-endian, ELF64
# =============================================================

ARCH          := arm64
ARCH_DIR      := arm64
TRIPLE        := aarch64-linux-gnu

CC            := $(TRIPLE)-gcc
LD            := $(TRIPLE)-ld
OBJCOPY       := $(TRIPLE)-objcopy
OBJDUMP       := $(TRIPLE)-objdump
AR            := $(TRIPLE)-ar
SIZE          := $(TRIPLE)-size

# ── Architecture-specific CFLAGS ─────────────────────────────
ARCH_CFLAGS   := -march=armv8-a           \
                 -mtune=cortex-a53         \
                 -mabi=lp64               \
                 -DARCH_ARM64             \
                 -DUIOX_ARCH_ARM64=1      \
                 -Iarch/arm64/include

# ── Include common settings ───────────────────────────────────
include build_config/common.mk

CFLAGS        := $(COMMON_CFLAGS) $(ARCH_CFLAGS) $(COMMON_INCS)
LDFLAGS       := $(COMMON_LDFLAGS)

# ── Output paths ──────────────────────────────────────────────
OBJ_DIR       := obj/arm64
LIB_DIR       := lib/arm64
TARGET_ELF    := uiox_arm64.elf
TARGET_BIN    := uiox_arm64.bin
STATIC_LIB    := $(LIB_DIR)/libuiox_arm64.a
SHARED_LIB    := $(LIB_DIR)/libuiox_arm64.so

# ── Collect object files ──────────────────────────────────────
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

# ── Targets ───────────────────────────────────────────────────
.PHONY: arm64 arm64_static arm64_shared arm64_bin arm64_clean

arm64: dirs $(TARGET_ELF) arm64_static arm64_shared
	@$(SIZE) $(TARGET_ELF) 2>/dev/null || true
	@echo "=== ARM64 build complete ==="

$(TARGET_ELF): $(ALL_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@echo "Linked: $@"

arm64_bin: $(TARGET_ELF)
	$(OBJCOPY) -O binary $(TARGET_ELF) $(TARGET_BIN)
	@echo "Binary image: $(TARGET_BIN)"

arm64_static: $(STATIC_LIB)
$(STATIC_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Static lib: $@"

arm64_shared: $(SHARED_LIB)
$(SHARED_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC $(LDFLAGS) -o $@ $^
	@echo "Shared lib: $@"

arm64_clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET_ELF) $(TARGET_BIN)

# ── QEMU run ──────────────────────────────────────────────────
arm64_run: arm64_bin
	qemu-system-aarch64 \
	  -M virt -cpu cortex-a53 -m 128M \
	  -nographic \
	  -kernel $(TARGET_BIN) \
	  -serial mon:stdio
