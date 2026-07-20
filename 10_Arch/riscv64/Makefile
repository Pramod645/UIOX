# =============================================================
#  10_Arch/riscv64/Makefile
#  RISC-V RV64IMAC_Zicsr architecture layer — boot path only
#
#  Boot library:    lib/libarch_riscv64.a
#  Runtime library: lib/libarch_riscv64_runtime.a
#
#  NOTE: -march=rv64imac_zicsr is required so that csrr/csrw
#        instructions assemble without error (Zicsr extension).
# =============================================================
ARCH := riscv64

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CC := riscv64-elf-gcc
  AS := riscv64-elf-gcc
  AR := riscv64-elf-ar
else
  CC := riscv64-linux-gnu-gcc
  AS := riscv64-linux-gnu-gcc
  AR := riscv64-linux-gnu-ar
endif

CFLAGS := \
  -std=c11 -Wall -Wextra -Wpedantic \
  -ffreestanding -fno-builtin \
  -fno-stack-protector \
  -ffunction-sections -fdata-sections \
  -Os -g \
  -march=rv64imac_zicsr \
  -mabi=lp64 \
  -mcmodel=medany \
  -DARCH_RISCV64 \
  -Iinclude \
  -I../../03_SoC/include

ASFLAGS := \
  -Wall \
  -ffreestanding \
  -march=rv64imac_zicsr \
  -mabi=lp64 \
  -mcmodel=medany \
  -DARCH_RISCV64 \
  -Iinclude \
  -option norvc

ARFLAGS  := rcs
SRCDIR   := src
OBJDIR   := obj
LIBDIR   := lib
STATIC         := $(LIBDIR)/libarch_$(ARCH).a
RUNTIME_STATIC := $(LIBDIR)/libarch_$(ARCH)_runtime.a

# ── Boot-path sources (C only) ────────────────────────────────
# arch_init.c — CLINT clear, PLIC init, stvec, CSR setup
BOOT_SRCS := \
  $(SRCDIR)/arch_init.c

BOOT_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(BOOT_SRCS))

# ── Runtime sources (C + Assembly) ───────────────────────────
# Used by 33_ProcessControlSubsystem, 32_FileSystem, 40_SCI
RUNTIME_C_SRCS := \
  $(SRCDIR)/arch_runtime.c

RUNTIME_ASM_SRCS := \
  $(SRCDIR)/arch_context.S      \
  $(SRCDIR)/arch_irq.S

RUNTIME_C_OBJS   := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(RUNTIME_C_SRCS))
RUNTIME_ASM_OBJS := $(patsubst $(SRCDIR)/%.S,$(OBJDIR)/%.o,$(RUNTIME_ASM_SRCS))
RUNTIME_OBJS     := $(RUNTIME_C_OBJS) $(RUNTIME_ASM_OBJS)

# ── Auto dependency tracking ──────────────────────────────────
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
-include $(BOOT_OBJS:.o=.d)
-include $(RUNTIME_C_OBJS:.o=.d)

.PHONY: all runtime clean dirs
all: dirs $(STATIC)

runtime: dirs $(RUNTIME_STATIC)

$(STATIC): $(BOOT_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(RUNTIME_STATIC): $(RUNTIME_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

# ── C compile rule ────────────────────────────────────────────
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

# ── Assembly compile rule ─────────────────────────────────────
$(OBJDIR)/%.o: $(SRCDIR)/%.S | $(OBJDIR)
	$(AS) $(ASFLAGS) -c $< -o $@
	@echo "  [AS]  $<"

dirs: $(OBJDIR) $(LIBDIR)
$(OBJDIR): ; mkdir -p $@
$(LIBDIR): ; mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
