# =============================================================================
# 30_KIX/34_CAS/Makefile  — v1.2.0
#
# Communication and Application Services — kernel IPC, signals, pipes,
# sockets and related communication primitives.
# NOT a device-driver layer — no ENABLE_* flags needed.
#
# Outputs: build/<arch>/libcas.a
# @version 1.2.0  @date 2026-07-26
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  = $(MFDIR)build
SRCDIR      = $(MFDIR)src
INCDIR      = $(MFDIR)include

CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
_ARCH       ?= arm64

_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror          \
  -ffreestanding -fno-builtin             \
  -fno-stack-protector                    \
  -ffunction-sections -fdata-sections     \
  -nostdinc -Os -g                        \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)           \
  -I$(INCDIR)                             \
  -I$(MFDIR)../common                     \
  -I$(MFDIR)../33_PCS/include

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(_BUILD_DIR)/%.o,$(SRCS))
LIB  := $(_BUILD_DIR)/libcas.a

# =============================================================================
# Arch entry points — re-invoke with _ARCH=$@ so $(_BUILD_DIR) is defined.
# =============================================================================
.PHONY: arm64 arm32 riscv64 x86_64

arm64 arm32 riscv64 x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build  \
	    _ARCH=$@                        \
	    CC="$(CC)" AR="$(AR)"           \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)"    \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)"

.PHONY: _build
_build: $(LIB)
	@echo "[CAS] $(_BUILD_DIR)/libcas.a  ($(words $(SRCS)) files)"

$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

$(_BUILD_DIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(COMMON_CFLAGS) -MMD -MP -c $< -o $@

-include $(wildcard $(_BUILD_DIR)/*.d)

.PHONY: clean
clean:
	rm -rf $(BUILD_BASE)
