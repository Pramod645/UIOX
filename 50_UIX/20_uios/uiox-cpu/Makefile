# =============================================================================
# UIOX CPU/SoC Stack — Makefile
#
# Supports: ARM64 | ARM32 | X86_64 | RV64
# Targets : all lib demo clean distclean install uninstall
#           size dump lint format docs tags info
# =============================================================================

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0
VERSION       := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC      $<"
  LOGAR    := @echo "  AR      $@"
  LOGLD    := @echo "  LD      $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q        :=
  LOGCC    :=
  LOGAR    :=
  LOGLD    :=
  LOGCLEAN :=
endif

# -----------------------------------------------------------------------------
# Platform detection and toolchain flags
# -----------------------------------------------------------------------------

ifndef PLATFORM
  MACHINE := $(shell uname -m)
  ifeq ($(MACHINE),aarch64)
    PLATFORM := ARM64
  else ifeq ($(MACHINE),armv7l)
    PLATFORM := ARM32
  else ifeq ($(MACHINE),riscv64)
    PLATFORM := RV64
  else
    PLATFORM := X86_64
  endif
endif

ifeq ($(PLATFORM),ARM64)
  ARCH_FLAGS := -march=armv8.2-a -DPLATFORM_ARM64 -DUIOX_ARCH_ARM64
  CROSS      ?= aarch64-linux-gnu-
else ifeq ($(PLATFORM),ARM32)
  ARCH_FLAGS := -march=armv7-a -mfpu=neon -mfloat-abi=hard -DPLATFORM_ARM32
  CROSS      ?= arm-linux-gnueabihf-
else ifeq ($(PLATFORM),RV64)
  ARCH_FLAGS := -march=rv64gc -mabi=lp64d -DPLATFORM_RV64 -DUIOX_ARCH_RV64
  CROSS      ?= riscv64-linux-gnu-
else ifeq ($(PLATFORM),X86_64)
  # -march=native works with both GCC and Clang on the native host.
  # Use -march=x86_64 only when cross-compiling to a generic x86-64 target.
  ifeq ($(CROSS),)
    ARCH_FLAGS := -march=native -DPLATFORM_X86_64 -DUIOX_ARCH_X86_64
  else
    ARCH_FLAGS := -march=x86_64 -DPLATFORM_X86_64 -DUIOX_ARCH_X86_64
  endif
  CROSS      ?=
else
  $(error Unknown PLATFORM=$(PLATFORM). Use ARM64, ARM32, RV64, or X86_64)
endif

############### Platform detection and toolchain flags

CC      := $(CROSS)gcc
AR      := $(CROSS)ar
RANLIB  := $(CROSS)ranlib
SIZE    := $(CROSS)size
STRIP   := $(CROSS)strip
OBJDUMP := $(CROSS)objdump

BUILD ?= debug
ifeq ($(BUILD),debug)
  OPT_FLAGS  := -O0 -g3 -DDEBUG -DUIOX_DEBUG
  STRIP_FLAG :=
else ifeq ($(BUILD),release)
  OPT_FLAGS  := -O2 -DNDEBUG
  STRIP_FLAG := $(STRIP) --strip-unneeded
else
  $(error Unknown BUILD=$(BUILD). Use debug or release)
endif

SRCDIR   := src
INCDIR   := include
BUILDDIR := build/$(PLATFORM)/$(BUILD)
OBJDIR   := $(BUILDDIR)/obj
LIBDIR   := $(BUILDDIR)/lib
BINDIR   := $(BUILDDIR)/bin

PREFIX   ?= /usr/local
INST_INC := $(PREFIX)/include/uiox/cpu
INST_LIB := $(PREFIX)/lib
INST_BIN := $(PREFIX)/bin

LIB_SRCS := \
    $(SRCDIR)/uiox_cpu_hw.c      \
    $(SRCDIR)/uiox_cpu_buf.c     \
    $(SRCDIR)/uiox_cpu_if.c      \
    $(SRCDIR)/uiox_cpu_pm.c      \
    $(SRCDIR)/uiox_cpu_feat.c    \
    $(SRCDIR)/uiox_cpu_subsys.c  \
    $(SRCDIR)/uiox_cpu_device.c

DEMO_SRC  := $(SRCDIR)/uiox_cpu_demo.c

HEADERS   := \
    $(INCDIR)/uiox_cpu_hw.h      \
    $(INCDIR)/uiox_cpu_buf.h     \
    $(INCDIR)/uiox_cpu_if.h      \
    $(INCDIR)/uiox_cpu_pm.h      \
    $(INCDIR)/uiox_cpu_feat.h    \
    $(INCDIR)/uiox_cpu_subsys.h  \
    $(INCDIR)/uiox_cpu_device.h

LIB_OBJS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))
DEMO_OBJ  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(DEMO_SRC))

