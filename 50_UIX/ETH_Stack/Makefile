# =============================================================================
# UIOX Networking Stack — Makefile
#
# Targets:
#   all          Build the demo binary (default)
#   lib          Build libuiox_net.a static library only
#   demo         Build the end-to-end demo binary
#   clean        Remove all build artefacts
#   distclean    Remove build directory entirely
#   install      Install library + headers to PREFIX
#   uninstall    Remove installed files
#   docs         Generate Doxygen HTML documentation
#   format       Run clang-format over all sources
#   lint         Run cppcheck static analysis
#   tags         Generate ctags index
#   info         Print build configuration summary
#
# Configuration variables (override on command line):
#   PLATFORM     ARM64 | ARM32 | X86_64 (default: auto-detect)
#   BUILD        debug | release         (default: debug)
#   PREFIX       Install prefix          (default: /usr/local)
#   CROSS        Cross-compiler prefix   (default: empty = native)
#   V            1 = verbose output      (default: 0 = quiet)
#
# Examples:
#   make                                 # Native debug build
#   make BUILD=release                   # Optimised release build
#   make PLATFORM=ARM64 CROSS=aarch64-linux-gnu-  # Cross-compile for ARM64
#   make PLATFORM=ARM32 CROSS=arm-linux-gnueabihf- # Cross-compile for ARM32
#   make install PREFIX=/opt/uiox        # Install to custom prefix
#   make V=1                             # Verbose build output
# =============================================================================

# -----------------------------------------------------------------------------
# Version
# -----------------------------------------------------------------------------

#build/
#└── ARM64/               ← or X86_64 / ARM32
#    └── debug/           ← or release
#        ├── obj/
#        │   ├── uiox_net_hw.o
#        │   ├── uiox_net_hw.d     ← auto-dependency
#        │   ├── uiox_netbuf.o
#        │   ├── uiox_netif.o
#        │   ├── uiox_proto.o
#        │   ├── uiox_socket.o
#        │   ├── uiox_connectivity.o
#        │   └── uiox_net_demo.o
#        ├── lib/
#        │   └── libuiox_net.a
#        └── bin/
#            └── uiox_net_demo



VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0
VERSION       := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# -----------------------------------------------------------------------------
# Verbosity control
# V=0 (default): short pretty output   V=1: full commands
# -----------------------------------------------------------------------------

V ?= 0
ifeq ($(V),0)
  Q        := @
  LOGCC    := @echo "  CC     $<"
  LOGAR    := @echo "  AR     $@"
  LOGLD    := @echo "  LD     $@"
  LOGCLEAN := @echo "  CLEAN"
else
  Q        :=
  LOGCC    :=
  LOGAR    :=
  LOGLD    :=
  LOGCLEAN :=
endif

# -----------------------------------------------------------------------------
# Platform detection / selection
# -----------------------------------------------------------------------------

ifndef PLATFORM
  MACHINE := $(shell uname -m)
  ifeq ($(MACHINE),aarch64)
    PLATFORM := ARM64
  else ifeq ($(MACHINE),armv7l)
    PLATFORM := ARM32
  else
    PLATFORM := X86_64
  endif
endif

ifeq ($(PLATFORM),ARM64)
  ARCH_FLAGS := -march=armv8-a -DPLATFORM_ARM64
  CROSS      ?= aarch64-linux-gnu-
else ifeq ($(PLATFORM),ARM32)
  ARCH_FLAGS := -march=armv7-a -mfpu=neon -mfloat-abi=hard -DPLATFORM_ARM32
  CROSS      ?= arm-linux-gnueabihf-
else ifeq ($(PLATFORM),X86_64)
  #ARCH_FLAGS := -march=x86-64 -DPLATFORM_X86_64
  ARCH_FLAGS := -mcpu=apple-m1 -DPLATFORM_X86_64
  CROSS      ?=
else
  $(error Unknown PLATFORM=$(PLATFORM). Use ARM64, ARM32, or X86_64)
endif

# -----------------------------------------------------------------------------
# Toolchain
# -----------------------------------------------------------------------------

CC      := $(CROSS)gcc
AR      := $(CROSS)ar
RANLIB  := $(CROSS)ranlib
SIZE    := $(CROSS)size
STRIP   := $(CROSS)strip
OBJDUMP := $(CROSS)objdump

# -----------------------------------------------------------------------------
# Build mode
# -----------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------
# Directories
# -----------------------------------------------------------------------------

