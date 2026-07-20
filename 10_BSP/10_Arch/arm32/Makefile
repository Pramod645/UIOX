# =============================================================
#  10_Arch/arm32/Makefile
#  ARMv7-A 32-bit architecture layer — boot path only
#
#  NOTE: arm_decode.c / arm_execute.c / arm_memory.c are
#        ISA simulator files.  They are built into a separate
#        libarm32_sim.a if needed for testing, NOT into the
#        main boot library.
# =============================================================
ARCH := arm32

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CC := arm-none-eabi-gcc
  AR := arm-none-eabi-ar
else
  CC := arm-linux-gnueabihf-gcc
  AR := arm-linux-gnueabihf-ar
endif

CFLAGS := \
  -std=c11 -Wall -Wextra -Wpedantic \
  -ffreestanding -fno-builtin \
  -fno-stack-protector \
  -ffunction-sections -fdata-sections \
  -Os -g \
  -march=armv7-a+sec \
  -mthumb \
  -mfloat-abi=soft \
  -DARCH_ARM32 \
  -Iinclude \
  -I../../03_SoC/include

ARFLAGS  := rcs
SRCDIR   := src
OBJDIR   := obj
LIBDIR   := lib
STATIC   := $(LIBDIR)/libarch_$(ARCH).a

# Boot-path sources only — simulator files excluded
BOOT_SRCS := \
  $(SRCDIR)/arch_init.c      \
  $(SRCDIR)/arm_exceptions.c

BOOT_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(BOOT_SRCS))

# Simulator sources (separate target, optional)
SIM_SRCS := \
  $(SRCDIR)/arm_decode.c     \
  $(SRCDIR)/arm_execute.c    \
  $(SRCDIR)/arm_memory.c
SIM_OBJS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SIM_SRCS))
SIM_LIB   := $(LIBDIR)/libarm32_sim.a

.PHONY: all sim clean dirs
all: dirs $(STATIC)

$(STATIC): $(BOOT_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

sim: dirs $(SIM_LIB)

$(SIM_LIB): $(SIM_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

dirs: $(OBJDIR) $(LIBDIR)
$(OBJDIR): ; mkdir -p $@
$(LIBDIR): ; mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
