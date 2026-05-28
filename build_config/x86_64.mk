# =============================================================
# build_config/x86_64.mk
# x86-64 (IA-32e / AMD64) compiler and linker configuration.
# =============================================================

include build_config/tools.mk
include build_config/common.mk

# --- x86_64 arch-specific compiler flags ---------------------
X86_ARCH_FLAGS := \
    -m64 \
    -mno-red-zone \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mcmodel=kernel \
    -mno-80387

# --- x86_64 preprocessor defines -----------------------------
X86_DEFS := \
    -DUIOX_ARCH_X86_64=1 \
    -DUIOX_BITS=64 \
    -DUIOX_ENDIAN_LITTLE=1

# --- x86_64 include paths ------------------------------------
X86_INCLUDES := \
    -I$(UIOX_ROOT)/$(DIR_ARCH)/x86_64/include \
    $(SUBSYS_INCLUDES)

# --- Combined x86_64 CFLAGS ----------------------------------
X86_CFLAGS := \
    $(COMMON_CFLAGS) \
    $(X86_ARCH_FLAGS) \
    $(X86_DEFS) \
    $(X86_INCLUDES)

# --- x86_64 linker flags -------------------------------------
X86_LDFLAGS := \
    $(COMMON_LDFLAGS) \
    -T $(UIOX_ROOT)/$(DIR_LINKER)/uiox_x86_64.ld \
    -Map=$(MAP_DIR)/uiox_x86_64.map \
    --print-memory-usage \
    -z max-page-size=0x1000

# --- x86_64 sources ------------------------------------------
X86_ARCH_SRCS := \
    $(UIOX_ROOT)/$(DIR_ARCH)/x86_64/src/arch_init.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/x86_64/src/x86_decode.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/x86_64/src/x86_execute.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/x86_64/src/x86_memory.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/x86_64/src/x86_exceptions.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/x86_64/src/x86_msr.c

X86_SRCS := \
    $(UIOX_ROOT)/main.c \
    $(X86_ARCH_SRCS) \
    $(SUBSYS_SRCS)

X86_OBJS := $(patsubst $(UIOX_ROOT)/%.c, \
               $(OBJ_DIR)/x86_64/%.o, \
               $(X86_SRCS))

X86_ELF  := $(BIN_DIR)/uiox_x86_64.elf
X86_BIN  := $(BIN_DIR)/uiox_x86_64.bin
X86_HEX  := $(BIN_DIR)/uiox_x86_64.hex
X86_LST  := $(BIN_DIR)/uiox_x86_64.lst

# --- x86_64 build rules --------------------------------------
.PHONY: x86_64 x86_64_clean

x86_64: $(X86_ELF) $(X86_BIN) $(X86_HEX) $(X86_LST)
	@echo "[x86_64] Build complete: $(X86_ELF)"
	@$(X86_SIZE) $(X86_ELF)

$(OBJ_DIR)/x86_64/%.o: $(UIOX_ROOT)/%.c
	@$(MKDIR_P) $(dir $@)
	$(X86_CC) $(X86_CFLAGS) -c -o $@ $<

$(X86_ELF): $(X86_OBJS)
	@$(MKDIR_P) $(BIN_DIR) $(MAP_DIR)
	$(X86_CC) $(X86_CFLAGS) \
	    -Wl,$(subst $(space),$(comma),$(X86_LDFLAGS)) \
	    -o $@ $^

$(X86_BIN): $(X86_ELF)
	$(X86_OBJCOPY) -O binary $< $@

$(X86_HEX): $(X86_ELF)
	$(X86_OBJCOPY) -O ihex $< $@

$(X86_LST): $(X86_ELF)
	$(X86_OBJDUMP) -d -S $< > $@

x86_64_clean:
	rm -rf $(OBJ_DIR)/x86_64 $(X86_ELF) \
	       $(X86_BIN) $(X86_HEX) $(X86_LST)
