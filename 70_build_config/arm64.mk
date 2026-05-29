# =============================================================
# build_config/arm64.mk
# ARM64 (AArch64 / ARMv8-A) compiler and linker configuration.
# =============================================================

include build_config/tools.mk
include build_config/common.mk

# --- ARM64 arch-specific compiler flags ----------------------
ARM64_ARCH_FLAGS := \
    -march=armv8-a \
    -mtune=cortex-a53 \
    -mabi=lp64 \
    -mstrict-align

# --- ARM64 preprocessor defines ------------------------------
ARM64_DEFS := \
    -DUIOX_ARCH_ARM64=1 \
    -DUIOX_BITS=64 \
    -DUIOX_ENDIAN_LITTLE=1

# --- ARM64 include paths -------------------------------------
ARM64_INCLUDES := \
    -I$(UIOX_ROOT)/$(DIR_ARCH)/arm64/include \
    $(SUBSYS_INCLUDES)

# --- Combined ARM64 CFLAGS -----------------------------------
ARM64_CFLAGS := \
    $(COMMON_CFLAGS) \
    $(ARM64_ARCH_FLAGS) \
    $(ARM64_DEFS) \
    $(ARM64_INCLUDES)

# --- ARM64 linker flags --------------------------------------
ARM64_LDFLAGS := \
    $(COMMON_LDFLAGS) \
    -T $(UIOX_ROOT)/$(DIR_LINKER)/uiox_arm64.ld \
    -Map=$(MAP_DIR)/uiox_arm64.map \
    --print-memory-usage

# --- ARM64 sources -------------------------------------------
ARM64_ARCH_SRCS := \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm64/src/arch_init.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm64/src/arm64_decode.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm64/src/arm64_execute.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm64/src/arm64_memory.c \
    $(UIOX_ROOT)/$(DIR_ARCH)/arm64/src/arm64_exceptions.c

ARM64_SRCS := \
    $(UIOX_ROOT)/main.c \
    $(ARM64_ARCH_SRCS) \
    $(SUBSYS_SRCS)

ARM64_OBJS := $(patsubst $(UIOX_ROOT)/%.c, \
                $(OBJ_DIR)/arm64/%.o, \
                $(ARM64_SRCS))

ARM64_ELF  := $(BIN_DIR)/uiox_arm64.elf
ARM64_BIN  := $(BIN_DIR)/uiox_arm64.bin
ARM64_HEX  := $(BIN_DIR)/uiox_arm64.hex
ARM64_LST  := $(BIN_DIR)/uiox_arm64.lst

# --- ARM64 build rules ---------------------------------------
.PHONY: arm64 arm64_clean

arm64: $(ARM64_ELF) $(ARM64_BIN) $(ARM64_HEX) $(ARM64_LST)
	@echo "[arm64] Build complete: $(ARM64_ELF)"
	@$(ARM64_SIZE) $(ARM64_ELF)

$(OBJ_DIR)/arm64/%.o: $(UIOX_ROOT)/%.c
	@$(MKDIR_P) $(dir $@)
	$(ARM64_CC) $(ARM64_CFLAGS) -c -o $@ $<

$(ARM64_ELF): $(ARM64_OBJS)
	@$(MKDIR_P) $(BIN_DIR) $(MAP_DIR)
	$(ARM64_CC) $(ARM64_CFLAGS) \
	    -Wl,$(subst $(space),$(comma),$(ARM64_LDFLAGS)) \
	    -o $@ $^

$(ARM64_BIN): $(ARM64_ELF)
	$(ARM64_OBJCOPY) -O binary $< $@

$(ARM64_HEX): $(ARM64_ELF)
	$(ARM64_OBJCOPY) -O ihex $< $@

$(ARM64_LST): $(ARM64_ELF)
	$(ARM64_OBJDUMP) -d -S $< > $@

arm64_clean:
	rm -rf $(OBJ_DIR)/arm64 $(ARM64_ELF) \
	       $(ARM64_BIN) $(ARM64_HEX) $(ARM64_LST)