LIB_NAME  := libuiox_cpu.a
LIB_OUT   := $(LIBDIR)/$(LIB_NAME)
DEMO_BIN  := $(BINDIR)/uiox_cpu_demo

# here started new 
# Detect whether we are using Clang or GCC
CC_IS_CLANG := $(shell $(CC) --version 2>&1 | grep -c clang)

ifeq ($(PLATFORM),X86_64)
  ifeq ($(CC_IS_CLANG),1)
    # Clang accepts x86_64 (underscore) or just "native"
    ARCH_FLAGS := -march=native -DPLATFORM_X86_64 -DUIOX_ARCH_X86_64
  else
    # GCC accepts both x86-64 and x86_64
    ARCH_FLAGS := -march=native -DPLATFORM_X86_64 -DUIOX_ARCH_X86_64
  endif
endif
#here end new

CFLAGS := \
    -std=c11                              \
    -Wall -Wextra -Wpedantic              \
    -Wshadow -Wstrict-prototypes          \
    -Wmissing-prototypes                  \
    -Wdouble-promotion -Wformat=2 -Wundef \
    -ffunction-sections -fdata-sections   \
    -I$(INCDIR)                           \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH) \
    $(ARCH_FLAGS) $(OPT_FLAGS)

LDFLAGS := -Wl,--gc-sections -Wl,--as-needed
LDLIBS  := -L$(LIBDIR) -luiox_cpu

DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

.PHONY: all lib demo clean distclean install uninstall \
        size dump lint format docs tags info

all: lib demo
	$(Q)echo ""
	$(Q)echo "  Build complete [PLATFORM=$(PLATFORM)  BUILD=$(BUILD)  VERSION=$(VERSION)]"
	$(Q)echo "  Library : $(LIB_OUT)"
	$(Q)echo "  Binary  : $(DEMO_BIN)"
	$(Q)echo ""

$(OBJDIR) $(LIBDIR) $(BINDIR):
	$(Q)mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(LOGCC)
	$(Q)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(LIB_OBJS:.o=.d)
-include $(DEMO_OBJ:.o=.d)

lib: $(LIB_OUT)

$(LIB_OUT): $(LIB_OBJS) | $(LIBDIR)
	$(LOGAR)
	$(Q)$(AR) rcs $@ $^
	$(Q)$(RANLIB) $@
ifneq ($(STRIP_FLAG),)
	$(Q)$(STRIP_FLAG) $@
endif

demo: $(DEMO_BIN)

$(DEMO_BIN): $(DEMO_OBJ) $(LIB_OUT) | $(BINDIR)
	$(LOGLD)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@
ifneq ($(STRIP_FLAG),)
	$(Q)$(STRIP_FLAG) $@
endif

clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

distclean:
	$(Q)rm -rf build/
	$(Q)echo "  DISTCLEAN — removed build/"

install: lib demo
	$(Q)echo "  INSTALL -> $(PREFIX)"
	$(Q)install -d $(INST_INC) $(INST_LIB) $(INST_BIN)
	$(Q)install -m 644 $(HEADERS)  $(INST_INC)/
	$(Q)install -m 644 $(LIB_OUT)  $(INST_LIB)/
	$(Q)$(RANLIB) $(INST_LIB)/$(LIB_NAME)
	$(Q)install -m 755 $(DEMO_BIN) $(INST_BIN)/
	$(Q)echo "  INSTALL done"

uninstall:
	$(Q)echo "  UNINSTALL <- $(PREFIX)"
	$(Q)rm -f  $(INST_LIB)/$(LIB_NAME)
	$(Q)rm -f  $(INST_BIN)/uiox_cpu_demo
	$(Q)rm -rf $(INST_INC)
	$(Q)echo "  UNINSTALL done"

size: $(LIB_OUT) $(DEMO_BIN)
	$(Q)$(SIZE) --format=SysV $(LIB_OUT)
	$(Q)$(SIZE) $(DEMO_BIN)

dump: $(DEMO_BIN)
	$(Q)$(OBJDUMP) -d -S $(DEMO_BIN) > $(BUILDDIR)/uiox_cpu_demo.asm
	$(Q)echo "  DUMP -> $(BUILDDIR)/uiox_cpu_demo.asm"

lint:
	$(Q)cppcheck --enable=all --std=c11 --platform=unix64 \
	    --suppress=missingIncludeSystem -I $(INCDIR) $(SRCDIR)/

format:
	$(Q)clang-format -i --style=file $(SRCDIR)/*.c $(INCDIR)/*.h

docs:
	$(Q)doxygen Doxyfile

tags:
	$(Q)ctags -R --c-kinds=+defgmpstux $(SRCDIR) $(INCDIR)

info:
	$(Q)echo "  UIOX CPU Stack $(VERSION)  PLATFORM=$(PLATFORM)  BUILD=$(BUILD)"
	$(Q)echo "  CC=$(CC)  ARCH_FLAGS=$(ARCH_FLAGS)"
