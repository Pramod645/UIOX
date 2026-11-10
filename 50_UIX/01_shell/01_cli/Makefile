# =============================================================================
# 50_UIX/01_shell/01_cli/Makefile — the command-line front-end
#
# PATH    50_UIX/01_shell/01_cli/Makefile
#
# A USER program.  Entry point main(), -nostdlib, no libc — the shape
# 50_UIX/20_uios/Makefile uses for its own binary.
#
# ── LINKS THE LIBRARY, not the sources ──────────────────────────────────
# 00_uixlibs produces  build/<arch>/libUIXlib.a  (56 objects).
# So this Makefile compiles ONE file and links the archive:
#
#     uix_read / uix_write / uix_open / uix_close      libUIXlib.a
#     uix_chdir / uix_getcwd                           libUIXlib.a
#     uix_fdopendir / uix_readdir / uix_closedir       libUIXlib.a
#
# ── v2.3.0: libgcc is linked, per arch ──────────────────────────────────
#   A freestanding link needs the COMPILER's runtime, which is neither
#   libc nor UIOX.  Two arches made that visible:
#
#   arm32  -mfloat-abi=soft emits CALLS to the EABI soft-float helpers
#          rather than hardware FP instructions, so uix_strtod's double
#          arithmetic needs __aeabi_dmul / __aeabi_dadd / __aeabi_ddiv /
#          __aeabi_i2d.  These live in libgcc.a for arm-none-eabi.
#
#   arm64  the atomics emit calls to __aarch64_ldadd4_acq_rel and
#          __aarch64_swp4_acq_rel.  arm64 linked anyway only because
#          nothing in the shell's call graph reached the function that
#          uses them — GCC pulls archive members in lazily, so the gap
#          was latent rather than fatal.  It would have surfaced the
#          moment the shell called that function.
#
#   LIBGCC is asked of the compiler that compiled the objects, so the
#   path is right per arch and per install location — no hard-coded
#   /opt/homebrew/... and no version number to go stale.
#
#   LIBS is recursively expanded (`=` not `:=`): $(CC) is set by the
#   per-arch sub-make invocation, so the $(shell ...) must not run until
#   the recipe expands, by which point CC is already arm-none-eabi-gcc.
#   With `:=` it would evaluate at parse time in the PARENT, where CC is
#   whatever the caller's shell had — including empty.
#
# ── v2.2.0: the toolchain is CROSS, per arch ────────────────────────────
#   The archive is ELF, built by aarch64-elf-ar.  `cc` on macOS is clang
#   and its linker reads Mach-O, so it reported
#
#       ld: archive member '/' not a mach-o file in libUIXlib.a
#
#   That is not a missing-archive error and no ld flag fixes it: the
#   object FORMATS differ.  cc compiled uiox_sh.c to Mach-O arm64 while
#   the library holds ELF aarch64 — two representations with no common
#   one.  The fix is the same toolchain that built the archive.
#
#   The `CC ?= cc` default was right for a Makefile linking nothing from
#   00_uixlibs.  It is wrong for this one.
#
# ── v2.1.0: the archive moved ───────────────────────────────────────────
#   The library's output moved from
#       $(UIXLIB)/00_lib/libUIXlib.a        (old, per-arch-blind)
#   to
#       $(UIXLIB)/build/<arch>/libUIXlib.a  (per-arch, correct)
#
#   The old path was the ad-hoc Makefile's output directory.  Once it was
#   removed, `-L` pointed at a directory that does not exist and `-l` had
#   nothing to find — a `search path not found` warning, which is a
#   different failure from an unresolved symbol.
#
#   UIXLIB_DIR derives from _ARCH, so arm64 links arm64 objects.  One
#   directory for all four was the same class of bug as the shared 00_obj:
#   whichever arch built last owns the archive.
#
# ── -fno-stack-protector matters here ───────────────────────────────────
# The shell builds with -fno-stack-protector so it does not ADD references
# to ___stack_chk_fail / ___stack_chk_guard / ___chkstk_darwin.  The
# library carries the same flag, so those names are gone from both sides.
# If they reappear, they come from an object built before the flag
# landed — rebuild the library, do not add flags here.
#
# ── the x86_64 flag set MATCHES the library's, deliberately ─────────────
# uix_math.c cannot compile with -mno-sse (SSE register return with SSE
# disabled), so the library builds x86_64 WITHOUT -mno-sse/-mno-sse2.
# That means its doubles return in xmm0 per the SysV ABI.  If the shell
# passed a double with the kernel's flags, the two halves would disagree
# silently — link fine, garbage at runtime.  The flags below are copied
# from 00_uixlibs so they cannot drift.
# =============================================================================

MFDIR       := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_BASE  := $(MFDIR)build

_ARCH       ?= arm64
_BUILD_DIR  := $(BUILD_BASE)/$(_ARCH)

