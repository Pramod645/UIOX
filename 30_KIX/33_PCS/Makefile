# =============================================================
# 30_KIX/33_PCS/Makefile
#
# Builds libpcs.a — the process-control subsystem static library.
# Called by the 30_KIX umbrella Makefile:
#   make CC=<cross-gcc> AR=<cross-ar> CFLAGS="<arch-flags>" static
# =============================================================

UNAMES := $(shell uname -s)
ifeq ($(UNAMES),Darwin)
  CC  ?= aarch64-elf-gcc
  AR  ?= aarch64-elf-ar
else
  CC  ?= aarch64-linux-gnu-gcc
  AR  ?= aarch64-linux-gnu-ar
endif

CFLAGS ?= \
  -std=c11 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin \
  -fno-stack-protector \
  -ffunction-sections -fdata-sections \
  -Os -g

BSPDIR    ?= $(realpath ../../10BSP)
SOCINC     = $(BSPDIR)/03SoC/include
ARCHINC    = $(BSPDIR)/10Arch/arm64/include
UIXLIBINC  = ../../50_UIX/00_libs/00_uixlibs

CINCLUDES  = \
  -Iinclude \
  -I40_procStruct/include \
  -I50_scps/include \
  -I$(SOCINC) \
  -I$(ARCHINC) \
  -I$(UIXLIBINC)

ARFLAGS    = rcs
OBJDIR     = build/obj
LIBDIR     = build/lib
STATIC     = $(LIBDIR)/libpcs.a

CSRCS := \
  40_procStruct/src/uiox_task.c \
  02_MemMngnt/src/uiox_phys_alloc.c \
  src/uiox_proc_init.c \
  50_scps/src/uiox_sys_call.c

CSRCS += $(wildcard 01_schedular/src/*.c)
CSRCS += $(wildcard 02_MemMngnt/src/uiox_timer*.c)

COBJS    := $(patsubst %.c,$(OBJDIR)/%.o,$(CSRCS))
DEPFLAGS  = -MMD -MP -MF $(@:.o=.d)
-include $(COBJS:.o=.d)

.PHONY: all static clean dirs
all: static
static: dirs $(STATIC)

$(STATIC): $(COBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CINCLUDES) $(DEPFLAGS) -c $< -o $@
	@echo "  [CC]  $<"

dirs: | $(OBJDIR) $(LIBDIR)
$(OBJDIR): ; mkdir -p $@
$(LIBDIR): ; mkdir -p $@
clean:
	rm -rf build/
