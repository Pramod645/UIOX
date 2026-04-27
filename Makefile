#uiox/
#└── uiox_target/
#    ├── arch/
#    │   ├── arm64/
#    │   │   ├── include/
#    │   │   │   └── arch_defs.h
#    │   │   └── src/
#    │   │       └── arch_init.c
#    │   ├── arm32/
#    │   │   ├── include/
#    │   │   │   └── arch_defs.h
#    │   │   └── src/
#    │   │       └── arch_init.c
#    │   └── x86_64/
#    │       ├── include/
#    │       │   └── arch_defs.h
#    │       └── src/
#    │           └── arch_init.c
#    ├── build_config/
#    │   ├── common.mk
#    │   ├── arm64.mk
#    │   ├── arm32.mk
#    │   └── x86_64.mk
#    ├── main.c
#    └── Makefile

# =============================================================
# uiox_target/Makefile
#
# Top-level Makefile.  Delegates to per-architecture configs
# in build_config/.
#
# Usage:
#   make                  — build all three architectures
#   make ARCH=arm64       — build ARM64 only
#   make ARCH=arm32       — build ARM32 only
#   make ARCH=x86_64      — build x86_64 only
#   make run              — run native x86_64 binary
#   make qemu_arm64       — run ARM64 image in QEMU
#   make qemu_arm32       — run ARM32 image in QEMU
#   make qemu_x86_64      — run x86_64 image in QEMU
#   make clean            — remove all build artefacts
# =============================================================

# Locate the UIOX root (parent of uiox_target/)
UIOX_ROOT := $(abspath ..)
export UIOX_ROOT

ARCH ?= all

.PHONY: all arm64 arm32 x86_64 run \
        qemu_arm64 qemu_arm32 qemu_x86_64 \
        clean help

# ── Default: build all three ─────────────────────────────────
all:
	$(MAKE) -f build_config/arm64.mk   UIOX_ROOT=$(UIOX_ROOT) arm64
	$(MAKE) -f build_config/arm32.mk   UIOX_ROOT=$(UIOX_ROOT) arm32
	$(MAKE) -f build_config/x86_64.mk  UIOX_ROOT=$(UIOX_ROOT) x86_64

# ── Individual architecture targets ──────────────────────────
arm64:
	$(MAKE) -f build_config/arm64.mk  UIOX_ROOT=$(UIOX_ROOT) arm64

arm32:
	$(MAKE) -f build_config/arm32.mk  UIOX_ROOT=$(UIOX_ROOT) arm32

x86_64:
	$(MAKE) -f build_config/x86_64.mk UIOX_ROOT=$(UIOX_ROOT) x86_64

# ── Run native x86_64 binary ─────────────────────────────────
run: x86_64
	./uiox_x86_64

# ── QEMU run targets ──────────────────────────────────────────
qemu_arm64: arm64
	$(MAKE) -f build_config/arm64.mk  UIOX_ROOT=$(UIOX_ROOT) arm64_run

qemu_arm32: arm32
	$(MAKE) -f build_config/arm32.mk  UIOX_ROOT=$(UIOX_ROOT) arm32_run

qemu_x86_64: x86_64
	$(MAKE) -f build_config/x86_64.mk UIOX_ROOT=$(UIOX_ROOT) x86_64_qemu

# ── Clean all architectures ───────────────────────────────────
clean:
	$(MAKE) -f build_config/arm64.mk  arm64_clean
	$(MAKE) -f build_config/arm32.mk  arm32_clean
	$(MAKE) -f build_config/x86_64.mk x86_64_clean
	rm -rf obj lib

# ── Help ─────────────────────────────────────────────────────
help:
	@echo "uiox_target build system"
	@echo ""
	@echo "  make              build ARM64 + ARM32 + x86_64"
	@echo "  make ARCH=arm64   build ARM64 only"
	@echo "  make ARCH=arm32   build ARM32 only"
	@echo "  make ARCH=x86_64  build x86_64 only"
	@echo "  make run          run native x86_64 binary"
	@echo "  make qemu_arm64   boot ARM64 in QEMU"
	@echo "  make qemu_arm32   boot ARM32 in QEMU"
	@echo "  make qemu_x86_64  boot x86_64 in QEMU"
	@echo "  make clean        remove all build artefacts"
	@echo ""
	@echo "Cross-compilers required:"
	@echo "  ARM64:  aarch64-linux-gnu-gcc"
	@echo "  ARM32:  arm-linux-gnueabihf-gcc"
	@echo "  x86_64: gcc (native)"
