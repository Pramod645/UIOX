# =============================================================================
#  10_Arch/Makefile
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC      $<"
  LOGAS    := @echo "  AS      $<"
  LOGAR    := @echo "  AR      $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q        :=  LOGCC  :=  LOGAS  :=  LOGAR  :=  LOGCLEAN :=
endif

BUILDDIR   := build
SOC_INCDIR := ../03_SoC/include

# uiox_arch_main.c lives HERE, parallel to the Makefile
ARCH_MAIN_C := uiox_arch_main.c

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  ARM64_CC := aarch64-elf-gcc
  ARM64_AR := aarch64-elf-ar
  ARM32_CC := arm-none-eabi-gcc
  ARM32_AR := arm-none-eabi-ar
  X86_CC   := x86_64-elf-gcc
  X86_AR   := x86_64-elf-ar
  RV64_CC  := riscv64-elf-gcc
  RV64_AR  := riscv64-elf-ar
else
  ARM64_CC := aarch64-linux-gnu-gcc
  ARM64_AR := aarch64-linux-gnu-ar
  ARM32_CC := arm-linux-gnueabihf-gcc
  ARM32_AR := arm-linux-gnueabihf-ar
  X86_CC   := x86_64-linux-gnu-gcc
  X86_AR   := x86_64-linux-gnu-ar
  RV64_CC  := riscv64-linux-gnu-gcc
  RV64_AR  := riscv64-linux-gnu-ar
endif

ARFLAGS := rcs

CFLAGS_COMMON := \
  -std=c11 -Wall -Wextra -Wpedantic          \
  -Wmissing-prototypes -Wstrict-prototypes    \
  -ffreestanding -fno-builtin                \
  -fno-stack-protector                       \
  -ffunction-sections -fdata-sections        \
  -Os -g                                     \
  -I$(SOC_INCDIR)                            \
  -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR)      \
  -DUIOX_VERSION_MINOR=$(VERSION_MINOR)      \
  -DUIOX_VERSION_PATCH=$(VERSION_PATCH)

DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

# =============================================================================
# ARM64
# =============================================================================
ARM64_SRCDIR := arm64/src
ARM64_INCDIR := arm64/include
ARM64_OBJDIR := $(BUILDDIR)/arm64
ARM64_LIB    := $(ARM64_OBJDIR)/libarch_arm64.a

ARM64_CFLAGS := $(CFLAGS_COMMON) \
  -march=armv8-a -mstrict-align  \
  -DARCH_ARM64 -I$(ARM64_INCDIR)

ARM64_ASFLAGS := -Wall -ffreestanding -march=armv8-a \
  -DARCH_ARM64 -I$(ARM64_INCDIR)

ARM64_C_SRCS := \
  $(ARM64_SRCDIR)/arch_init.c    \
  $(ARM64_SRCDIR)/arch_runtime.c \
  $(ARCH_MAIN_C)

ARM64_S_SRCS := \
  $(ARM64_SRCDIR)/arch_context.S \
  $(ARM64_SRCDIR)/arch_irq.S

ARM64_C_OBJS := $(patsubst %.c,$(ARM64_OBJDIR)/%.o,$(notdir $(ARM64_C_SRCS)))
ARM64_S_OBJS := $(patsubst %.S,$(ARM64_OBJDIR)/%.o,$(notdir $(ARM64_S_SRCS)))
ARM64_OBJS   := $(ARM64_C_OBJS) $(ARM64_S_OBJS)

.PHONY: arm64
arm64: $(ARM64_LIB)
	$(Q)echo "  OK  arch ARM64 => $(ARM64_LIB)"

$(ARM64_LIB): $(ARM64_OBJS) | $(ARM64_OBJDIR)
	$(LOGAR)
	$(Q)$(ARM64_AR) $(ARFLAGS) $@ $^

$(ARM64_OBJDIR)/%.o: $(ARM64_SRCDIR)/%.c | $(ARM64_OBJDIR)
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM64_OBJDIR)/%.o: $(ARM64_SRCDIR)/%.S | $(ARM64_OBJDIR)
	$(LOGAS)
	$(Q)$(ARM64_CC) $(ARM64_ASFLAGS) -c $< -o $@

