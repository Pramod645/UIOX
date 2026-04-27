# =============================================================
# build_config/x86_64.mk
#
# x86_64 (AMD64 / Intel 64) build configuration.
#
# Compiler: gcc (native or cross)
# Target:   x86-64, SSE2 baseline, ELF64
# =============================================================

ARCH          := x86_64
ARCH_DIR      := x86_64
TRIPLE        :=   # native — no cross prefix

CC            := gcc
LD            := ld
OBJCOPY       := objcopy
OBJDUMP       := objdump
AR            := ar
SIZE          := size

# ── Architecture-specific CFLAGS ─────────────────────────────
ARCH_CFLAGS   := -march=x86-64           \
                 -mtune=generic           \
                 -msse2                   \
                 -DARCH_X86_64            \
                 -DUIOX_ARCH_X86_64=1     \
                 -Iarch/x86_64/include

include build_config/common.mk

CFLAGS        := $(COMMON_CFLAGS) $(ARCH_CFLAGS) $(COMMON_INCS)
LDFLAGS       := $(COMMON_LDFLAGS)

# ── Output paths ──────────────────────────────────────────────
OBJ_DIR       := obj/x86_64
LIB_DIR       := lib/x86_64
TARGET_ELF    := uiox_x86_64
TARGET_BIN    := uiox_x86_64.bin
STATIC_LIB    := $(LIB_DIR)/libuiox_x86_64.a
SHARED_LIB    := $(LIB_DIR)/libuiox_x86_64.so

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

.PHONY: x86_64 x86_64_static x86_64_shared x86_64_bin x86_64_clean

x86_64: dirs $(TARGET_ELF) x86_64_static x86_64_shared
	@$(SIZE) $(TARGET_ELF) 2>/dev/null || true
	@echo "=== x86_64 build complete ==="

$(TARGET_ELF): $(ALL_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@echo "Linked: $@"

x86_64_bin: $(TARGET_ELF)
	$(OBJCOPY) -O binary $(TARGET_ELF) $(TARGET_BIN)
	@echo "Binary image: $(TARGET_BIN)"

x86_64_static: $(STATIC_LIB)
$(STATIC_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Static lib: $@"

x86_64_shared: $(SHARED_LIB)
$(SHARED_LIB): $(ALL_OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC $(LDFLAGS) -o $@ $^
	@echo "Shared lib: $@"

x86_64_clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET_ELF) $(TARGET_BIN)

x86_64_run: $(TARGET_ELF)
	./$(TARGET_ELF)

x86_64_qemu: x86_64_bin
	qemu-system-x86_64 \
	  -M q35 -m 128M \
	  -nographic \
	  -kernel $(TARGET_BIN) \
	  -serial mon:stdio