SRCDIR   := src
INCDIR   := include
BUILDDIR := build/$(PLATFORM)/$(BUILD)
OBJDIR   := $(BUILDDIR)/obj
LIBDIR   := $(BUILDDIR)/lib
BINDIR   := $(BUILDDIR)/bin

PREFIX   ?= /usr/local
INST_INC := $(PREFIX)/include/uiox
INST_LIB := $(PREFIX)/lib
INST_BIN := $(PREFIX)/bin

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

# Library sources (all layers except demo)
LIB_SRCS := \
    $(SRCDIR)/uiox_net_hw.c       \
    $(SRCDIR)/uiox_netbuf.c       \
    $(SRCDIR)/uiox_netif.c        \
    $(SRCDIR)/uiox_proto.c        \
    $(SRCDIR)/uiox_socket.c       \
    $(SRCDIR)/uiox_connectivity.c

# Demo application source
DEMO_SRC := $(SRCDIR)/uiox_net_demo.c

# Public headers
HEADERS := \
    $(INCDIR)/uiox_net_hw.h       \
    $(INCDIR)/uiox_netbuf.h       \
    $(INCDIR)/uiox_netif.h        \
    $(INCDIR)/uiox_proto.h        \
    $(INCDIR)/uiox_socket.h       \
    $(INCDIR)/uiox_connectivity.h

# Derive object file lists
LIB_OBJS  := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(LIB_SRCS))
DEMO_OBJ  := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(DEMO_SRC))

# Static library and demo binary names
LIB_NAME  := libuiox_net.a
LIB_OUT   := $(LIBDIR)/$(LIB_NAME)
DEMO_BIN  := $(BINDIR)/uiox_net_demo

# -----------------------------------------------------------------------------
# Flags
# -----------------------------------------------------------------------------

CFLAGS := \
    -std=c11                        \
    -Wall                           \
    -Wextra                         \
    -Wpedantic                      \
    -Wshadow                        \
    -Wstrict-prototypes             \
    -Wmissing-prototypes            \
    -Wdouble-promotion              \
    -Wformat=2                      \
    -Wundef                         \
    -ffunction-sections             \
    -fdata-sections                 \
    -I$(INCDIR)                     \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH) \
    $(ARCH_FLAGS)                   \
    $(OPT_FLAGS)

# Linker flags
LDFLAGS := \
#    -Wl,--gc-sections               #\
#    -Wl,--as-needed

# Libraries to link into demo
LDLIBS := -L$(LIBDIR) -luiox_net

# Dependency flags (auto-generate .d files)
DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

# -----------------------------------------------------------------------------
# Phony targets
# -----------------------------------------------------------------------------

.PHONY: all lib demo clean distclean install uninstall \
        docs format lint tags info size dump

# -----------------------------------------------------------------------------
# Default target
# -----------------------------------------------------------------------------

all: lib demo
	$(Q)echo ""
	$(Q)echo "  Build complete  [PLATFORM=$(PLATFORM)  BUILD=$(BUILD)  VERSION=$(VERSION)]"
	$(Q)echo "  Library : $(LIB_OUT)"
	$(Q)echo "  Binary  : $(DEMO_BIN)"
	$(Q)echo ""

# -----------------------------------------------------------------------------
# Create output directories
# -----------------------------------------------------------------------------

$(OBJDIR) $(LIBDIR) $(BINDIR):
	$(Q)mkdir -p $@

# -----------------------------------------------------------------------------
# Compile rule — all .c → .o
# -----------------------------------------------------------------------------

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(LOGCC)
	$(Q)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Include generated dependency files (suppress error on first build)
# -----------------------------------------------------------------------------

-include $(LIB_OBJS:.o=.d)
-include $(DEMO_OBJ:.o=.d)

# -----------------------------------------------------------------------------
# Static library
# -----------------------------------------------------------------------------

lib: $(LIB_OUT)

$(LIB_OUT): $(LIB_OBJS) | $(LIBDIR)
	$(LOGAR)
	$(Q)$(AR) rcs $@ $^
	$(Q)$(RANLIB) $@
ifneq ($(STRIP_FLAG),)
	$(Q)$(STRIP_FLAG) $@
endif

# -----------------------------------------------------------------------------
# Demo binary
# -----------------------------------------------------------------------------

demo: $(DEMO_BIN)

$(DEMO_BIN): $(DEMO_OBJ) $(LIB_OUT) | $(BINDIR)
	$(LOGLD)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@
