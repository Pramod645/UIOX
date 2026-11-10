# =============================================================================
# 50_UIX/00_libs/00_uixlibs/Makefile  — v2.5.0
#
# THE COMMON LIBRARY MAKEFILE.
# Builds the UIX userland library for all four architectures, as either:
#     build/<arch>/libUIXlib.a      static archive (always)
#     build/<arch>/libUIXlib.so     shared object (BUILD_SO=1)
#
# Usage:
#   make arm64 / arm32 / riscv64 / x86_64
#   make all                              ← static only, all four
#   make all BUILD_SO=1                   ← static + .so, all four
#   make so-arm64 / so        ← .so targets directly
#   make clean[-<arch>]
#   make info[-<arch>]
#   make srclist                          ← what the globs resolved to
#   make syms                             ← duplicate definitions
#   make verify[-<arch>]                  ← ELF class + unresolved report
#   make clean && make all
#   make verify
#   make so-arm64 so-arm32 so-riscv64 so-x86_64
#
# ── READ THIS BEFORE USING THE .so ───────────────────────────────────
#   The shared objects this builds are REAL: correct ELF class, correct
#   machine, correctly relocatable for their architecture.  They are
#   also NOT LOADABLE ON ANY UIOX TARGET, because UIOX has no dynamic
#   loader — no PT_INTERP, no ld.so, no runtime symbol resolution.  A
#   .so is only useful to you for two things:
#     · inspecting relocations with `readelf -r`, to see what the layer
#       would need resolved if a loader ever exists
#     · handing the layer to a HOSTED build (a Linux/macOS userland
#       that has a real ld.so) for testing outside UIOX
#   Linking one of these into a UIOX image will produce a binary the
#   kernel cannot start.  The static archive remains the deliverable.
#
# ── v2.5.0 changes ───────────────────────────────────────────────────
#   · LD added per arch; the .so link uses it explicitly rather than
#     relying on $(CC) to pick the right linker driver.
#   · BUILD_SO switch; per-arch .so in build/<arch>/, not build/host/.
#   · so-<arch> targets, and all/so fan out over the four arches.
#   · Object set shared between .a and .so: the .so is built from the
#     SAME -fPIC objects, so the two cannot drift.  This matters — a
#     .so built from different objects than the .a is a second source
#     of truth about what the layer contains.
#   · verify-<arch> target: readelf class/machine + unresolved count.
#
# ── v2.4.1 correction (kept for the record) ──────────────────────────
#   v2.4.0 assumed each layer had a src/ child, copied from the 32_FS
#   layout.  It does not.  00_uixlibs is FLAT: the .c files sit directly
#   in PoStd/, arpa/, net/, netinet/, sys/, and nothing lives at the root.
#   Every wildcard was therefore empty and the $(error) guard at the top
#   fired on the first `make srclist` — which is what the guard is for.
#
# ── the layers, all children of this directory ────────────────────────
#   PoStd/*.c       POSIX-shaped std wrappers, freestanding body
#   arpa/*.c        inet resolution
#   net/*.c         socket-level helpers
#   netinet/*.c     protocol helpers
#   sys/*.c         the syscall stubs: uixopen/uixread/uixwrite/...
#
# ── why CC / AR / LD / RANLIB are per-arch ───────────────────────────
#   The old ad-hoc Makefile archived every architecture with
#   aarch64-elf-ar.  An archive of arm32 objects stamped by
#   aarch64-elf-ar is not a valid arm32 archive and the linker rejects
#   it — which is the failure that consumed a whole session in the
#   transcript.  RANLIB is invoked too: `ar rcs` does not guarantee the
#   symbol index, and a cross ar and the host's ar differ on that.
#
# ── the include graph ────────────────────────────────────────────────
#   every layer  →  its own directory  +  40_SCIX
#   sys/         →  the same 40_SCIX, so the SYSOPEN/SYSSEND/SYSRECV
#                    numbers cannot drift between the stubs here and the
#                    router in 33_PCS.
#
#   SCIX_DIR is the one upward include.  It is a variable, not a
#   relative climb, so its depth cannot bite you again.
# =============================================================================

# ── Roots ─────────────────────────────────────────────────────────────
MFDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

SCIX_DIR ?= $(abspath $(MFDIR)../../../40_SCIX)

# ── the flat layers ───────────────────────────────────────────────────
LIB_DIRS := \
  $(MFDIR)PoStd    \
  $(MFDIR)arpa     \
  $(MFDIR)net      \
  $(MFDIR)netinet  \
  $(MFDIR)sys

