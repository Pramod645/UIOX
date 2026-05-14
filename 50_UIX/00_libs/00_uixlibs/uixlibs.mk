# =============================================================
# 00_uixlibs/uixlibs.mk
#
# Shared flags and pattern rule.
# Included by Makefile AFTER SRC_DIR and OBJ_DIR are set.
# Do NOT define $(OBJ_DIR) or $(LIB_DIR) targets here.
# =============================================================

# ── C standard and warnings ──────────────────────────────────
COMMON_CFLAGS := \
    -std=c11              \
    -Wall                 \
    -Wextra               \
    -Wno-unused-parameter \
    -Wno-missing-prototypes \
    -Wno-strict-prototypes  \
    -fno-common           \
    -O2

# ── Include paths ─────────────────────────────────────────────
# Headers live in each sub-directory and in the root.
# SRC_DIRS is set by Makefile before this file is included.
COMMON_INCS = $(foreach d,$(SRC_DIRS),-I$(d)) -I$(ROOT_DIR)

# ── Archiver defaults ─────────────────────────────────────────
AR      ?= ar
ARFLAGS ?= rcs

# ── Linker flags ──────────────────────────────────────────────
COMMON_LDFLAGS :=

# ── Pattern rules: compile .c → .o ───────────────────────────
# One rule per sub-directory so make can find the right source.
# ROOT_DIR and SRC_DIRS are set by Makefile.
$(OBJ_DIR)/%.o: $(ROOT_DIR)/PoStd/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(ROOT_DIR)/arpa/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(ROOT_DIR)/net/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(ROOT_DIR)/netinet/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(ROOT_DIR)/sys/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -fPIC -c $< -o $@
