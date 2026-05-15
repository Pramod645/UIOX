# ──────────────────────────────────────────────────────────────
# 00_uixlibs/Makefile  (macOS / Linux)
# ──────────────────────────────────────────────────────────────
CC      := gcc
AR      := ar
ARFLAGS := rcs

ROOT_DIR := $(abspath .)

SRC_DIRS := \
    $(ROOT_DIR)/PoStd   \
    $(ROOT_DIR)/arpa    \
    $(ROOT_DIR)/net     \
    $(ROOT_DIR)/netinet \
    $(ROOT_DIR)/sys

OBJ_DIR := $(ROOT_DIR)/00_obj
LIB_DIR := $(ROOT_DIR)/00_lib
LIB_NAME := UIXlib

# ── Compiler flags ────────────────────────────────────────────
# NOTE: -fcommon removed — uix_sys.h now uses static inline,
#       so there are no more duplicate symbols.
CFLAGS := \
    -std=c11                       \
    -Wall -Wextra                  \
    -Wno-unused-parameter          \
    -Wno-unused-variable           \
    -Wno-unused-but-set-parameter  \
    -Wno-missing-prototypes        \
    -Wno-strict-prototypes         \
    -Wno-incompatible-pointer-types \
    -Wno-pointer-bool-conversion   \
    -Wno-shift-overflow            \
    -Wno-return-type               \
    -Wno-empty-body                \
    -Wno-invalid-noreturn          \
    -fno-common                    \
    -fPIC -O2                      \
    $(foreach d,$(SRC_DIRS),-I$(d)) \
    -I$(ROOT_DIR)

# ── Sources ───────────────────────────────────────────────────
SRCS := $(foreach d,$(SRC_DIRS),$(wildcard $(d)/*.c))
ifeq ($(strip $(SRCS)),)
  $(error No .c files found under $(SRC_DIRS))
endif

OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(patsubst %.c,%.o,$(SRCS))))

# ── Build control ─────────────────────────────────────────────
BUILD_STATIC ?= 1
BUILD_SHARED ?= 1

ifneq ($(BUILD_STATIC),0)
  STATIC := $(LIB_DIR)/lib$(LIB_NAME).a
endif
ifneq ($(BUILD_SHARED),0)
  SHARED := $(LIB_DIR)/lib$(LIB_NAME).so
endif

# ─────────────────────────────────────────────────────────────
.PHONY: all static shared clean dirs

all: \
  $(if $(filter-out 0,$(BUILD_STATIC)),static) \
  $(if $(filter-out 0,$(BUILD_SHARED)),shared)

# ── Static library ────────────────────────────────────────────
ifneq ($(BUILD_STATIC),0)
static: $(STATIC)
$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built static: $@"
endif

# ── Shared library ────────────────────────────────────────────
ifneq ($(BUILD_SHARED),0)
shared: $(SHARED)
$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -o $@ $^
	@echo "Built shared: $@"
endif

# ── Per-subdirectory pattern rules ────────────────────────────
$(OBJ_DIR)/%.o: $(ROOT_DIR)/PoStd/%.c   | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/%.o: $(ROOT_DIR)/arpa/%.c    | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/%.o: $(ROOT_DIR)/net/%.c     | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/%.o: $(ROOT_DIR)/netinet/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/%.o: $(ROOT_DIR)/sys/%.c     | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