LIB_NAME := UIXlib

# ── Host / toolchain (identical to 32_FS, plus LD) ───────────────────
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC := aarch64-elf-gcc
  ARM64_LD := aarch64-elf-ld
  ARM64_AR := aarch64-elf-ar
  ARM64_RL := aarch64-elf-ranlib
  ARM32_CC := arm-none-eabi-gcc
  ARM32_LD := arm-none-eabi-ld
  ARM32_AR := arm-none-eabi-ar
  ARM32_RL := arm-none-eabi-ranlib
  RV64_CC  := riscv64-elf-gcc
  RV64_LD  := riscv64-elf-ld
  RV64_AR  := riscv64-elf-ar
  RV64_RL  := riscv64-elf-ranlib
  X86_CC   := x86_64-elf-gcc
  X86_LD   := x86_64-elf-ld
  X86_AR   := x86_64-elf-ar
  X86_RL   := x86_64-elf-ranlib
else
  ARM64_CC := aarch64-linux-gnu-gcc
  ARM64_LD := aarch64-linux-gnu-ld
  ARM64_AR := aarch64-linux-gnu-ar
  ARM64_RL := aarch64-linux-gnu-ranlib
  ARM32_CC := arm-linux-gnueabihf-gcc
  ARM32_LD := arm-linux-gnueabihf-ld
  ARM32_AR := arm-linux-gnueabihf-ar
  ARM32_RL := arm-linux-gnueabihf-ranlib
  RV64_CC  := riscv64-linux-gnu-gcc
  RV64_LD  := riscv64-linux-gnu-ld
  RV64_AR  := riscv64-linux-gnu-ar
  RV64_RL  := riscv64-linux-gnu-ranlib
  X86_CC   := x86_64-linux-gnu-gcc
  X86_LD   := x86_64-linux-gnu-ld
  X86_AR   := x86_64-linux-gnu-ar
  X86_RL   := x86_64-linux-gnu-ranlib
endif

ARFLAGS := rcs

# ── Common flags ─────────────────────────────────────────────────────
# -ffreestanding, NOT -nostdinc.  stdarg.h / stddef.h / stdbool.h are
# supplied by GCC ITSELF, not by a C library, and C11 requires them to
# be available in a freestanding implementation.  -nostdinc removes
# GCC's own include directory along with libc's, which killed
# uix_semaphore.c:4 — and would have killed every va_list in the layer.
COMMON_CFLAGS := \
  -std=c11                \
  -Wall -Wextra           \
  -Wno-unused-parameter   \
  -Wno-missing-prototypes \
  -Wno-strict-prototypes  \
  -ffreestanding          \
  -fno-builtin            \
  -fno-stack-protector    \
  -fno-common             \
  -ffunction-sections     \
  -fdata-sections         \
  -O2 -g

ifneq ($(WERROR),)
  COMMON_CFLAGS += -Werror
endif

COMMON_INCLUDES := \
  $(addprefix -I,$(LIB_DIRS)) \
  -I$(MFDIR)                  \
  -I$(SCIX_DIR)

DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

