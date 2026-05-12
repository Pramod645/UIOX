# =============================================================
# 00_uixlibs/uixlibs.mk
#
# Flags and rules shared by POXIS and Standerd library builds.
# Included by uixlibs.mk.
# =============================================================

# ── C standard and warnings ──────────────────────────────────
COMMON_CFLAGS := -std=c11 -Wall -Wextra -Wpedantic \
                 -Wmissing-prototypes -Wstrict-prototypes \
                 -fno-common -ffunction-sections -fdata-sections \
                 -O2

# ── Include search paths (relative to uiox_target/) ──────────
UIX_LIBS      ?= $(abspath ./..)
ARPA_INC      := $(UIX_LIBS)/arpa/include
NET_INC       := $(UIX_LIBS)/net/include
NETINET_INC   := $(UIX_LIBS)/netinet/include
POSTD_INC     := $(UIX_LIBS)/PoStd/include
SYS_INC       := $(UIX_LIBS)/sys/include

COMMON_INCS   := -I$(ARPA_INC) -I$(NET_INC) -I$(NETINET_INC) -I$(POSTD_INC) -I$(SYS_INC)

# ── Source files from all three sub-layers ───────────────────
ARPA_SRCS  := $(wildcard $(UIX_LIBS)/arpa/*.c)
NET_SRCS := $(wildcard $(UIX_LIBS)/net/*.c)
NETINET_SRCS  := $(wildcard $(UIX_LIBS)/netinet/*.c)
POSTD_SRCS  := $(wildcard $(UIX_LIBS)/PoStd/*.c)
SYS_SRCS  := $(wildcard $(UIX_LIBS)/sys/*.c)

# ── Archiver ─────────────────────────────────────────────────
AR       ?= ar
ARFLAGS  := rcs

# ── Linker flags ─────────────────────────────────────────────
COMMON_LDFLAGS := -Wl,--gc-sections

# ── Object directory (overridden per arch) ───────────────────
OBJ_DIR  ?= obj/unknown
LIB_DIR  ?= lib/unknown

# ── Common build rules ────────────────────────────────────────
$(OBJ_DIR)/%.o: $(UIX_LIBS)/uiox_fs/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(UIX_LIBS)/uiox_dev/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: $(UIX_LIBS)/uiox_hw/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/%.o: arch/$(ARCH_DIR)/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

#$(OBJ_DIR)/main.o: main.c | $(OBJ_DIR)
#	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

.PHONY: dirs
dirs: $(OBJ_DIR) $(LIB_DIR)
