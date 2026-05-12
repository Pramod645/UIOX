# =============================================================
# 00_uixlibs/uixlibs.mk
#
# Flags and rules shared by POSIX and Standard library builds.
# Included by the top-level Makefile.
# =============================================================

# ── C standard and warnings ──────────────────────────────────
COMMON_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic       \
                 -Wmissing-prototypes -Wstrict-prototypes  \
                 -fno-common -ffunction-sections           \
                 -fdata-sections                           \
                 -O2

# ── Include search paths ─────────────────────────────────────
# UIX_LIBS_DIR defaults to the directory containing this file.
# Override on the command line:
#   make UIX_LIBS_DIR=/path/to/00_uixlibs
UIX_LIBS_DIR  ?= $(abspath .)

COMMON_INCS   := -I$(UIX_LIBS_DIR)

# ── Source files ─────────────────────────────────────────────
# All .c files directly inside UIX_LIBS_DIR
ALL_SRCS      := $(wildcard $(UIX_LIBS_DIR)/*.c)

# ── Archiver ─────────────────────────────────────────────────
AR      ?= ar
ARFLAGS := rcs

# ── Linker flags ─────────────────────────────────────────────
COMMON_LDFLAGS :=

# ── Object and library output directories ───────────────────
OBJ_DIR ?= ./00_obj
LIB_DIR ?= ./00_lib

# ── Pattern rule: compile every .c → .o ──────────────────────
# NOTE: recipe lines MUST use a real TAB character.
$(OBJ_DIR)/%.o: $(UIX_LIBS_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -fPIC -c $< -o $@

# ── Directory creation ────────────────────────────────────────
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

.PHONY: dirs
dirs: $(OBJ_DIR) $(LIB_DIR)
