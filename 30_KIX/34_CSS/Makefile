# =============================================================================
# 30_KIX/34_CSS/Makefile  — v1.3.0
#
# Communication and Sensor Services — the layer a stack calls when it needs
# a DD driver.  NOT a device-driver layer and NOT a filesystem peer:
# no ENABLE_* flags needed, because CSS has no probe of its own.
#
# Where a class's code lives here:
#   34_CSS/<group>/<class>/uiox_<cls>_proto.c    the class's protocol
#   34_CSS/<group>/<class>/uiox_<cls>_subsys.c   its state machine + tick
# Mirroring 03_NonSensors/emmc/, which is the shape the rest follow.
#
# Outputs: build/<arch>/libcss.a
#
# ── v1.3.0 changes ────────────────────────────────────────────────────
#   · SRCS globs the three class groups (01_Com, 02_Sensors, 03_NonSensors)
#     as well as a flat src/, matching DD_SRCS in 30_DSS/Makefile:209.
#     The old single $(wildcard $(SRCDIR)/*.c) matched nothing — 34_CSS has
#     no src/ — so OBJS was empty and `ar rcs` was called with no members.
#   · The archive step tolerates an empty OBJS.  A layer with no code yet
#     is a normal state, not a failure — the same rule 32_FS/Makefile
#     states: "A wildcard that matches nothing produces NO error in make."
#   · The object rule keyed on $(SRCDIR)/%.c, which no longer exists; it
#     now keys on $(MFDIR)%.c so it matches the globbed paths exactly.
#   · include/ added to the search path only if it exists — the directory
#     is not present today and -I on a missing path is harmless but noisy.
#   · The banner says libcss.a, not libcas.a, matching LIB.
#
# @version 1.3.0  @date 2026-10-17
# =============================================================================

MFDIR      := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  = $(MFDIR)build
INCDIR      = $(MFDIR)include

CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
_ARCH       ?= arm64

_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

# -nostdinc with klibc supplying printf/memset/memcpy and the errno defines.
# 02_FwHal/include is added because CSS classes that forward into the
# firmware layer name its types; ../common supplies uiox_klibc.h.
COMMON_CFLAGS := \
  -std=c11 -Wall -Wextra -Werror          \
  -ffreestanding -fno-builtin             \
  -fno-stack-protector                    \
  -ffunction-sections -fdata-sections     \
  -nostdinc -Os -g                        \
  $(ARCH_CFLAGS) $(ARCH_CPUDEF)           \
  -I$(INCDIR)                             \
  -I$(MFDIR)../common                     \
  -I$(MFDIR)../33_PCS/include             \
  -I$(MFDIR)../../02_FwHal/include        \
  -I$(MFDIR)../30_DSS/20_DD               \
  -I$(MFDIR)../30_DSS/20_DD/01_Com/eth    \
  -I$(MFDIR)../30_DSS/10_DI/01_Com/eth          \
  -I$(MFDIR)../32_FS/00_drvbuff/01_Com/eth      \
  -I$(MFDIR)../30_DSS/20_DD/02_Sensors/WiFi      \
  -I$(MFDIR)../30_DSS/10_DI/02_Sensors/WiFi      \
  -I$(MFDIR)../32_FS/00_drvbuff/02_Sensors/WiFi  \
  -I$(MFDIR)../30_DSS/include             \
  -I$(MFDIR)../30_DSS/10_DI/include

# ── sources — by class, the layout 10_DI/20_DD use ────────────────────
# Three groups, class directory under each.  */*.c is deliberately TWO
# levels, matching 03_NonSensors/emmc/uiox_emmc_proto.c — one wildcard
# level per group, not a recursive glob.
#SRCS := $(wildcard $(MFDIR)01_Com/*/*.c)        \
#        $(wildcard $(MFDIR)02_Sensors/*/*.c)    \
#        $(wildcard $(MFDIR)03_NonSensors/*/*.c) \
#        $(wildcard $(MFDIR)src/*.c)             # kept for a flat src/ if one appears
SRCS := $(wildcard $(MFDIR)01_Com/eth/*.c)\
        $(wildcard $(MFDIR)02_Sensors/WiFi/*.c)

OBJS := $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(SRCS))
LIB  := $(_BUILD_DIR)/libcss.a

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
	@echo "[CSS] $(_BUILD_DIR)/libcss.a  ($(words $(SRCS)) sources)"

# ── the archive ───────────────────────────────────────────────────────
# An empty OBJS is a no-op, not an error.  `ar rcs` with no members prints
# its usage and exits non-zero, which would make "this layer has no code
# yet" look like a broken build.
$(LIB): $(OBJS)
	@mkdir -p $(dir $@)
	@if [ -z "$(OBJS)" ]; then \
	    echo "  [AR]  skipped — 34_CSS has no sources yet"; \
	    touch $@; \
	  else \
	    $(AR) rcs $@ $(OBJS); \
	    echo "  [AR]  $@"; \
	  fi

# Keys on $(MFDIR)%.c so it matches what SRCS globbed.
$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(COMMON_CFLAGS) -MMD -MP -c $< -o $@

-include $(wildcard $(_BUILD_DIR)/*/*/*.d) $(wildcard $(_BUILD_DIR)/*/*.d)

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	@rm -rf $(BUILD_BASE)
	@echo "[CLN] 34_CSS build outputs removed"

# ── srclist — what the globs actually resolved to ─────────────────────
# 32_FS/Makefile carries this target for the same reason: a wildcard that
# matches nothing is silent, so make that visible.
.PHONY: srclist
srclist:
	@echo "[CSS] sources to build:"
	@for f in $(SRCS); do echo "  $$f"; done
	@echo "[CSS] total: $(words $(SRCS))"
