# =============================================================
# build_config/arm32.mk
# ARM32 (ARMv7-A) compiler and linker configuration.
# =============================================================

include build_config/tools.mk
include build_config/common.mk

# --- ARM32 arch-specific compiler flags ----------------------
ARM32_ARCH_FLAGS := \
    -march=armv7-a \
    -mtune=cortex-a9 \
    -mfpu=vfpv3-d16 \
    -mfloat-abi=hard \
    -marm \
    -mabi=aapcs

# --- ARM32 preprocessor defines ------------------------------
ARM32_DEFS := \
    -DUIOX_ARCH_ARM32=1 \
    -DUIOX_BITS=32 \
    -DUIOX_ENDIAN_LITTLE=1

# --- ARM32 include paths -------------------------------------
ARM32_INCLUDES := \
    -I$(UIOX_ROOT)/$(DIR_ARCH)/arm32/include \
    $(SUBSYS_INCLUDES)

# --- Combined ARM32 CFLAGS -----------------------------------
ARM32_CFLAGS := \
    $(COMMON_CFLAGS) \
    $(ARM32_ARCH_FLAGS) \
    $(ARM32_DEFS) \
    $(ARM32_INCLUDES)

# --- ARM32 linker flags --------------------------------------
ARM32_LDFLAGS := \
    $(COMMON_LDFLAGS) \
    -T $(UIOX_ROOT)/$(DIR_LINKER)/uiox_arm32.ld \
    -Map=$(MAP_DIR)/uiox_arm32.map \
    --print-memory-usage

# --- ARM32 sources -------------------------------------------
ARM32_ARCH_SRCS := \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm32/src/arch_init.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm32/src/arm_decode.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm32/src/arm_execute.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm32/src/arm_memory.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm32/src/arm_exceptions.c

ARM32_SRCS := \
    $(UIOX_ROOT)/main.c \
    $(ARM32_ARCH_SRCS) \
    $(SUBSYS_SRCS)

ARM32_OBJS := $(patsubst $(UIOX_ROOT)/%.c, \
                $(OBJ_DIR)/arm32/%.o, \
                $(ARM32_SRCS))

ARM32_ELF  := $(BIN_DIR)/uiox_arm32.elf
ARM32_BIN  := $(BIN_DIR)/uiox_arm32.bin
ARM32_HEX  := $(BIN_DIR)/uiox_arm32.hex
ARM32_LST  := $(BIN_DIR)/uiox_arm32.lst

# --- ARM32 build rules ---------------------------------------
.PHONY: arm32 arm32_clean

arm32: $(ARM32_ELF) $(ARM32_BIN) $(ARM32_HEX) $(ARM32_LST)
	@echo "[arm32] Build complete: $(ARM32_ELF)"
	@$(ARM32_SIZE) $(ARM32_ELF)

$(OBJ_DIR)/arm32/%.o: $(UIOX_ROOT)/%.c
	@$(MKDIR_P) $(dir $@)
	$(ARM32_CC) $(ARM32_CFLAGS) -c -o $@ $<

$(ARM32_ELF): $(ARM32_OBJS)
	@$(MKDIR_P) $(BIN_DIR) $(MAP_DIR)
	$(ARM32_CC) $(ARM32_CFLAGS) \
	    -Wl,$(subst $(space),$(comma),$(ARM32_LDFLAGS)) \
	    -o $@ $^

$(ARM32_BIN): $(ARM32_ELF)
	$(ARM32_OBJCOPY) -O binary $< $@

$(ARM32_HEX): $(ARM32_ELF)
	$(ARM32_OBJCOPY) -O ihex $< $@

$(ARM32_LST): $(ARM32_ELF)
	$(ARM32_OBJDUMP) -d -S $< > $@

arm32_clean:
	rm -rf $(OBJ_DIR)/arm32 $(ARM32_ELF) \
	       $(ARM32_BIN) $(ARM32_HEX) $(ARM32_LST)
