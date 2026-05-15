# ──────────────────────────────────────────────────────────────
# 00_uixlibs/Makefile
#
# Builds libUIXlib.a (static) and/or libUIXlib.so (shared)
# from source files in:
#   PoStd/   arpa/   net/   netinet/   sys/
#
# Usage:
#   make                         # static only (default)
#   make BUILD_STATIC=1          # static
#   make BUILD_SHARED=1          # shared
#   make BUILD_STATIC=1 BUILD_SHARED=1
#   make clean
# ──────────────────────────────────────────────────────────────

# ── Compiler ──────────────────────────────────────────────────
CC := gcc

# ── Root of the 00_uixlibs tree (where this Makefile lives) ───
ROOT_DIR := $(abspath .)

# ── Sub-directories that contain .c source files ─────────────
SRC_DIRS := \
    $(ROOT_DIR)/PoStd    \
    $(ROOT_DIR)/arpa     \
    $(ROOT_DIR)/net      \
    $(ROOT_DIR)/netinet  \
    $(ROOT_DIR)/sys

# ── Output directories ────────────────────────────────────────
OBJ_DIR := $(ROOT_DIR)/00_obj
LIB_DIR := $(ROOT_DIR)/00_lib

# ── Pull in shared flags and pattern rules ────────────────────
include uixlibs.mk

# ── Effective compile flags ───────────────────────────────────
# -fcommon: allows multiple tentative definitions of the same
#   symbol across translation units (needed because uix_sys.h
#   defines sys_* functions without 'static', so every .c that
#   includes it gets a copy — -fcommon lets the linker merge them
#   silently instead of erroring on duplicate symbols).
#   The correct long-term fix is to add 'static' to all sys_*
#   definitions in uix_sys.h, but -fcommon is the safe shim.
CFLAGS := $(COMMON_CFLAGS) \
          -fPIC             \
          -fcommon          \
          -Wno-int-conversion \
          -Wno-incompatible-pointer-types \
          -Wno-pointer-bool-conversion    \
          -Wno-shift-overflow             \
          -Wno-return-type                \
          -Wno-empty-body                 \
          -Wno-invalid-noreturn

# ── Library name ──────────────────────────────────────────────
LIB_NAME := UIXlib

# ── Discover all .c files across sub-directories ─────────────
SRCS := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.c))

# Safety check
ifeq ($(strip $(SRCS)),)
  $(error No .c files found in $(SRC_DIRS))
endif

# ── Derive object list (all land flat in OBJ_DIR) ────────────
# notdir strips the sub-directory prefix so every .o is unique
# by filename — which is guaranteed because no two sub-dirs
# contain a file with the same name.
OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))

# ── Build-control flags ────────────────────────────────────────
BUILD_STATIC ?= 0
BUILD_SHARED ?= 1

# ── Output targets ────────────────────────────────────────────
ifneq ($(BUILD_STATIC),0)
STATIC := $(LIB_DIR)/lib$(LIB_NAME).a
endif

ifneq ($(BUILD_SHARED),0)
SHARED := $(LIB_DIR)/lib$(LIB_NAME).so
endif

# ──────────────────────────────────────────────────────────────
.PHONY: all static shared clean dirs

all: \
  $(if $(filter-out 0,$(BUILD_STATIC)),static) \
  $(if $(filter-out 0,$(BUILD_SHARED)),shared)

# ── Static library ─────────────────────────────────────────────
ifneq ($(BUILD_STATIC),0)
static: $(STATIC)

$(STATIC): $(OBJS) | $(LIB_DIR)
	@echo "  AR   $@"
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built static library: $@"
endif

# ── Shared library ─────────────────────────────────────────────
ifneq ($(BUILD_SHARED),0)
shared: $(SHARED)

$(SHARED): $(OBJS) | $(LIB_DIR)
	@echo "  LD   $@"
	$(CC) -shared -o $@ $^
	@echo "Built shared library: $@"
endif

# ── Directory creation (owned here only) ───────────────────────
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

dirs: $(OBJ_DIR) $(LIB_DIR)

# ── Clean ──────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
