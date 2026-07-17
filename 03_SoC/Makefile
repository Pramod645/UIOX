# =============================================================================
#  UIOX -- 03_SoC/Makefile
#  SoC Platform Layer  (silicon bring-up, PSCI, secure-boot, POST, memory)
#  Source : https://github.com/Pramod645/UIOX/tree/main/03_SoC
#
#  Targets : arm64 | arm32 | x86_64 | riscv64 | all | clean
#
#  Generated source files in $(SRCDIR)/   (6 common + 4 arch-specific):
#  -- Common (compiled into every arch target) --
#   $(SRCDIR)/uiox_fw_mem.c
#   $(SRCDIR)/uiox_fw_post.c
#   $(SRCDIR)/uiox_fw_power.c
#   $(SRCDIR)/uiox_fw_psci.c
#   $(SRCDIR)/uiox_fw_secboot.c
#   $(SRCDIR)/uiox_soc_main.c
#  -- Arch-specific (one per target) --
#   $(SRCDIR)/uiox_soc_arm64.c     (arm64)
#   $(SRCDIR)/uiox_soc_arm32.c     (arm32)
#   $(SRCDIR)/uiox_soc_x86.c       (x86_64)
#   $(SRCDIR)/uiox_soc_riscv64.c   (riscv64)
#
#  Header files in $(INCDIR)/            (16 .h files):
#   $(INCDIR)/uiox_fw.h
#   $(INCDIR)/uiox_fw_clock.h
#   $(INCDIR)/uiox_fw_dma.h
#   $(INCDIR)/uiox_fw_irq.h
#   $(INCDIR)/uiox_fw_mem.h
#   $(INCDIR)/uiox_fw_post.h
#   $(INCDIR)/uiox_fw_power.h
#   $(INCDIR)/uiox_fw_psci.h
#   $(INCDIR)/uiox_fw_secboot.h
#   $(INCDIR)/uiox_fw_types.h
#   $(INCDIR)/uiox_fw_tz.h
#   $(INCDIR)/uiox_soc.h
#   $(INCDIR)/uiox_soc_clk.h
#   $(INCDIR)/uiox_soc_map.h
#   $(INCDIR)/uiox_soc_pm.h
#   $(INCDIR)/uiox_soc_types.h
#
#  Each target produces a static library:
#    build/arm64/libuiox_soc_arm64.a
#    build/arm32/libuiox_soc_arm32.a
#    build/x86_64/libuiox_soc_x86_64.a
#    build/riscv64/libuiox_soc_riscv64.a
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC      $<"
  LOGAR    := @echo "  AR      $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q        :=
  LOGCC    :=
  LOGAR    :=
  LOGCLEAN :=
endif

SRCDIR   := src
INCDIR   := include
BUILDDIR := build

# =============================================================================
# Cross-toolchain selection
# =============================================================================
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC  := aarch64-elf-gcc
  ARM32_CC  := arm-none-eabi-gcc
  X86_CC    := x86_64-elf-gcc
  RV64_CC   := riscv64-unknown-elf-gcc
else
  ARM64_CC  := aarch64-linux-gnu-gcc
  ARM32_CC  := arm-linux-gnueabihf-gcc
  X86_CC    := gcc
  RV64_CC   := riscv64-linux-gnu-gcc
endif

# =============================================================================
# Common flags
# =============================================================================
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

CFLAGS_COMMON := \
  -std=c11 -Wall -Wextra -Wpedantic \
  -ffreestanding -fno-builtin \
  -fno-stack-protector \
  -ffunction-sections -fdata-sections \
  -Os -g \
  -I$(INCDIR) \
  -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
  -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
  -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

# =============================================================================
# SOC_COMMON_SRCS -- 6 files compiled into every arch target
# =============================================================================
SOC_COMMON_SRCS := \
  $(SRCDIR)/uiox_fw_mem.c \
  $(SRCDIR)/uiox_fw_post.c \
  $(SRCDIR)/uiox_fw_power.c \
  $(SRCDIR)/uiox_fw_psci.c \
  $(SRCDIR)/uiox_fw_secboot.c \
  $(SRCDIR)/uiox_soc_main.c

