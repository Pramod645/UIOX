# =============================================================================
#  10_BSP/Makefile  —  static or dynamic kernel, same source tree
#
#  make ARCH=arm64                    → static  (BSP+kernel one image)
#  make ARCH=arm64 DYNAMIC_KERNEL=1   → dynamic (BSP standalone ELF)
#  make all                           → all four arches, static
#  make all DYNAMIC_KERNEL=1          → all four arches, dynamic
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC    $<"
  LOGAS    := @echo "  AS    $<"
  LOGAR    := @echo "  AR    $@"
  LOGLD    := @echo "  LD    $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q := LOGCC := LOGAS := LOGAR := LOGLD := LOGCLEAN :=
endif

# ── Dynamic kernel switch ─────────────────────────────────────────
# Set DYNAMIC_KERNEL=1 on the make command line to enable.
# Adds -DUIOX_DYNAMIC_KERNEL_LOAD and links uiox_kernel_loader.c.
DYNAMIC_KERNEL ?= 0
ifeq ($(DYNAMIC_KERNEL),1)
  DYNAMIC_FLAG := -DUIOX_DYNAMIC_KERNEL_LOAD
  DYNAMIC_SRC  := soc/src/uiox_kernel_loader.c
  BUILD_MODE   := dynamic
else
  DYNAMIC_FLAG :=
  DYNAMIC_SRC  :=
  BUILD_MODE   := static
endif

BUILDDIR   := build
SOC_INCDIR := soc/include

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC := aarch64-elf-gcc  ;  ARM64_AR := aarch64-elf-ar
  ARM32_CC := arm-none-eabi-gcc;  ARM32_AR := arm-none-eabi-ar
  X86_CC   := x86_64-elf-gcc  ;  X86_AR   := x86_64-elf-ar
  RV64_CC  := riscv64-elf-gcc  ;  RV64_AR  := riscv64-elf-ar
else
  ARM64_CC := aarch64-linux-gnu-gcc  ;  ARM64_AR := aarch64-linux-gnu-ar
  ARM32_CC := arm-linux-gnueabihf-gcc;  ARM32_AR := arm-linux-gnueabihf-ar
  X86_CC   := x86_64-linux-gnu-gcc   ;  X86_AR   := x86_64-linux-gnu-ar
  RV64_CC  := riscv64-linux-gnu-gcc  ;  RV64_AR  := riscv64-linux-gnu-ar
endif

ARFLAGS := rcs
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

CFLAGS_COMMON := \
  -std=c11 -Wall -Wextra -Wpedantic        \
  -Wmissing-prototypes -Wstrict-prototypes  \
  -ffreestanding -fno-builtin               \
  -fno-stack-protector                      \
  -ffunction-sections -fdata-sections       \
  -Os -g                                    \
  -I$(SOC_INCDIR)                           \
  $(DYNAMIC_FLAG)                           \
  -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR)     \
  -DUIOX_VERSION_MINOR=$(VERSION_MINOR)     \
  -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

# ── Common SoC sources (both modes) ──────────────────────────────
SOC_COMMON_SRCS := \
  soc/src/uiox_soc_main.c         \
  soc/src/uiox_soc_clk.c          \
  soc/src/uiox_soc_irq.c          \
  soc/src/uiox_soc_mem.c          \
  soc/src/uiox_soc_power.c        \
  soc/src/uiox_soc_post.c         \
  soc/src/uiox_soc_psci.c         \
  soc/src/uiox_soc_secboot.c      \
  soc/src/uiox_soc_tz.c           \
  soc/src/uiox_soc_dma.c          \
  soc/src/uiox_soc_pcie.c         \
  soc/src/uiox_soc_hw.c           \
  soc/src/uiox_soc_stdio.c        \
  uiox_arch_main.c                \
  $(DYNAMIC_SRC)                  # empty in static, loader in dynamic

# =============================================================================
# ARM64
# =============================================================================
ARM64_SRCDIR := arch/arm64/src
ARM64_INCDIR := arch/arm64/include
ARM64_OBJDIR := $(BUILDDIR)/arm64

ARM64_CFLAGS := $(CFLAGS_COMMON)    \
  -march=armv8-a -mstrict-align     \
  -DARCH_ARM64 -I$(ARM64_INCDIR)

