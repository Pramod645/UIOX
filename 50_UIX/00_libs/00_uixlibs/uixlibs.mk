# =============================================================
# 00_uixlibs/uixlibs.mk
#
# Shared compiler flags and pattern rules.
# Included by Makefile — do NOT define OBJ_DIR / LIB_DIR
# targets here; Makefile owns those to avoid duplicate warnings.
# =============================================================

# ── C standard and warnings ──────────────────────────────────
COMMON_CFLAGS := \
    -std=c11        \
    -Wall           \
    -Wextra         \
    -Wpedantic      \
    -Wmissing-prototypes  \
    -Wstrict-prototypes   \
    -fno-common           \
    -ffunction-sections   \
    -fdata-sections       \
    -O2

# ── Include search path ───────────────────────────────────────
# All headers live flat in the same directory as the sources.
# SRC_DIR is set by Makefile before including this file.
COMMON_INCS = -I$(SRC_ARPA)
COMMON_INCS += -I$(SRC_NET)
COMMON_INCS += -I$(SRC_NETINET)
COMMON_INCS += -I$(SRC_POSTD)
COMMON_INCS += -I$(SRC_SYS)

# ── Archiver defaults (Makefile may override) ─────────────────
AR      ?= ar
ARFLAGS ?= rcs

# ── Linker flags ──────────────────────────────────────────────
# -Wl,--gc-sections is a GNU ld option; safe on Linux/macOS+ld64.
COMMON_LDFLAGS :=

# ── Pattern rule: compile one .c → one .o ─────────────────────
# Makefile sets SRC_DIR, OBJ_DIR before including this file.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -fPIC -c $< -o $@
