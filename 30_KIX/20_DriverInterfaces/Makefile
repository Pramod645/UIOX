#uiox/
#└── uiox_hw/
#    ├── include/
#    │   ├── hw_types.h
#    │   ├── irq.h
#    │   ├── mmio.h
#    │   └── cpu.h
#    ├── src/
#    │   ├── irq.c
#    │   ├── mmio.c
#    │   └── cpu.c
#    ├── main.c
#    └── Makefile

# =============================================================
# uiox_hw — hardware control layer
#
# Targets ARM64, ARM32, and x86_64.
# Default build uses the host compiler (native arch).
#
# Cross-compile examples:
#   make ARCH=arm64  CC=aarch64-linux-gnu-gcc
#   make ARCH=arm32  CC=arm-linux-gnueabihf-gcc
#   make ARCH=x86_64 CC=gcc
# =============================================================

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11 -Iinclude
AR      := ar
ARFLAGS := rcs

# ── Architecture selection ───────────────────────────────────
ARCH    ?= native

ifeq ($(ARCH), arm64)
  CC      := aarch64-linux-gnu-gcc
  CFLAGS  += -march=armv8-a -DARCH_ARM64
  TRIPLE  := arm64
else ifeq ($(ARCH), arm32)
  CC      := arm-linux-gnueabihf-gcc
  CFLAGS  += -march=armv7-a -mfpu=vfpv3 -DARCH_ARM32
  TRIPLE  := arm32
else ifeq ($(ARCH), x86_64)
  CC      := gcc
  CFLAGS  += -march=x86-64 -DARCH_X86_64
  TRIPLE  := x86_64
else
  # native: let the compiler detect the arch
  TRIPLE  := native
endif

# ── Directories ──────────────────────────────────────────────
SRC_DIR := src
OBJ_DIR := obj/$(TRIPLE)
LIB_DIR := lib/$(TRIPLE)
TARGET  := uiox_hw_$(TRIPLE)

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

STATIC  := $(LIB_DIR)/libuiox_hw.a
SHARED  := $(LIB_DIR)/libuiox_hw.so

# ── Phony targets ────────────────────────────────────────────
.PHONY: all static shared run clean help

all: $(TARGET) static shared

# ── Executable ───────────────────────────────────────────────
$(TARGET): $(OBJS) main.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC $^ -o $@
	@echo "Built executable: $@"

# ── Static library ───────────────────────────────────────────
static: $(STATIC)

$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built static lib: $@"

# ── Shared library ───────────────────────────────────────────
shared: $(SHARED)

$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC -o $@ $^
	@echo "Built shared lib: $@"

# ── Object files ─────────────────────────────────────────────
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# ── Directory creation ───────────────────────────────────────
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# ── Run (native only) ────────────────────────────────────────
run: $(TARGET)
	./$(TARGET)

# ── Build all three architectures ───────────────────────────
all-archs:
	$(MAKE) ARCH=x86_64
	$(MAKE) ARCH=arm64
	$(MAKE) ARCH=arm32

# ── Clean ────────────────────────────────────────────────────
clean:
	rm -rf obj lib uiox_hw_*

# ── Help ─────────────────────────────────────────────────────
help:
	@echo "Usage:"
	@echo "  make                  — native build"
	@echo "  make ARCH=arm64       — cross-compile for AArch64"
	@echo "  make ARCH=arm32       — cross-compile for ARMv7"
	@echo "  make ARCH=x86_64      — explicit x86_64 build"
	@echo "  make all-archs        — build all three targets"
	@echo "  make run              — run native binary"
	@echo "  make clean            — remove all build artefacts"
