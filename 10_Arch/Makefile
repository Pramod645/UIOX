# =============================================================================
#  10_Arch/Makefile  —  UIOX Architecture Layer (common build)
#
#  Builds a static library for each supported architecture:
#    build/arm64/libarch_arm64.a
#    build/arm32/libarch_arm32.a
#    build/x86_64/libarch_x86_64.a
#    build/riscv64/libarch_riscv64.a
#
#  Targets:
#    make all          build all four libraries
#    make arm64        build ARM64 only
#    make arm32        build ARM32 only
#    make x86_64       build x86-64 only
#    make riscv64      build RISC-V 64 only
#    make clean        remove build/ directory
#    make V=1 <target> verbose output
#    make list-srcs    show source files per target
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

# -----------------------------------------------------------------------------
# Verbosity
# -----------------------------------------------------------------------------
V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC      $<"
  LOGAS    := @echo "  AS      $<"
  LOGAR    := @echo "  AR      $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q        :=
  LOGCC    :=
  LOGAS    :=
  LOGAR    :=
  LOGCLEAN :=
endif

# -----------------------------------------------------------------------------
# Directories
# -----------------------------------------------------------------------------
BUILDDIR      := build
SOC_INCDIR    := ../03_SoC/include   # SoC layer headers (stdio/string stubs)

# -----------------------------------------------------------------------------
# Toolchain selection  (Darwin = macOS cross-compilers, Linux = native)
# -----------------------------------------------------------------------------
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  ARM64_CC  := aarch64-elf-gcc
  ARM64_AR  := aarch64-elf-ar
  ARM32_CC  := arm-none-eabi-gcc
  ARM32_AR  := arm-none-eabi-ar
  X86_CC    := x86_64-elf-gcc
  X86_AR    := x86_64-elf-ar
  RV64_CC   := riscv64-elf-gcc
  RV64_AR   := riscv64-elf-ar
else
  ARM64_CC  := aarch64-linux-gnu-gcc
  ARM64_AR  := aarch64-linux-gnu-ar
  ARM32_CC  := arm-linux-gnueabihf-gcc
  ARM32_AR  := arm-linux-gnueabihf-ar
  X86_CC    := x86_64-linux-gnu-gcc
  X86_AR    := x86_64-linux-gnu-ar
  RV64_CC   := riscv64-linux-gnu-gcc
  RV64_AR   := riscv64-linux-gnu-ar
endif

ARFLAGS := rcs

# -----------------------------------------------------------------------------
# Common compiler flags shared by all four targets
# -----------------------------------------------------------------------------
CFLAGS_COMMON := \
  -std=c11                          \
  -Wall                             \
  -Wextra                           \
  -Wpedantic                        \
  -Wmissing-prototypes              \
  -Wstrict-prototypes               \
  -ffreestanding                    \
  -fno-builtin                      \
  -fno-stack-protector              \
  -ffunction-sections               \
  -fdata-sections                   \
  -Os                               \
  -g                                \
  -I$(SOC_INCDIR)                   \
  -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
  -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
  -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

# =============================================================================
# ARM64  (ARMv8-A / Cortex-A)
# =============================================================================
ARM64_SRCDIR  := arm64/src
ARM64_INCDIR  := arm64/include
ARM64_OBJDIR  := $(BUILDDIR)/arm64
ARM64_LIB     := $(ARM64_OBJDIR)/libarch_arm64.a

ARM64_CFLAGS  := $(CFLAGS_COMMON)   \
  -march=armv8-a                    \
  -mstrict-align                    \
  -DARCH_ARM64                      \
  -I$(ARM64_INCDIR)

