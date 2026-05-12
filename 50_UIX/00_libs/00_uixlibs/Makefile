# ──────────────────────────────────────────────────────────────
# 00_uixlibs/Makefile
#
# Builds libUIXlib.a (static) and/or libUIXlib.so (shared)
# from every .c file that lives directly in this directory
# (uix_errno.c, uix_string.c, uix_stdio.c, … uix_regex.c).
#
# Usage:
#   make                      # build whatever BUILD_* flags say
#   make BUILD_STATIC=1       # static only  (default)
#   make BUILD_SHARED=1       # shared only
#   make BUILD_STATIC=1 BUILD_SHARED=1   # both
#   make clean                # remove obj/ and lib/
# ──────────────────────────────────────────────────────────────

# ── Pull in shared flags and pattern rules ───────────────────
include uixlibs.mk

# ── Compiler ─────────────────────────────────────────────────
CC      := gcc

# ── Effective CFLAGS: merge repo COMMON_CFLAGS + local extras ─
CFLAGS  := $(COMMON_CFLAGS) \
           -Wall -Wextra -fPIC -O2

# ── Library name ─────────────────────────────────────────────
LIB_NAME := UIXlib

# ── Source / object / library directories ────────────────────
SRC_DIR  := $(UIX_LIBS_DIR)       # all .c files sit here
OBJ_DIR  := ./00_obj
LIB_DIR  := ./00_lib

# ── Enumerate sources and derived objects ────────────────────
SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# ── Build-control flags (0 = off, 1 = on) ────────────────────
BUILD_SHARED ?= 0
BUILD_STATIC ?= 1

# ── Output targets (conditionally defined) ───────────────────
ifneq ($(BUILD_STATIC),0)
    STATIC := $(LIB_DIR)/lib$(LIB_NAME).a
endif

ifneq ($(BUILD_SHARED),0)
    SHARED := $(LIB_DIR)/lib$(LIB_NAME).so
endif

# ──────────────────────────────────────────────────────────────
.PHONY: all static shared clean dirs

# Default: build everything that is enabled
all: $(if $(filter-out 0,$(BUILD_STATIC)),static) \
     $(if $(filter-out 0,$(BUILD_SHARED)),shared)

# ── Static library target ────────────────────────────────────
ifneq ($(BUILD_STATIC),0)
static: $(STATIC)

$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built static library : $@"
endif

# ── Shared library target ─────────────────────────────────────
ifneq ($(BUILD_SHARED),0)
shared: $(SHARED)

$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -o $@ $^
	@echo "Built shared library : $@"
endif

# ── Compile .c → .o ──────────────────────────────────────────
# (The pattern rule from uixlibs.mk already covers this, but
#  we keep an explicit rule here as well for clarity.)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(COMMON_INCS) -c $< -o $@

# ── Directory creation ────────────────────────────────────────
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

dirs: $(OBJ_DIR) $(LIB_DIR)

# ── Clean ─────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
