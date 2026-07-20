# =============================================================
#  10_Arch/arm64/Makefile
#  ARMv8-A 64-bit architecture layer — boot path only
#
#  Boot library:   lib/libarch_arm64.a
#  Runtime library: lib/libarch_arm64_runtime.a  (context switch,
#                    MMU, atomics, spinlocks — built separately)
# =============================================================
ARCH := arm64

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CC := aarch64-elf-gcc
  AS := aarch64-elf-gcc
  AR := aarch64-elf-ar
else
  CC := aarch64-linux-gnu-gcc
  AS := aarch64-linux-gnu-gcc
  AR := aarch64-linux-gnu-ar
endif

CFLAGS := \
  -std=c11 -Wall -Wextra -Wpedantic \
  -ffreestanding -fno-builtin \
  -fno-stack-protector \
  -ffunction-sections -fdata-sections \
  -Os -g \
  -march=armv8-a \
  -mstrict-align \
  -DARCH_ARM64 \
  -Iinclude \
  -I../../03_SoC/include

ASFLAGS := \
  -Wall \
  -ffreestanding \
  -march=armv8-a \
  -DARCH_ARM64 \
  -Iinclude

ARFLAGS  := rcs
SRCDIR   := src
OBJDIR   := obj
LIBDIR   := lib
STATIC          := $(LIBDIR)/libarch_$(ARCH).a
RUNTIME_STATIC  := $(LIBDIR)/libarch_$(ARCH)_runtime.a

# ── Boot-path sources (C only) ────────────────────────────────
# arch_init.c — GIC, cache enable, VBAR_EL1, generic timer
# arm64_exceptions.c — exception handler stubs
BOOT_SRCS := \
  $(SRCDIR)/arch_init.c         \
  $(SRCDIR)/arm64_exceptions.c

BOOT_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(BOOT_SRCS))

# ── Runtime sources (C + Assembly) ───────────────────────────
# Used by 33_ProcessControlSubsystem (context switch),
# 32_FileSystem (MMU map/unmap), 40_SystemCallInterface (syscall entry)
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
