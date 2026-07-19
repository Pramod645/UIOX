# =============================================================
#  10_Arch/riscv64/Makefile
#  RISC-V 64-bit architecture layer (RV64IMAC + Zicsr)
# =============================================================
ARCH      := riscv64

UNAME_S   := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CC := riscv64-elf-gcc
  AR := riscv64-elf-ar
else
  CC := riscv64-linux-gnu-gcc
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

ARFLAGS  := rcs
SRCDIR   := src
OBJDIR   := obj
LIBDIR   := lib
STATIC   := $(LIBDIR)/libarch_$(ARCH).a

SRCS     := $(wildcard $(SRCDIR)/*.c)
OBJS     := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean dirs
all: dirs $(STATIC)

$(STATIC): $(OBJS) | $(LIBDIR)
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