# uiox_arch_main.c compiled with ARM64 flags into ARM64 objdir
$(ARM64_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(ARM64_OBJDIR)
	$(LOGCC)
	$(Q)$(ARM64_CC) $(ARM64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM64_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# ARM32
# =============================================================================
ARM32_SRCDIR := arm32/src
ARM32_INCDIR := arm32/include
ARM32_OBJDIR := $(BUILDDIR)/arm32
ARM32_LIB    := $(ARM32_OBJDIR)/libarch_arm32.a

ARM32_CFLAGS := $(CFLAGS_COMMON) \
  -march=armv7-a+sec -mthumb -mfloat-abi=soft \
  -DARCH_ARM32 -I$(ARM32_INCDIR)

ARM32_ASFLAGS := -Wall -ffreestanding -march=armv7-a+sec -mthumb \
  -DARCH_ARM32 -I$(ARM32_INCDIR)

ARM32_C_SRCS := \
  $(ARM32_SRCDIR)/arch_init.c    \
  $(ARM32_SRCDIR)/arch_runtime.c \
  $(ARCH_MAIN_C)

ARM32_S_SRCS := \
  $(ARM32_SRCDIR)/arch_context.S \
  $(ARM32_SRCDIR)/arch_irq.S

ARM32_C_OBJS := $(patsubst %.c,$(ARM32_OBJDIR)/%.o,$(notdir $(ARM32_C_SRCS)))
ARM32_S_OBJS := $(patsubst %.S,$(ARM32_OBJDIR)/%.o,$(notdir $(ARM32_S_SRCS)))
ARM32_OBJS   := $(ARM32_C_OBJS) $(ARM32_S_OBJS)

.PHONY: arm32
arm32: $(ARM32_LIB)
	$(Q)echo "  OK  arch ARM32 => $(ARM32_LIB)"

$(ARM32_LIB): $(ARM32_OBJS) | $(ARM32_OBJDIR)
	$(LOGAR)
	$(Q)$(ARM32_AR) $(ARFLAGS) $@ $^

$(ARM32_OBJDIR)/%.o: $(ARM32_SRCDIR)/%.c | $(ARM32_OBJDIR)
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM32_OBJDIR)/%.o: $(ARM32_SRCDIR)/%.S | $(ARM32_OBJDIR)
	$(LOGAS)
	$(Q)$(ARM32_CC) $(ARM32_ASFLAGS) -c $< -o $@

$(ARM32_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(ARM32_OBJDIR)
	$(LOGCC)
	$(Q)$(ARM32_CC) $(ARM32_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(ARM32_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# x86_64
# =============================================================================
X86_SRCDIR := x86_64/src
X86_INCDIR := x86_64/include
X86_OBJDIR := $(BUILDDIR)/x86_64
X86_LIB    := $(X86_OBJDIR)/libarch_x86_64.a

X86_CFLAGS := $(CFLAGS_COMMON) \
  -m64 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
  -DARCH_X86_64 -I$(X86_INCDIR)

X86_ASFLAGS := -Wall -ffreestanding -m64 \
  -DARCH_X86_64 -I$(X86_INCDIR)

X86_C_SRCS := \
  $(X86_SRCDIR)/arch_init.c    \
  $(X86_SRCDIR)/arch_runtime.c \
  $(ARCH_MAIN_C)

X86_S_SRCS := \
  $(X86_SRCDIR)/arch_context.S \
  $(X86_SRCDIR)/arch_irq.S

X86_C_OBJS := $(patsubst %.c,$(X86_OBJDIR)/%.o,$(notdir $(X86_C_SRCS)))
X86_S_OBJS := $(patsubst %.S,$(X86_OBJDIR)/%.o,$(notdir $(X86_S_SRCS)))
X86_OBJS   := $(X86_C_OBJS) $(X86_S_OBJS)

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

$(X86_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(X86_OBJDIR)
	$(LOGCC)
	$(Q)$(X86_CC) $(X86_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(X86_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# RISC-V 64
# =============================================================================
RV64_SRCDIR := riscv64/src
RV64_INCDIR := riscv64/include
RV64_OBJDIR := $(BUILDDIR)/riscv64
RV64_LIB    := $(RV64_OBJDIR)/libarch_riscv64.a

RV64_CFLAGS := $(CFLAGS_COMMON) \
  -march=rv64imafdc_zicsr_zifencei -mabi=lp64d -mcmodel=medany \
  -DARCH_RISCV64 -I$(RV64_INCDIR)

RV64_ASFLAGS := -Wall -ffreestanding \
  -march=rv64imafdc_zicsr_zifencei -mabi=lp64d \
  -DARCH_RISCV64 -I$(RV64_INCDIR)

RV64_C_SRCS := \
  $(RV64_SRCDIR)/arch_init.c    \
  $(RV64_SRCDIR)/arch_runtime.c \
  $(ARCH_MAIN_C)

RV64_S_SRCS := \
  $(RV64_SRCDIR)/arch_context.S \
  $(RV64_SRCDIR)/arch_irq.S

RV64_C_OBJS := $(patsubst %.c,$(RV64_OBJDIR)/%.o,$(notdir $(RV64_C_SRCS)))
RV64_S_OBJS := $(patsubst %.S,$(RV64_OBJDIR)/%.o,$(notdir $(RV64_S_SRCS)))
RV64_OBJS   := $(RV64_C_OBJS) $(RV64_S_OBJS)

.PHONY: riscv64
riscv64: $(RV64_LIB)
	$(Q)echo "  OK  arch RISC-V64 => $(RV64_LIB)"

$(RV64_LIB): $(RV64_OBJS) | $(RV64_OBJDIR)
	$(LOGAR)
	$(Q)$(RV64_AR) $(ARFLAGS) $@ $^

$(RV64_OBJDIR)/%.o: $(RV64_SRCDIR)/%.c | $(RV64_OBJDIR)
	$(LOGCC)
	$(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(RV64_OBJDIR)/%.o: $(RV64_SRCDIR)/%.S | $(RV64_OBJDIR)
	$(LOGAS)
	$(Q)$(RV64_CC) $(RV64_ASFLAGS) -c $< -o $@

$(RV64_OBJDIR)/uiox_arch_main.o: uiox_arch_main.c | $(RV64_OBJDIR)
	$(LOGCC)
	$(Q)$(RV64_CC) $(RV64_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(RV64_OBJDIR):
	$(Q)mkdir -p $@

# =============================================================================
# Auto-generated dependency includes
# =============================================================================
-include $(ARM64_C_OBJS:.o=.d)
-include $(ARM32_C_OBJS:.o=.d)
-include $(X86_C_OBJS:.o=.d)
-include $(RV64_C_OBJS:.o=.d)

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
	$(Q)echo "  make arm64      make arm32      make x86_64      make riscv64"
	$(Q)echo "  make all        make clean      make V=1 <tgt>   make list-srcs"
	$(Q)echo ""
	$(Q)echo "Kernel entry: uiox_arch_main(dtb_pa)  in uiox_arch_main.c"
	$(Q)echo "  calls arch_init() → uiox_soc_init() in that order"

.PHONY: list-srcs
list-srcs:
	$(Q)echo "--- ARM64 ---"   ; for f in $(ARM64_C_SRCS) $(ARM64_S_SRCS); do echo "  $$f"; done
	$(Q)echo "--- ARM32 ---"   ; for f in $(ARM32_C_SRCS) $(ARM32_S_SRCS); do echo "  $$f"; done
	$(Q)echo "--- x86_64 ---"  ; for f in $(X86_C_SRCS)   $(X86_S_SRCS);   do echo "  $$f"; done
	$(Q)echo "--- RISC-V64 ---"; for f in $(RV64_C_SRCS)  $(RV64_S_SRCS);  do echo "  $$f"; done