# ── toolchain — cross, mirroring 00_uixlibs and 32_FS ───────────────────
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

# ── arch flag sets — kept identical to 00_uixlibs ───────────────────────
ARM64_FLAGS   := -march=armv8-a -mabi=lp64 -mlittle-endian
ARM32_FLAGS   := -march=armv7-a -marm -mfloat-abi=soft -mlittle-endian
RV64_FLAGS    := -march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany
X86_FLAGS     := -m64 -mno-red-zone -mno-mmx -mcmodel=kernel

ARM64_CPUDEF  := -D__aarch64__
ARM32_CPUDEF  := -D__arm__
RV64_CPUDEF   := -D__riscv -D__riscv_xlen=64
X86_CPUDEF    := -D__x86_64__

# 01_cli → 01_shell → 50_UIX   (three hops, then UIOX is the parent of 50_UIX)
#   50_UIX/01_shell/01_cli/../../..  ==  UIOX/
UIXROOT     := $(MFDIR)../../..
UIXLIB      := $(UIXROOT)/50_UIX/00_libs/00_uixlibs

# ── the library archive, per arch ───────────────────────────────────────
UIXLIB_DIR  := $(UIXLIB)/build/$(_ARCH)
UIXLIB_AR   := $(UIXLIB_DIR)/libUIXlib.a

CFLAGS := -std=c99 -Wall -Wextra \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -nostdinc -Os -g \
          $(ARCH_CFLAGS) $(ARCH_CPUDEF) \
          -I$(UIXROOT)/40_SCIX \
          -I$(UIXLIB) \
          -I$(UIXLIB)/PoStd \
          -I$(UIXLIB)/sys \
          -DSYS_CALL_ENBLE_DISABLE=0

# ── the archive MUST come after the object that needs it ────────────────
# ld walks archives in command order.  With the library ahead of
# src/uiox_sh.c, the archive is scanned before uiox_sh.o exists, so
# nothing has yet referenced uix_write / uix_read / uix_open and EVERY
# symbol in it is skipped — which is what the first link reported: twelve
# undefined references, one per shell call site.
#
# So the recipe is $(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS) $(LIBS):
# sources first, then the archive by PATH, then libgcc last.
#
# The archive is named by path, not with -l, so `make -n` shows which
# per-arch archive a given arch is about to link.
LDFLAGS := -nostdlib -nostartfiles -ffreestanding -Wl,-e,main

# `=` deliberately: see the v2.3.0 note on recursive expansion.
LIBS    = $(UIXLIB_AR) $(shell $(CC) -print-libgcc-file-name)

TARGET := $(_BUILD_DIR)/sh

SRCS := src/uiox_sh.c

# ── arch targets ──────────────────────────────────────────────────────────
# The per-arch recipe re-invokes in the SUB-make, where _ARCH is already
# correct.  Handing _ARCH down from the parent does not work — the
# parent's _ARCH is still the default, because `arm32` is a target, not a
# variable.
#
# The toolchain check is a hard stop, not a fallback: if aarch64-elf-gcc
# is absent, `cc` would produce Mach-O and fail later with a confusing
# "not a mach-o file" instead of here with the real cause.
.PHONY: arm64
arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=arm64 CC="$(ARM64_CC)" \
	    ARCH_CFLAGS="$(ARM64_FLAGS)" ARCH_CPUDEF="$(ARM64_CPUDEF)"

.PHONY: arm32
arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=arm32 CC="$(ARM32_CC)" \
	    ARCH_CFLAGS="$(ARM32_FLAGS)" ARCH_CPUDEF="$(ARM32_CPUDEF)"

.PHONY: riscv64
riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=riscv64 CC="$(RV64_CC)" \
	    ARCH_CFLAGS="$(RV64_FLAGS)" ARCH_CPUDEF="$(RV64_CPUDEF)"

.PHONY: x86_64
x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build _ARCH=x86_64 CC="$(X86_CC)" \
	    ARCH_CFLAGS="$(X86_FLAGS)" ARCH_CPUDEF="$(X86_CPUDEF)"

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "[CLI] all four front-ends built"

.PHONY: _guard_archive
_guard_archive:
	@[ -f "$(UIXLIB_AR)" ] || { \
	  echo "[CLI] MISSING: $(UIXLIB_AR)"; \
	  echo "[CLI] build the library first:"; \
	  echo "[CLI]   make -C $(UIXLIB) $(_ARCH)"; \
	  exit 1; }

.PHONY: _build
_build: _guard_archive $(TARGET)
	@echo "[CLI]     $(_ARCH) → $(TARGET)"

$(TARGET): $(SRCS)
	@mkdir -p $(dir $@)
	@echo "  [CC]  $(_ARCH)  uiox_sh.c  +  $(notdir $(UIXLIB_AR))  +  libgcc"
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS) $(LIBS)