ARM64_ASFLAGS := -Wall -ffreestanding -march=armv8-a \
  -DARCH_ARM64 -I$(ARM64_INCDIR)

ARM64_ARCH_SRCS := \
  $(ARM64_SRCDIR)/arch_init.c    \
  $(ARM64_SRCDIR)/arch_runtime.c

ARM64_ARCH_ASMS := \
  $(ARM64_SRCDIR)/arch_context.S \
  $(ARM64_SRCDIR)/arch_irq.S

ARM64_ARCH_CSRCS := \
  soc/src/uiox_soc_arm64.c       \
  soc/src/uiox_soc_imx8mp.c      \
  soc/src/uiox_soc_rk3588.c

ARM64_ALL_SRCS  := $(ARM64_ARCH_SRCS) $(ARM64_ARCH_CSRCS) $(SOC_COMMON_SRCS)
ARM64_C_OBJS    := $(patsubst %.c,$(ARM64_OBJDIR)/%.o,$(notdir $(ARM64_ALL_SRCS)))
ARM64_S_OBJS    := $(patsubst %.S,$(ARM64_OBJDIR)/%.o,$(notdir $(ARM64_ARCH_ASMS)))
ARM64_OBJS      := $(ARM64_C_OBJS) $(ARM64_S_OBJS)
ARM64_LIB       := $(ARM64_OBJDIR)/libarch_arm64.a

.PHONY: arm64
arm64: dirs $(ARM64_LIB)
	$(Q)echo "  OK  BSP ARM64 ($(BUILD_MODE)) => $(ARM64_LIB)"

$(ARM64_LIB): $(ARM64_OBJS) | $(ARM64_OBJDIR)
	$(LOGAR) ; $(Q)$(ARM64_AR) $(ARFLAGS) $@ $^

$(ARM64_OBJDIR)/%.o: $(ARM64_SRCDIR)/%.c | $(ARM64_OBJDIR)
	$(LOGCC) ; $(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM64_OBJDIR)/%.o: $(ARM64_SRCDIR)/%.S | $(ARM64_OBJDIR)
	$(LOGAS) ; $(Q)$(ARM64_CC) $(ARM64_ASFLAGS) -c $< -o $@