ifneq ($(STRIP_FLAG),)
	$(Q)$(STRIP_FLAG) $@
endif

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------

clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

distclean:
	$(Q)rm -rf build/
	$(Q)echo "  DISTCLEAN  — removed build/"

# -----------------------------------------------------------------------------
# Install / Uninstall
# -----------------------------------------------------------------------------

install: lib
	$(Q)echo "  INSTALL  -> $(PREFIX)"
	$(Q)install -d $(INST_INC) $(INST_LIB) $(INST_BIN)
	$(Q)install -m 644 $(HEADERS)  $(INST_INC)/
	$(Q)install -m 644 $(LIB_OUT)  $(INST_LIB)/
	$(Q)$(RANLIB) $(INST_LIB)/$(LIB_NAME)
	$(Q)install -m 755 $(DEMO_BIN) $(INST_BIN)/
	$(Q)echo "  INSTALL  done"

uninstall:
	$(Q)echo "  UNINSTALL <- $(PREFIX)"
	$(Q)rm -f  $(INST_LIB)/$(LIB_NAME)
	$(Q)rm -f  $(INST_BIN)/uiox_net_demo
	$(Q)rm -rf $(INST_INC)
	$(Q)echo "  UNINSTALL done"

# -----------------------------------------------------------------------------
# Size report — shows text/data/bss breakdown per object file
# -----------------------------------------------------------------------------

size: $(LIB_OUT) $(DEMO_BIN)
	$(Q)echo ""
	$(Q)echo "=== Library section sizes ==="
	$(Q)$(SIZE) --format=SysV $(LIB_OUT)
	$(Q)echo ""
	$(Q)echo "=== Demo binary sizes ==="
	$(Q)$(SIZE) $(DEMO_BIN)

# -----------------------------------------------------------------------------
# Disassembly dump
# -----------------------------------------------------------------------------

dump: $(DEMO_BIN)
	$(Q)$(OBJDUMP) -d -S $(DEMO_BIN) > $(BUILDDIR)/uiox_net_demo.asm
	$(Q)echo "  DUMP  -> $(BUILDDIR)/uiox_net_demo.asm"

# -----------------------------------------------------------------------------
# Static analysis — cppcheck
# -----------------------------------------------------------------------------

lint:
	$(Q)echo "  LINT"
	$(Q)cppcheck \
	    --enable=all              \
	    --std=c11                 \
	    --platform=unix64         \
	    --suppress=missingIncludeSystem \
	    -I $(INCDIR)              \
	    $(SRCDIR)/

# -----------------------------------------------------------------------------
# Code formatting — clang-format
# -----------------------------------------------------------------------------

format:
	$(Q)echo "  FORMAT"
	$(Q)clang-format -i --style=file \
	    $(SRCDIR)/*.c $(INCDIR)/*.h
	$(Q)echo "  FORMAT done"

# -----------------------------------------------------------------------------
# Documentation — Doxygen
# -----------------------------------------------------------------------------

docs:
	$(Q)echo "  DOCS"
	$(Q)doxygen Doxyfile
	$(Q)echo "  DOCS  -> docs/html/index.html"

# -----------------------------------------------------------------------------
# ctags index
# -----------------------------------------------------------------------------

tags:
	$(Q)ctags -R --c-kinds=+defgmpstux $(SRCDIR) $(INCDIR)
	$(Q)echo "  TAGS  -> tags"

# -----------------------------------------------------------------------------
# Build info summary
# -----------------------------------------------------------------------------

info:
	$(Q)echo ""
	$(Q)echo "  UIOX Networking Stack $(VERSION)"
	$(Q)echo "  ─────────────────────────────────────"
	$(Q)echo "  PLATFORM  : $(PLATFORM)"
	$(Q)echo "  BUILD     : $(BUILD)"
	$(Q)echo "  CC        : $(CC)"
	$(Q)echo "  AR        : $(AR)"
	$(Q)echo "  CFLAGS    : $(CFLAGS)"
	$(Q)echo "  LDFLAGS   : $(LDFLAGS)"
	$(Q)echo "  BUILDDIR  : $(BUILDDIR)"
	$(Q)echo "  PREFIX    : $(PREFIX)"
	$(Q)echo ""
	$(Q)echo "  Library sources:"
	$(Q)$(foreach s,$(LIB_SRCS),echo "    $(s)";)
	$(Q)echo ""
	$(Q)echo "  Headers:"
	$(Q)$(foreach h,$(HEADERS),echo "    $(h)";)
	$(Q)echo ""
