# =============================================================================
# 50_UIX/21_apps/Makefile — the ten UIOS applications
#
# Same contract as 20_uios: user binaries, no kernel objects, the same
# freestanding link (no crt0, entry at main).  Each app is a single .c at
# this level — there is no src/.
#
# app_sigtest is listed: uix_uios.c's g_apps[] has ten entries.
#
# ── v2.1.0: the toolchain is CROSS, per arch ─────────────────────────
#   The build ran with CC="cc" — Apple clang — and linked
#   libclang_rt.osx.a, the macOS runtime.  `cc` is the host compiler and
#   produces Mach-O; every other target in this tree is ELF.  Two things
#   followed from it:
#
#     · $(CC) -print-libgcc-file-name returned a Darwin runtime library,
#       so LIBS was wrong in the same breath as the objects were.
#     · the app would never have linked against the ELF library archive
#       anyway — the same "not a mach-o file" failure the shell hit.
#
#   CC is now per-arch, and the four arch targets pass it explicitly,
#   with the arch flag sets copied from 00_uixlibs so the ABI agrees.
#
# ── v2.0.0: app_shell LINKS THE FRONT-END ────────────────────────────
#   app_shell was a 35-line standalone main; the shell itself is 386
#   lines living in 01_shell/01_cli/src/uiox_sh.c, which is the layer
#   that owns it.  21_apps is the PACKAGE layer, so app_shell.c is its
#   entry and does one thing: call the front-end.
#
#       app_shell.c   main() { return uix_sh_main(); }
#       uiox_sh.c     int uix_sh_main(void)   ← the front-end entry
#
#   Two mains cannot both be -Wl,-e,main, so the shell's is guarded by
#   UIOX_SH_NO_MAIN, which this Makefile passes when building the app.
#   The standalone build in 01_cli still gets its own main.
#
#   LINKING THE SOURCE, not an object from 01_cli/build: that directory
#   holds a LINKED BINARY (sh), not a relocatable object, and pulling
#   objects out of it would need a second build rule over there.
#
# install — stages <IMAGE_ROOT>/21_apps_/.  The trailing underscore is
# deliberate: it matches the path g_apps[] names (/../21_apps_/app_*),
# not the source directory.
# =============================================================================

MFDIR       := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE   = $(MFDIR)build
_ARCH       ?= arm64
_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

# ── toolchain — cross, mirroring 00_uixlibs and 01_cli ────────────────
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC := aarch64-elf-gcc
  ARM32_CC := arm-none-eabi-gcc
  RV64_CC  := riscv64-elf-gcc
  X86_CC   := x86_64-elf-gcc
else
  ARM64_CC := aarch64-linux-gnu-gcc
  ARM32_CC := arm-linux-gnueabihf-gcc
  RV64_CC  := riscv64-linux-gnu-gcc
  X86_CC   := x86_64-linux-gnu-gcc
endif

# ── arch flag sets — identical to 00_uixlibs and 01_cli ──────────────
ARM64_FLAGS   := -march=armv8-a -mabi=lp64 -mlittle-endian
ARM32_FLAGS   := -march=armv7-a -marm -mfloat-abi=soft -mlittle-endian
RV64_FLAGS    := -march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany
X86_FLAGS     := -m64 -mno-red-zone -mno-mmx -mcmodel=kernel

ARM64_CPUDEF  := -D__aarch64__
ARM32_CPUDEF  := -D__arm__
RV64_CPUDEF   := -D__riscv -D__riscv_xlen=64
X86_CPUDEF    := -D__x86_64__

# 21_apps → 50_UIX, then the shell's home and the library.
UIXROOT     := $(MFDIR)../..
SHLIB       := $(MFDIR)../01_shell/01_cli/src
UIXLIB      := $(UIXROOT)/50_UIX/00_libs/00_uixlibs

CFLAGS := -std=c99 -Wall -Wextra \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -nostdinc -Os -g \
          $(ARCH_CFLAGS) $(ARCH_CPUDEF) \
          -I$(UIXROOT)/40_SCIX \
          -I$(UIXROOT)/30_KIX/common \
          -I$(UIXLIB) \
          -I$(UIXLIB)/PoStd \
          -I$(UIXLIB)/sys \
          -I$(SHLIB)

LDFLAGS := -nostdlib -nostartfiles -ffreestanding -Wl,-e,main

# The library archive, per arch.  Same path 01_cli links.
UIXLIB_AR := $(UIXLIB)/build/$(_ARCH)/libUIXlib.a

# libgcc is the COMPILER's runtime and is neither libc nor UIOX: arm32's
# -mfloat-abi=soft emits calls to __aeabi_dmul/dadd/ddiv/i2d, and arm64's
# atomics to __aarch64_ldadd4_acq_rel/swp4_acq_rel.  Asked of the compiler
# that compiled the objects.
# `=` not `:=` — $(CC) is set by the per-arch sub-make, so the $(shell …)
# must not run until the recipe expands.  With `:=` it would evaluate in
# the PARENT, where CC is unset, and return the host runtime — which is
# exactly the libclang_rt.osx.a that appeared in the failing link.
LIBS = $(UIXLIB_AR) $(shell $(CC) -print-libgcc-file-name)