# ── Sources — flat layers, one glob each ─────────────────────────────
LIB_SRCS_C :=
LIB_SRCS_C += $(wildcard $(MFDIR)PoStd/*.c)
LIB_SRCS_C += $(wildcard $(MFDIR)arpa/*.c)
LIB_SRCS_C += $(wildcard $(MFDIR)net/*.c)
LIB_SRCS_C += $(wildcard $(MFDIR)netinet/*.c)
LIB_SRCS_C += $(wildcard $(MFDIR)sys/*.c)

ifeq ($(strip $(LIB_SRCS_C)),)
  $(error No .c files found under $(LIB_DIRS))
endif

# Report a layer that resolved to nothing, without stopping the build.
define check_layer
$(if $(strip $(wildcard $(1)/*.c)),,$(warning LIX: no .c files in $(1)))
endef
$(foreach d,$(LIB_DIRS),$(eval $(call check_layer,$(d))))

vpath %.c $(LIB_DIRS)

# ── Build control ─────────────────────────────────────────────────────
# BUILD_STATIC: the archive.  1 by default; the .so link reuses the
#               same -fPIC objects, so this also gates object compiles.
# BUILD_SO:     the per-arch shared object.  Off by default; see the
#               warning at the top of this file before turning it on.
BUILD_STATIC ?= 1
BUILD_SO     ?= 0

# ── arch entry points ─────────────────────────────────────────────────
# ARCH_CCOPT carries the per-arch policy flag.  Only x86_64 differs, and
# it differs for the same reason it does in 32_FS: the kernel is built
# -fno-pic -mcmodel=kernel, and an overlay compiled -fPIC emits
# GOT-relative references the kernel image cannot resolve.
#
# NOTE on BUILD_SO + x86_64: a shared object requires PIC, and this arch
# is explicitly -fno-pic.  `make so-x86_64` will therefore fail at the
# link, by design, and the guard in _build_so says so rather than
# producing something that looks like a .so and is not one.
.PHONY: arm64
arm64: _do_arm64
	@echo "[LIX] arm64 done → build/arm64/"
.PHONY: _do_arm64
_do_arm64:
	@command -v $(ARM64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                    \
	    _ARCH=arm64 CC=$(ARM64_CC) LD=$(ARM64_LD) AR=$(ARM64_AR) RL=$(ARM64_RL) \
	    ARCH_CFLAGS="-march=armv8-a -mabi=lp64 -mlittle-endian"  \
	    ARCH_CCOPT="-fPIC"                                        \
	    ARCH_CPUDEF="-D__aarch64__"

.PHONY: arm32
arm32: _do_arm32
	@echo "[LIX] arm32 done → build/arm32/"
.PHONY: _do_arm32
_do_arm32:
	@command -v $(ARM32_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(ARM32_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                          \
	    _ARCH=arm32 CC=$(ARM32_CC) LD=$(ARM32_LD) AR=$(ARM32_AR) RL=$(ARM32_RL) \
	    ARCH_CFLAGS="-march=armv7-a -marm -mfloat-abi=soft -mlittle-endian" \
	    ARCH_CCOPT="-fPIC"                                               \
	    ARCH_CPUDEF="-D__arm__"

.PHONY: riscv64
riscv64: _do_riscv64
	@echo "[LIX] riscv64 done → build/riscv64/"
.PHONY: _do_riscv64
_do_riscv64:
	@command -v $(RV64_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(RV64_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                         \
	    _ARCH=riscv64 CC=$(RV64_CC) LD=$(RV64_LD) AR=$(RV64_AR) RL=$(RV64_RL) \
	    ARCH_CFLAGS="-march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany" \
	    ARCH_CCOPT="-fPIC"                                                     \
	    ARCH_CPUDEF="-D__riscv -D__riscv_xlen=64"

.PHONY: x86_64
x86_64: _do_x86_64
	@echo "[LIX] x86_64 done → build/x86_64/"
.PHONY: _do_x86_64
_do_x86_64:
	@command -v $(X86_CC) >/dev/null 2>&1 || \
	    { echo "ERROR: $(X86_CC) not found"; exit 1; }
	$(MAKE) -f $(MFDIR)Makefile _build                            \
	    _ARCH=x86_64 CC=$(X86_CC) LD=$(X86_LD) AR=$(X86_AR) RL=$(X86_RL) \
	    ARCH_CFLAGS="-m64 -mno-red-zone -mno-mmx -mcmodel=kernel" \
	    ARCH_CCOPT="-fno-pic"                                     \
	    ARCH_CPUDEF="-D__x86_64__"

_BUILD_DIR = $(MFDIR)build/$(_ARCH)

_CFLAGS_ALL = $(COMMON_CFLAGS) \
              $(COMMON_INCLUDES) \
              $(ARCH_CFLAGS)     \
              $(ARCH_CCOPT)      \
              $(ARCH_CPUDEF)

# $(MFDIR) ends in a slash, so $(MFDIR)%.c strips to a clean relative
# path and objects keep their layer directory in their name:
#   build/arm64/PoStd/uix_stdio.o
_OBJS = $(patsubst $(MFDIR)%.c,$(_BUILD_DIR)/%.o,$(LIB_SRCS_C))
-include $(_OBJS:.o=.d)

# ── the real build ────────────────────────────────────────────────────
.PHONY: _build
_build:
	@$(MAKE) -f $(MFDIR)Makefile _build_static \
	    _ARCH=$(_ARCH) CC=$(CC) LD=$(LD) AR=$(AR) RL=$(RL) \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CCOPT="$(ARCH_CCOPT)" \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)" BUILD_STATIC=$(BUILD_STATIC) BUILD_SO=$(BUILD_SO)
ifneq ($(BUILD_SO),0)
	@$(MAKE) -f $(MFDIR)Makefile _build_so \
	    _ARCH=$(_ARCH) CC=$(CC) LD=$(LD) AR=$(AR) RL=$(RL) \
	    ARCH_CFLAGS="$(ARCH_CFLAGS)" ARCH_CCOPT="$(ARCH_CCOPT)" \
	    ARCH_CPUDEF="$(ARCH_CPUDEF)" BUILD_STATIC=$(BUILD_STATIC) BUILD_SO=$(BUILD_SO)
endif
	@echo "[LIX] _build done: $(_ARCH)"

# ── static archive ────────────────────────────────────────────────────
.PHONY: _build_static
_build_static: $(_BUILD_DIR)/lib$(LIB_NAME).a
	@echo "[LIX] static done: $(_ARCH)"

$(_BUILD_DIR)/lib$(LIB_NAME).a: $(_OBJS)
	@mkdir -p $(dir $@)
	rm -f $@
	$(AR) $(ARFLAGS) $@ $^
	$(RL) $@
	@echo "  [AR]  $@  ($$(ls -l $@ | awk '{print $$5}') bytes, $(words $^) objects)"

# ── shared object ─────────────────────────────────────────────────────
# Linked with $(LD) -shared, not $(CC) -shared, so the linker driver
# cannot silently substitute a different one.  -nostdlib and no crt
# objects: this layer has no libc to link against, and pulling in a
# host crt0 would make the .so look loadable when it is not.
.PHONY: _build_so
_build_so: $(_BUILD_DIR)/lib$(LIB_NAME).so
	@echo "[LIX] shared done: $(_ARCH)"

$(_BUILD_DIR)/lib$(LIB_NAME).so: $(_OBJS)
ifneq ($(_ARCH),x86_64)
	@mkdir -p $(dir $@)
	rm -f $@
	$(LD) -shared -nostdlib -no-undefined \
	    -soname lib$(LIB_NAME).so \
	    $(ARCH_CFLAGS) $(ARCH_CCOPT) \
	    -o $@ $^
	@echo "  [SO]  $@  ($$(ls -l $@ | awk '{print $$5}') bytes) — NOT loadable on UIOX"
else
	@echo "  [SO]  x86_64 SKIPPED: this arch is -fno-pic -mcmodel=kernel."
	@echo "        A shared object requires PIC; there is nothing correct to emit."
	@echo "        Build the .a, or use one of the other three arches for the .so."
endif

# ── object rule (shared by .a and .so) ───────────────────────────────
$(_BUILD_DIR)/%.o: $(MFDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(_CFLAGS_ALL) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $(_ARCH)  $<"

# ── convenience: so-<arch> and all ───────────────────────────────────
.PHONY: so-arm64 so-arm32 so-riscv64 so-x86_64 so
so-arm64:   ; @$(MAKE) -f $(MFDIR)Makefile arm64   BUILD_SO=1
so-arm32:   ; @$(MAKE) -f $(MFDIR)Makefile arm32   BUILD_SO=1
so-riscv64: ; @$(MAKE) -f $(MFDIR)Makefile riscv64 BUILD_SO=1
so-x86_64:  ; @$(MAKE) -f $(MFDIR)Makefile x86_64  BUILD_SO=1

so: so-arm64 so-arm32 so-riscv64 so-x86_64
	@echo "[LIX] shared build pass complete (x86_64 skipped by design)"

.PHONY: all
all: arm64 arm32 riscv64 x86_64
	@echo "" && echo "[LIX] all builds complete"

# ── verify — what each artifact actually is ──────────────────────────
# readelf on the cross tools is often not on PATH as `readelf`; use the
# host one, which reads any ELF.  This exists because "the archive
# built" and "the archive is what I think it is" are different claims,
# and your transcript has a case where the second one was false.
.PHONY: verify-arm64 verify-arm32 verify-riscv64 verify-x86_64 verify
verify-arm64:   ; @$(MAKE) -f $(MFDIR)Makefile _verify _ARCH=arm64
verify-arm32:   ; @$(MAKE) -f $(MFDIR)Makefile _verify _ARCH=arm32
verify-riscv64: ; @$(MAKE) -f $(MFDIR)Makefile _verify _ARCH=riscv64
verify-x86_64:  ; @$(MAKE) -f $(MFDIR)Makefile _verify _ARCH=x86_64
verify: verify-arm64 verify-arm32 verify-riscv64 verify-x86_64

.PHONY: _verify
_verify:
	@d=$(_BUILD_DIR); \
	if [ -f $$d/lib$(LIB_NAME).a ]; then \
	  echo "[LIX] $(_ARCH) archive:"; \
	  echo "  members:   $$(ar t $$d/lib$(LIB_NAME).a | wc -l | tr -d ' ')"; \
	  echo "  undefined: $$(nm -u $$d/lib$(LIB_NAME).a 2>/dev/null | grep -c ' U ')"; \
	  echo "  defined:   $$(nm -g --defined-only $$d/lib$(LIB_NAME).a 2>/dev/null | grep -c ' [A-TV-Z] ')"; \
	else echo "[LIX] $(_ARCH): no archive"; fi; \
	if [ -f $$d/lib$(LIB_NAME).so ]; then \
	  echo "[LIX] $(_ARCH) shared:"; \
	  readelf -h $$d/lib$(LIB_NAME).so | sed -n 's/^  \(Class\|Machine\|Type\):/\1:/p' | sed 's/^/  /'; \
	  echo "  relocs:    $$(readelf -r $$d/lib$(LIB_NAME).so 2>/dev/null | grep -c '^0')"; \
	fi

# ── clean ─────────────────────────────────────────────────────────────
.PHONY: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64 clean
clean-arm64:   ; @rm -rf $(MFDIR)build/arm64
clean-arm32:   ; @rm -rf $(MFDIR)build/arm32
clean-riscv64: ; @rm -rf $(MFDIR)build/riscv64
clean-x86_64:  ; @rm -rf $(MFDIR)build/x86_64
clean: clean-arm64 clean-arm32 clean-riscv64 clean-x86_64
	@echo "[CLN] all build outputs removed"

# ── info ──────────────────────────────────────────────────────────────
.PHONY: info-arm64 info-arm32 info-riscv64 info-x86_64 info
info-arm64:
	@echo "HOST=$(UNAME_S) ARCH=arm64  CC=$(ARM64_CC) LD=$(ARM64_LD) AR=$(ARM64_AR) RL=$(ARM64_RL)"
	@echo "  MFDIR    = $(MFDIR)"
	@echo "  SCIX_DIR = $(SCIX_DIR)"
	@echo "  BUILD_STATIC=$(BUILD_STATIC)  BUILD_SO=$(BUILD_SO)"
info-arm32:   ; @echo "HOST=$(UNAME_S) ARCH=arm32   CC=$(ARM32_CC) LD=$(ARM32_LD) AR=$(ARM32_AR) RL=$(ARM32_RL)"
info-riscv64: ; @echo "HOST=$(UNAME_S) ARCH=riscv64 CC=$(RV64_CC) LD=$(RV64_LD) AR=$(RV64_AR) RL=$(RV64_RL)"
info-x86_64:  ; @echo "HOST=$(UNAME_S) ARCH=x86_64  CC=$(X86_CC) LD=$(X86_LD) AR=$(X86_AR) RL=$(X86_RL)"
info: info-arm64 info-arm32 info-riscv64 info-x86_64

# ── srclist — every file the globs resolved to ────────────────────────
.PHONY: srclist
srclist:
	@echo "[LIX] sources to build:"
	@for f in $(LIB_SRCS_C); do echo "  $$f"; done
	@echo "[LIX] total: $(words $(LIB_SRCS_C))"

# ── syms — duplicate definitions ─────────────────────────────────────
# The twelve unresolved uixopen/uixread/uixwrite symbols were never
# unresolved; they were defined more than once.  This prints every
# global defined in more than one object, so the next collision is
# visible before the link fails instead of after.
.PHONY: syms
syms:
	@for a in arm64 arm32 riscv64 x86_64; do \
	  f=$(MFDIR)build/$$a/lib$(LIB_NAME).a; \
	  [ -f $$f ] || continue; \
	  echo "[LIX] $$a:"; \
	  nm -g --defined-only $$f 2>/dev/null \
	    | awk '/^[0-9a-f]+ [A-TV-Z] /{print $$3}' \
	    | sort | uniq -d | sed 's/^/  DUP /'; \
	  echo "  total globals: $$(nm -g --defined-only $$f 2>/dev/null | grep -c ' [A-TV-Z] ')"; \
	done