ARM64_SRCS    := $(wildcard $(ARM64_SRCDIR)/*.c)
ARM64_OBJS    := $(patsubst $(ARM64_SRCDIR)/%.c,$(ARM64_OBJDIR)/%.o,$(ARM64_SRCS))

.PHONY: arm64
arm64: $(ARM64_LIB)
	$(Q)echo "  OK  arch ARM64 => $(ARM64_LIB)"

$(ARM64_LIB): $(ARM64_OBJS) | $(ARM64_OBJDIR)
	$(LOGAR)
	$(Q)$(ARM64_AR) $(ARFLAGS) $@ $^

$(ARM64_OBJDIR)/%.o: $(ARM64_SRCDIR)/%.c | $(ARM64_OBJDIR)
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM64_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# ARM32  (ARMv7-A / Cortex-A)
# =============================================================================
ARM32_SRCDIR  := arm32/src
ARM32_INCDIR  := arm32/include
ARM32_OBJDIR  := $(BUILDDIR)/arm32
ARM32_LIB     := $(ARM32_OBJDIR)/libarch_arm32.a

ARM32_CFLAGS  := $(CFLAGS_COMMON)   \
  -march=armv7-a+sec                \
  -mthumb                           \
  -mfloat-abi=soft                  \
  -DARCH_ARM32                      \
  -I$(ARM32_INCDIR)

ARM32_SRCS    := $(wildcard $(ARM32_SRCDIR)/*.c)
ARM32_OBJS    := $(patsubst $(ARM32_SRCDIR)/%.c,$(ARM32_OBJDIR)/%.o,$(ARM32_SRCS))

.PHONY: arm32
arm32: $(ARM32_LIB)
	$(Q)echo "  OK  arch ARM32 => $(ARM32_LIB)"

$(ARM32_LIB): $(ARM32_OBJS) | $(ARM32_OBJDIR)
	$(LOGAR)
	$(Q)$(ARM32_AR) $(ARFLAGS) $@ $^

$(ARM32_OBJDIR)/%.o: $(ARM32_SRCDIR)/%.c | $(ARM32_OBJDIR)
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM32_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# x86_64  (AMD64 / Intel 64)
# =============================================================================
X86_SRCDIR    := x86_64/src
X86_INCDIR    := x86_64/include
X86_OBJDIR    := $(BUILDDIR)/x86_64
X86_LIB       := $(X86_OBJDIR)/libarch_x86_64.a

X86_CFLAGS    := $(CFLAGS_COMMON)   \
  -m64                              \
  -mno-red-zone                     \
  -mno-mmx -mno-sse -mno-sse2       \
  -DARCH_X86_64                     \
  -I$(X86_INCDIR)

X86_ASFLAGS   := \
  -Wall -ffreestanding              \
  -I$(X86_INCDIR)

X86_C_SRCS    := $(wildcard $(X86_SRCDIR)/*.c)
X86_ASM_SRCS  := $(wildcard $(X86_SRCDIR)/*.S)
X86_C_OBJS    := $(patsubst $(X86_SRCDIR)/%.c,  $(X86_OBJDIR)/%.o, $(X86_C_SRCS))
X86_ASM_OBJS  := $(patsubst $(X86_SRCDIR)/%.S,  $(X86_OBJDIR)/%.o, $(X86_ASM_SRCS))
X86_OBJS      := $(X86_C_OBJS) $(X86_ASM_OBJS)

.PHONY: x86_64
x86_64: $(X86_LIB)
	$(Q)echo "  OK  arch x86_64 => $(X86_LIB)"

$(X86_LIB): $(X86_OBJS) | $(X86_OBJDIR)
	$(LOGAR)
	$(Q)$(X86_AR) $(ARFLAGS) $@ $^

$(X86_OBJDIR)/%.o: $(X86_SRCDIR)/%.c | $(X86_OBJDIR)
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(X86_OBJDIR)/%.o: $(X86_SRCDIR)/%.S | $(X86_OBJDIR)
	$(LOGAS)
	$(Q)$(X86_CC) $(X86_ASFLAGS) -c $< -o $@

$(X86_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# RISC-V 64  (RV64IMAC + Zicsr)
# =============================================================================
RV64_SRCDIR   := riscv64/src
RV64_INCDIR   := riscv64/include
RV64_OBJDIR   := $(BUILDDIR)/riscv64
RV64_LIB      := $(RV64_OBJDIR)/libarch_riscv64.a

RV64_CFLAGS   := $(CFLAGS_COMMON)   \
  -march=rv64imac_zicsr             \
  -mabi=lp64                        \
  -mcmodel=medany                   \
  -DARCH_RISCV64                    \
  -I$(RV64_INCDIR)

RV64_SRCS     := $(wildcard $(RV64_SRCDIR)/*.c)
RV64_OBJS     := $(patsubst $(RV64_SRCDIR)/%.c,$(RV64_OBJDIR)/%.o,$(RV64_SRCS))

.PHONY: riscv64
riscv64: $(RV64_LIB)
	$(Q)echo "  OK  arch RISC-V64 => $(RV64_LIB)"

$(RV64_LIB): $(RV64_OBJS) | $(RV64_OBJDIR)
	$(LOGAR)
	$(Q)$(RV64_AR) $(ARFLAGS) $@ $^

$(RV64_OBJDIR)/%.o: $(RV64_SRCDIR)/%.c | $(RV64_OBJDIR)
	$(LOGCC)
	$(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(RV64_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# Auto-generated dependency includes
# =============================================================================
-include $(ARM64_OBJS:.o=.d)
-include $(ARM32_OBJS:.o=.d)
-include $(X86_C_OBJS:.o=.d)
-include $(RV64_OBJS:.o=.d)

# =============================================================================
# Top-level targets
# =============================================================================
.PHONY: all
all: arm64 arm32 x86_64 riscv64

.PHONY: clean
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

.PHONY: help
help:
	$(Q)echo "UIOX Architecture Layer -- available targets:"
	$(Q)echo "  make arm64          build ARM64 static library"
	$(Q)echo "  make arm32          build ARM32 static library"
	$(Q)echo "  make x86_64         build x86-64 static library"
	$(Q)echo "  make riscv64        build RISC-V 64 static library"
	$(Q)echo "  make all            build all four targets"
	$(Q)echo "  make clean          remove build/ directory"
	$(Q)echo "  make V=1 <target>   verbose build"
	$(Q)echo ""
	$(Q)echo "Output libraries:"
	$(Q)echo "  build/arm64/libarch_arm64.a"
	$(Q)echo "  build/arm32/libarch_arm32.a"
	$(Q)echo "  build/x86_64/libarch_x86_64.a"
	$(Q)echo "  build/riscv64/libarch_riscv64.a"

.PHONY: list-srcs
list-srcs:
	$(Q)echo "--- ARM64 sources ---"
	$(Q)for f in $(ARM64_SRCS); do echo "  $$f"; done
	$(Q)echo "--- ARM32 sources ---"
	$(Q)for f in $(ARM32_SRCS); do echo "  $$f"; done
	$(Q)echo "--- x86_64 sources ---"
	$(Q)for f in $(X86_C_SRCS) $(X86_ASM_SRCS); do echo "  $$f"; done
	$(Q)echo "--- RISC-V64 sources ---"
	$(Q)for f in $(RV64_SRCS); do echo "  $$f"; done