APPS := app_shell   app_logger   app_fileserver app_netserver \
        app_ipctest app_memtest app_scheduler  app_sigtest   \
        app_fstest  app_clocktest

TARGETS := $(addprefix $(_BUILD_DIR)/,$(APPS))

# Same formula as 20_uios, so both installs land in one image tree.
IMAGE_ROOT := $(BUILD_BASE)/$(_ARCH)/image

# ── arch entry points ─────────────────────────────────────────────────
# Same shape as 01_cli: the per-arch recipe re-invokes in the SUB-make,
# where _ARCH is already correct.  Handing _ARCH down from the parent
# does not work — the parent's _ARCH is still the default, because
# `arm32` is a target, not a variable.
#
# The toolchain check is a hard stop, not a fallback: if aarch64-elf-gcc
# is absent, `cc` would produce Mach-O and link the macOS runtime again.
.PHONY: arm64
arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=arm64 CC="$(ARM64_CC)" \
	    ARCH_CFLAGS="$(ARM64_FLAGS)" ARCH_CPUDEF="$(ARM64_CPUDEF)"

.PHONY: arm32
arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=arm32 CC="$(ARM32_CC)" \
	    ARCH_CFLAGS="$(ARM32_FLAGS)" ARCH_CPUDEF="$(ARM32_CPUDEF)"

.PHONY: riscv64
riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=riscv64 CC="$(RV64_CC)" \
	    ARCH_CFLAGS="$(RV64_FLAGS)" ARCH_CPUDEF="$(RV64_CPUDEF)"

.PHONY: x86_64
x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build_install _ARCH=x86_64 CC="$(X86_CC)" \
	    ARCH_CFLAGS="$(X86_FLAGS)" ARCH_CPUDEF="$(X86_CPUDEF)"

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "[APPS] all four arches built and staged"

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

# ── app_shell — the ONLY app with an external source ──────────────────
# -DUIOX_SH_NO_MAIN keeps uiox_sh.c from defining a second main; the
# entry is app_shell.c's.  uiox_sh.c carries the guard:
#
#     #ifndef UIOX_SH_NO_MAIN
#     int main(void) { return uix_sh_main(); }
#     #endif
#
# and uiox_sh.h (beside it) declares uix_sh_main for app_shell.c.
$(_BUILD_DIR)/app_shell: $(MFDIR)app_shell.c $(SHLIB)/uiox_sh.c $(SHLIB)/uiox_sh.h
	@mkdir -p $(dir $@)
	@echo "  [CC]  $(_ARCH)  app_shell.c + uiox_sh.c"
	$(CC) $(CFLAGS) -DUIOX_SH_NO_MAIN $(LDFLAGS) \
	    -o $@ $(MFDIR)app_shell.c $(SHLIB)/uiox_sh.c $(LIBS)

# ── every other app is one self-contained .c ──────────────────────────
$(_BUILD_DIR)/%: %.c
	@mkdir -p $(dir $@)
	@echo "  [CC]  $(_ARCH)  $<"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# ── check-binary — is the linked app self-contained? ──────────────────
# Anything still undefined here is something the image cannot supply.
# Run with the arch named: make check-binary _ARCH=arm32
.PHONY: check-binary
check-binary:
	@b=$(_BUILD_DIR)/app_shell; \
	[ -f $$b ] || { echo "[APPS] no $$b — build first"; exit 1; }; \
	echo "undefined in app_shell [$(_ARCH)]:"; \
	nm -u $$b 2>/dev/null | sed 's/^ *U //' | sort -u | sed 's/^/  /' \
	  || echo "  (none)"; \
	echo "[APPS] ELF:"; \
	readelf -h $$b 2>/dev/null | sed -n 's/^  \(Class\|Machine\|Type\):/\1:/p' | sed 's/^/  /' || true

.PHONY: info
info:
	@echo "HOST=$(UNAME_S)"
	@echo "  UIXLIB_AR = $(UIXLIB_AR)"
	@echo "  ARM64_CC  = $(ARM64_CC)"
	@echo "  ARM32_CC  = $(ARM32_CC)"
	@echo "  RV64_CC   = $(RV64_CC)"
	@echo "  X86_CC    = $(X86_CC)"

.PHONY: srclist
srclist:
	@echo "[APPS] $(words $(APPS)) apps:"
	@for a in $(APPS); do echo "  $$a"; done
	@echo "[APPS] app_shell links: $(SHLIB)/uiox_sh.c"

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	@rm -rf $(BUILD_BASE)
	@echo "[CLN] 21_apps build outputs removed"
