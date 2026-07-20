# =============================================================
#  10_Arch/x86_64/Makefile
#  AMD64 / Intel 64-bit architecture layer
# =============================================================
ARCH      := x86_64

UNAME_S   := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CC := x86_64-elf-gcc
  AR := x86_64-elf-ar
else
  CC := x86_64-linux-gnu-gcc
  AR := x86_64-linux-gnu-ar
endif

CFLAGS := \
  -std=c11 -Wall -Wextra -Wpedantic \
  -ffreestanding -fno-builtin \
  -fno-stack-protector \
  -ffunction-sections -fdata-sections \
  -Os -g \
  -m64 \
  -mno-red-zone \
  -mno-mmx -mno-sse -mno-sse2 \
  -DARCH_X86_64 \
  -Iinclude \
  -I../../03_SoC/include

ASFLAGS := \
  -Wall \
  -ffreestanding \
  -Iinclude

ARFLAGS  := rcs
SRCDIR   := src
OBJDIR   := obj
LIBDIR   := lib
STATIC   := $(LIBDIR)/libarch_$(ARCH).a

C_SRCS   := $(wildcard $(SRCDIR)/*.c)
ASM_SRCS := $(wildcard $(SRCDIR)/*.S)
C_OBJS   := $(patsubst $(SRCDIR)/%.c,  $(OBJDIR)/%.o, $(C_SRCS))
ASM_OBJS := $(patsubst $(SRCDIR)/%.S,  $(OBJDIR)/%.o, $(ASM_SRCS))
OBJS     := $(C_OBJS) $(ASM_OBJS)

.PHONY: all clean dirs
all: dirs $(STATIC)

$(STATIC): $(OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

$(OBJDIR)/%.o: $(SRCDIR)/%.S | $(OBJDIR)
	$(CC) $(ASFLAGS) -c $< -o $@
	@echo "  [AS]  $<"

dirs: $(OBJDIR) $(LIBDIR)
$(OBJDIR): ; mkdir -p $@
$(LIBDIR): ; mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