# =============================================================================
# ARM64
# =============================================================================
ARM64_CFLAGS  := $(CFLAGS_COMMON) -march=armv8-a -mstrict-align
ARM64_SRCS    := $(SOC_COMMON_SRCS) $(SRCDIR)/uiox_soc_arm64.c
ARM64_OBJS    := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm64/%.o,$(ARM64_SRCS))
ARM64_LIB     := $(BUILDDIR)/arm64/libuiox_soc_arm64.a

.PHONY: arm64
arm64: $(ARM64_LIB)
	$(Q)echo "  OK  SoC ARM64 => $(ARM64_LIB)"

$(ARM64_LIB): $(ARM64_OBJS) | $(BUILDDIR)/arm64
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/arm64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm64
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm64:
	$(Q)mkdir -p $@

# =============================================================================
# ARM32
# =============================================================================
ARM32_CFLAGS  := $(CFLAGS_COMMON) -march=armv7-a -mthumb -mfloat-abi=soft
ARM32_SRCS    := $(SOC_COMMON_SRCS) $(SRCDIR)/uiox_soc_arm32.c
ARM32_OBJS    := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/arm32/%.o,$(ARM32_SRCS))
ARM32_LIB     := $(BUILDDIR)/arm32/libuiox_soc_arm32.a

.PHONY: arm32
arm32: $(ARM32_LIB)
	$(Q)echo "  OK  SoC ARM32 => $(ARM32_LIB)"

$(ARM32_LIB): $(ARM32_OBJS) | $(BUILDDIR)/arm32
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/arm32/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/arm32
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/arm32:
	$(Q)mkdir -p $@

# =============================================================================
# x86_64
# =============================================================================
X86_CFLAGS    := $(CFLAGS_COMMON) -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2
X86_SRCS      := $(SOC_COMMON_SRCS) $(SRCDIR)/uiox_soc_x86.c
X86_OBJS      := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/x86_64/%.o,$(X86_SRCS))
X86_LIB       := $(BUILDDIR)/x86_64/libuiox_soc_x86_64.a

.PHONY: x86_64
x86_64: $(X86_LIB)
	$(Q)echo "  OK  SoC x86_64 => $(X86_LIB)"

$(X86_LIB): $(X86_OBJS) | $(BUILDDIR)/x86_64
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/x86_64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/x86_64
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/x86_64:
	$(Q)mkdir -p $@

# =============================================================================
# RISC-V 64  (rv64imac, medany model)
# =============================================================================
RV64_CFLAGS   := $(CFLAGS_COMMON) -march=rv64imac -mabi=lp64 -mcmodel=medany
RV64_SRCS     := $(SOC_COMMON_SRCS) $(SRCDIR)/uiox_soc_riscv64.c
RV64_OBJS     := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/riscv64/%.o,$(RV64_SRCS))
RV64_LIB      := $(BUILDDIR)/riscv64/libuiox_soc_riscv64.a

.PHONY: riscv64
riscv64: $(RV64_LIB)
	$(Q)echo "  OK  SoC RISC-V64 => $(RV64_LIB)"

$(RV64_LIB): $(RV64_OBJS) | $(BUILDDIR)/riscv64
	$(LOGAR)
	$(Q)ar rcs $@ $^

$(BUILDDIR)/riscv64/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/riscv64
	$(LOGCC)
	$(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILDDIR)/riscv64:
	$(Q)mkdir -p $@

# =============================================================================
# Auto-generated dependency includes
# =============================================================================
-include $(ARM64_OBJS:.o=.d)
-include $(ARM32_OBJS:.o=.d)
-include $(X86_OBJS:.o=.d)
-include $(RV64_OBJS:.o=.d)

# =============================================================================
# Top-level convenience targets
# =============================================================================
.PHONY: all
all: arm64 arm32 x86_64 riscv64

.PHONY: clean
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

.PHONY: help
help:
	$(Q)echo "UIOX SoC Platform Layer -- available targets:"
	$(Q)echo "  make arm64          build ARM64 static library"
	$(Q)echo "  make arm32          build ARM32 static library"
	$(Q)echo "  make x86_64         build x86_64 static library"
	$(Q)echo "  make riscv64        build RISC-V64 static library"
	$(Q)echo "  make all            build all four targets"
	$(Q)echo "  make clean          remove build/ directory"
	$(Q)echo "  make V=1 <target>   verbose build output"