$(ARM64_OBJDIR)/%.o: soc/src/%.c | $(ARM64_OBJDIR)
	$(LOGCC) ; $(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM64_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(ARM64_OBJDIR)
	$(LOGCC) ; $(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM64_OBJDIR): ; $(Q)mkdir -p $@

# =============================================================================
# ARM32
# =============================================================================
ARM32_SRCDIR := arch/arm32/src
ARM32_INCDIR := arch/arm32/include
ARM32_OBJDIR := $(BUILDDIR)/arm32

ARM32_CFLAGS := $(CFLAGS_COMMON)                          \
  -march=armv7-a+sec -mthumb -mfloat-abi=soft             \
  -DARCH_ARM32 -I$(ARM32_INCDIR)

ARM32_ASFLAGS := -Wall -ffreestanding -march=armv7-a+sec -mthumb \
  -DARCH_ARM32 -I$(ARM32_INCDIR)

ARM32_ARCH_SRCS := \
  $(ARM32_SRCDIR)/arch_init.c    \
  $(ARM32_SRCDIR)/arch_runtime.c

ARM32_ARCH_ASMS := \
  $(ARM32_SRCDIR)/arch_context.S \
  $(ARM32_SRCDIR)/arch_irq.S

ARM32_ARCH_CSRCS := \
  soc/src/uiox_soc_arm32.c       \
  soc/src/uiox_soc_omap4430.c

ARM32_ALL_SRCS := $(ARM32_ARCH_SRCS) $(ARM32_ARCH_CSRCS) $(SOC_COMMON_SRCS)
ARM32_C_OBJS   := $(patsubst %.c,$(ARM32_OBJDIR)/%.o,$(notdir $(ARM32_ALL_SRCS)))
ARM32_S_OBJS   := $(patsubst %.S,$(ARM32_OBJDIR)/%.o,$(notdir $(ARM32_ARCH_ASMS)))
ARM32_OBJS     := $(ARM32_C_OBJS) $(ARM32_S_OBJS)
ARM32_LIB      := $(ARM32_OBJDIR)/libarch_arm32.a

.PHONY: arm32
arm32: dirs $(ARM32_LIB)
	$(Q)echo "  OK  BSP ARM32 ($(BUILD_MODE)) => $(ARM32_LIB)"

$(ARM32_LIB): $(ARM32_OBJS) | $(ARM32_OBJDIR)
	$(LOGAR) ; $(Q)$(ARM32_AR) $(ARFLAGS) $@ $^

$(ARM32_OBJDIR)/%.o: $(ARM32_SRCDIR)/%.c | $(ARM32_OBJDIR)
	$(LOGCC) ; $(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM32_OBJDIR)/%.o: $(ARM32_SRCDIR)/%.S | $(ARM32_OBJDIR)
	$(LOGAS) ; $(Q)$(ARM32_CC) $(ARM32_ASFLAGS) -c $< -o $@

$(ARM32_OBJDIR)/%.o: soc/src/%.c | $(ARM32_OBJDIR)
	$(LOGCC) ; $(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM32_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(ARM32_OBJDIR)
	$(LOGCC) ; $(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM32_OBJDIR): ; $(Q)mkdir -p $@

# =============================================================================
# x86_64
# =============================================================================
X86_SRCDIR := arch/x86_64/src
X86_INCDIR := arch/x86_64/include
X86_OBJDIR := $(BUILDDIR)/x86_64

X86_CFLAGS := $(CFLAGS_COMMON)                               \
  -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2             \
  -DARCH_X86_64 -I$(X86_INCDIR)

X86_ASFLAGS := -Wall -ffreestanding -m64 -DARCH_X86_64 -I$(X86_INCDIR)

X86_ARCH_SRCS := \
  $(X86_SRCDIR)/arch_init.c    \
  $(X86_SRCDIR)/arch_runtime.c

X86_ARCH_ASMS := \
  $(X86_SRCDIR)/arch_context.S \
  $(X86_SRCDIR)/arch_irq.S

X86_ARCH_CSRCS := soc/src/uiox_soc_x86.c

X86_ALL_SRCS := $(X86_ARCH_SRCS) $(X86_ARCH_CSRCS) $(SOC_COMMON_SRCS)
X86_C_OBJS   := $(patsubst %.c,$(X86_OBJDIR)/%.o,$(notdir $(X86_ALL_SRCS)))
X86_S_OBJS   := $(patsubst %.S,$(X86_OBJDIR)/%.o,$(notdir $(X86_ARCH_ASMS)))
X86_OBJS     := $(X86_C_OBJS) $(X86_S_OBJS)
X86_LIB      := $(X86_OBJDIR)/libarch_x86_64.a

.PHONY: x86_64
x86_64: dirs $(X86_LIB)
	$(Q)echo "  OK  BSP x86_64 ($(BUILD_MODE)) => $(X86_LIB)"

$(X86_LIB): $(X86_OBJS) | $(X86_OBJDIR)
	$(LOGAR) ; $(Q)$(X86_AR) $(ARFLAGS) $@ $^

$(X86_OBJDIR)/%.o: $(X86_SRCDIR)/%.c | $(X86_OBJDIR)
	$(LOGCC) ; $(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(X86_OBJDIR)/%.o: $(X86_SRCDIR)/%.S | $(X86_OBJDIR)
	$(LOGAS) ; $(Q)$(X86_CC) $(X86_ASFLAGS) -c $< -o $@

$(X86_OBJDIR)/%.o: soc/src/%.c | $(X86_OBJDIR)
	$(LOGCC) ; $(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(X86_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(X86_OBJDIR)
	$(LOGCC) ; $(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(X86_OBJDIR): ; $(Q)mkdir -p $@

# =============================================================================
# RISC-V 64
# =============================================================================
RV64_SRCDIR := arch/riscv64/src
RV64_INCDIR := arch/riscv64/include
RV64_OBJDIR := $(BUILDDIR)/riscv64

RV64_CFLAGS := $(CFLAGS_COMMON)                               \
  -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany            \
  -DARCH_RISCV64 -I$(RV64_INCDIR)

RV64_ASFLAGS := -Wall -ffreestanding                          \
  -march=rv64imac_zicsr -mabi=lp64 -mcmodel=medany            \
  -DARCH_RISCV64 -I$(RV64_INCDIR)

RV64_ARCH_SRCS := \
  $(RV64_SRCDIR)/arch_init.c    \
  $(RV64_SRCDIR)/arch_runtime.c

RV64_ARCH_ASMS := \
  $(RV64_SRCDIR)/arch_context.S \
  $(RV64_SRCDIR)/arch_irq.S

RV64_ARCH_CSRCS := \
  soc/src/uiox_soc_riscv64.c   \
  soc/src/uiox_soc_th1520.c

RV64_ALL_SRCS := $(RV64_ARCH_SRCS) $(RV64_ARCH_CSRCS) $(SOC_COMMON_SRCS)
RV64_C_OBJS   := $(patsubst %.c,$(RV64_OBJDIR)/%.o,$(notdir $(RV64_ALL_SRCS)))
RV64_S_OBJS   := $(patsubst %.S,$(RV64_OBJDIR)/%.o,$(notdir $(RV64_ARCH_ASMS)))
RV64_OBJS     := $(RV64_C_OBJS) $(RV64_S_OBJS)
RV64_LIB      := $(RV64_OBJDIR)/libarch_riscv64.a

.PHONY: riscv64
riscv64: dirs $(RV64_LIB)
	$(Q)echo "  OK  BSP RISC-V64 ($(BUILD_MODE)) => $(RV64_LIB)"

$(RV64_LIB): $(RV64_OBJS) | $(RV64_OBJDIR)
	$(LOGAR) ; $(Q)$(RV64_AR) $(ARFLAGS) $@ $^

$(RV64_OBJDIR)/%.o: $(RV64_SRCDIR)/%.c | $(RV64_OBJDIR)
	$(LOGCC) ; $(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(RV64_OBJDIR)/%.o: $(RV64_SRCDIR)/%.S | $(RV64_OBJDIR)
	$(LOGAS) ; $(Q)$(RV64_CC) $(RV64_ASFLAGS) -c $< -o $@

$(RV64_OBJDIR)/%.o: soc/src/%.c | $(RV64_OBJDIR)
	$(LOGCC) ; $(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(RV64_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(RV64_OBJDIR)
	$(LOGCC) ; $(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(RV64_OBJDIR): ; $(Q)mkdir -p $@

# =============================================================================
# Dependencies
# =============================================================================
-include $(ARM64_C_OBJS:.o=.d)
-include $(ARM32_C_OBJS:.o=.d)
-include $(X86_C_OBJS:.o=.d)
-include $(RV64_C_OBJS:.o=.d)

# =============================================================================
# Top-level
# =============================================================================
.PHONY: all clean dirs help

dirs:
	$(Q)mkdir -p \
	  $(ARM64_OBJDIR) $(ARM32_OBJDIR) $(X86_OBJDIR) $(RV64_OBJDIR)

all: arm64 arm32 x86_64 riscv64
	$(Q)echo ""
	$(Q)echo "Build mode : $(BUILD_MODE)"
	$(Q)echo "Libraries  :"
	$(Q)echo "  $(ARM64_LIB)"
	$(Q)echo "  $(ARM32_LIB)"
	$(Q)echo "  $(X86_LIB)"
	$(Q)echo "  $(RV64_LIB)"

clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

help:
	$(Q)echo "Usage:"
	$(Q)echo "  make [ARCH=<arch>] [DYNAMIC_KERNEL=1] [V=1]"
	$(Q)echo ""
	$(Q)echo "  ARCH values : arm64  arm32  x86_64  riscv64"
	$(Q)echo ""
	$(Q)echo "  Static  (default)   — BSP+kernel one image"
	$(Q)echo "    make all"
	$(Q)echo "    make arm64"
	$(Q)echo ""
	$(Q)echo "  Dynamic — BSP standalone ELF, loads kernel at runtime"
	$(Q)echo "    make all DYNAMIC_KERNEL=1"
	$(Q)echo "    make arm64 DYNAMIC_KERNEL=1"
	$(Q)echo ""
	$(Q)echo "  Verbose:"
	$(Q)echo "    make arm64 V=1"
