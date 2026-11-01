# =============================================================================
# 50_UIX/21_apps/Makefile — the ten UIOS applications
#
# Same contract as 20_uios: user binaries, no kernel objects, the same
# freestanding link (no crt0, entry at main).  Each app is a single .c at
# this level — there is no src/.
#
# app_sigtest is listed: uix_uios.c's g_apps[] has ten entries.
#
# install — stages <IMAGE_ROOT>/21_apps_/.  The trailing underscore is
# deliberate: it matches the path g_apps[] names (/../21_apps_/app_*),
# not the source directory.
# =============================================================================

MFDIR       := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE   = $(MFDIR)build
CC          ?= gcc
AR          ?= ar
ARCH_CFLAGS ?=
ARCH_CPUDEF ?=
_ARCH       ?= arm64
_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

CFLAGS := -std=c99 -Wall -Wextra \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -nostdinc -Os -g \
          $(ARCH_CFLAGS) $(ARCH_CPUDEF) \
          -I$(MFDIR)../../40_SCIX \
          -I$(MFDIR)../../30_KIX/common \
          -I$(MFDIR)../00_libs/00_uixlibs \
          -I$(MFDIR)../00_libs/00_uixlibs/PoStd \
          -DSYS_CALL_ENBLE_DISABLE=0

LDFLAGS := -nostdlib -nostartfiles -ffreestanding -Wl,-e,main

APPS := app_shell   app_logger   app_fileserver app_netserver \
        app_ipctest app_memtest app_scheduler  app_sigtest   \
        app_fstest  app_clocktest

TARGETS := $(addprefix $(_BUILD_DIR)/,$(APPS))

# Same formula as 20_uios, so both installs land in one image tree.
IMAGE_ROOT := $(BUILD_BASE)/$(_ARCH)/image

# ── THE ARCH TARGETS DO THE INSTALL THEMSELVES ────────────────────────
# Same reason as 20_uios: the per-arch recipe runs in the parent make,
# where `_ARCH` is still `arm64`.  Doing the install in the sub-make is
# what makes `make arm32` stage the arm32 tree.
#
# NOTE ON THE `install` TARGET BELOW: it was COPIED from 20_uios and is
# WRONG HERE.  This file builds ten binaries, has no $(TARGET), and its
# install belongs in $(IMAGE_ROOT)/21_apps_/ — which is what the real
# recipe below it does.  As written it referenced an undefined $(TARGET)
# and would have failed if ever invoked.  It is REMOVED, not repaired,
# because _build_install supersedes it.
.PHONY: arm64 arm32 riscv64 x86_64
arm64 arm32 riscv64 x86_64:
	$(MAKE) -f $(MFDIR)Makefile _build_install   \
	    _ARCH=$@                                 \
	    CC="$(CC)" AR="$(AR)"                    \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)"             \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)"

.PHONY: _build
_build: $(TARGETS)
	@echo "[APPS] $(_ARCH) → $(words $(APPS)) binaries in $(_BUILD_DIR)"

.PHONY: _build_install
_build_install: $(TARGETS)
	@echo "[APPS] $(_ARCH) → $(words $(APPS)) binaries in $(_BUILD_DIR)"
	@mkdir -p $(IMAGE_ROOT)/21_apps_
	@for a in $(APPS); do \
	    cp $(_BUILD_DIR)/$$a $(IMAGE_ROOT)/21_apps_/$$a; \
	done
	@echo "[APPS] installed $(words $(APPS)) → $(IMAGE_ROOT)/21_apps_"

$(_BUILD_DIR)/%: %.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $<"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

.PHONY: srclist
srclist:
	@echo "[APPS] $(words $(APPS)) apps:"
	@for a in $(APPS); do echo "  $$a"; done

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	@rm -rf $(BUILD_BASE)
	@echo "[CLN] 21_apps build outputs removed"
