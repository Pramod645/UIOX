# =============================================================
# build_config/common.mk
#
# Flags and rules shared by all three architecture builds.
# Included by arm64.mk, arm32.mk, and x86_64.mk.
# =============================================================

# ── C standard and warnings ──────────────────────────────────
COMMON_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic \
                 -Wmissing-prototypes -Wstrict-prototypes \
                 -fno-common -ffunction-sections -fdata-sections \
                 -O2

# ── Include search paths (relative to uiox_target/) ──────────
UIOX_ROOT     ?= $(abspath ../..)
FS_INC        := $(UIOX_ROOT)/uiox_fs/include
DEV_INC       := $(UIOX_ROOT)/uiox_dev/include
HW_INC        := $(UIOX_ROOT)/uiox_hw/include

COMMON_INCS   := -I$(FS_INC) -I$(DEV_INC) -I$(HW_INC)

# ── Source files from all three sub-layers ───────────────────
FS_SRCS  := $(wildcard $(UIOX_ROOT)/uiox_fs/src/*.c)
DEV_SRCS := $(wildcard $(UIOX_ROOT)/uiox_dev/src/*.c)
HW_SRCS  := $(wildcard $(UIOX_ROOT)/uiox_hw/src/*.c)

# ── Archiver ─────────────────────────────────────────────────
AR       ?= ar
ARFLAGS  := rcs

# ── Linker flags ─────────────────────────────────────────────
COMMON_LDFLAGS := -Wl,--gc-sections

# ── Object directory (overridden per arch) ───────────────────
OBJ_DIR  ?= obj/unknown
LIB_DIR  ?= lib/unknown

# ── Common build rules ────────────────────────────────────────
$(OBJ_DIR)/%.o: $(UIOX_ROOT)/uiox_fs/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(UIOX_ROOT)/uiox_dev/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(UIOX_ROOT)/uiox_hw/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: arch/$(ARCH_DIR)/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/main.o: main.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

.PHONY: dirs
dirs: $(OBJ_DIR) $(LIB_DIR)