# ── what the archive supplies, and what it does not ──────────────────────
# nm on libUIXlib.a now leaves only COMPILER-RUNTIME names, and only on
# the arches whose code path reaches them:
#
#   arm64   __aarch64_ldadd4_acq_rel __aarch64_swp4_acq_rel
#   arm32   __aeabi_dmul __aeabi_dadd __aeabi_ddiv __aeabi_i2d
#
# Both sets are libgcc, now supplied by the link line.  check-unresolved
# reads the ARCHIVE alone, so it will keep reporting them even after the
# link succeeds — that is expected, not a regression.  Run it against the
# linked binary to see the difference:
#
#   make check-binary
#
# HOST RUNTIME names (___chkstk_darwin, _memcpy, _memset, _strlen, the
# stack-chk trio) are gone: the library carries -ffreestanding
# -fno-builtin -fno-stack-protector.  If any reappears, the archive is
# stale — rebuild it, do not add flags here.
.PHONY: check-unresolved
check-unresolved:
	@[ -f "$(UIXLIB_AR)" ] || { echo "[CLI] no archive at $(UIXLIB_AR)"; exit 1; }
	@nm "$(UIXLIB_AR)" 2>/dev/null | grep ' U ' \
	  | sed 's/.*U //' | sort -u > /tmp/sh_undef
	@nm "$(UIXLIB_AR)" 2>/dev/null \
	  | grep ' [TtWwDdBbRr] ' | sed 's/.*[TtWwDdBbRr] //' | sort -u > /tmp/sh_def
	@echo "unresolved in $(notdir $(UIXLIB_AR)) [$(_ARCH)]:"
	@comm -23 /tmp/sh_undef /tmp/sh_def | sed 's/^/  /'
	@echo "  ($$(comm -23 /tmp/sh_undef /tmp/sh_def | wc -l | tr -d ' ') total)"

# ── check-binary — is the LINKED shell self-contained? ──────────────────
# The question that matters.  An archive may reference libgcc helpers it
# never needs; a finished binary may not reference anything at all.
# Zero here means the image stands alone.
.PHONY: check-binary
check-binary:
	@[ -f "$(TARGET)" ] || { echo "[CLI] no binary at $(TARGET)"; exit 1; }
	@echo "undefined in $(notdir $(TARGET)) [$(_ARCH)]:"
	@nm -u "$(TARGET)" 2>/dev/null | sed 's/^ *U //' | sort -u | sed 's/^/  /' \
	  || echo "  (none — nm reports no undefined symbols)"

# ── the leak test ─────────────────────────────────────────────────────────
# A front-end may not name a kernel symbol and may not reach the trap
# layer directly.  Zero hits is correct; any hit means the boundary leaked.
.PHONY: check-layering
check-layering:
	@grep -rn 'uiox_kix_scfs_\|uiox_console_\|uiox_uart\|my_syscall' src/ && \
	  { echo "[CLI] LAYERING LEAK: the front-end named below-the-boundary"; \
	    exit 1; } || \
	  echo "[CLI] layering ok: wrapper layer only, no kernel symbols"

# ── info ──────────────────────────────────────────────────────────────────
.PHONY: info
info:
	@echo "HOST=$(UNAME_S)"
	@echo "  UIXROOT   = $(UIXROOT)"
	@echo "  UIXLIB_AR = $(UIXLIB_AR)"
	@echo "  ARM64_CC  = $(ARM64_CC)"
	@echo "  ARM32_CC  = $(ARM32_CC)"
	@echo "  RV64_CC   = $(RV64_CC)"
	@echo "  X86_CC    = $(X86_CC)"

# libgcc is asked per arch, so it needs a live CC — report each.
.PHONY: info-gcc
info-gcc:
	@echo "arm64   libgcc: $$($(ARM64_CC) -print-libgcc-file-name)"
	@echo "arm32   libgcc: $$($(ARM32_CC) -print-libgcc-file-name)"
	@echo "riscv64 libgcc: $$($(RV64_CC) -print-libgcc-file-name)"
	@echo "x86_64  libgcc: $$($(X86_CC) -print-libgcc-file-name)"

.PHONY: clean clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
clean-arm64:   ; @rm -rf $(BUILD_BASE)/arm64
clean-arm32:   ; @rm -rf $(BUILD_BASE)/arm32
clean-riscv64: ; @rm -rf $(BUILD_BASE)/riscv64
clean-x86_64:  ; @rm -rf $(BUILD_BASE)/x86_64
clean:
	@rm -rf $(BUILD_BASE)
	@echo "[CLN] 01_cli build outputs removed"

# ── no install target ─────────────────────────────────────────────────────
# 20_uios stages to $(IMAGE_ROOT)/bin/uios because kinit_main execs that
# path.  Nothing execs a shell yet — there is no filesystem to install
# into and no $PATH search to find it.
