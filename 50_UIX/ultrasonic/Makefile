#uiox-ultrasonic/
#├── include/
#│   ├── uiox_us_hw.h          # Layer 1  — HAL: timer, GPIO, DMA, IRQ
#│   ├── uiox_us_buf.h         # Buffer pool for echo sample frames
#│   ├── uiox_us_if.h          # Layer 2  — Interface driver (GPIO/SPI/UART)
#│   ├── uiox_us_sensor.h      # Sensor abstraction (pulse config, reg map)
#│   ├── uiox_us_dsp.h         # Layer 3  — DSP: envelope, ToF, temp comp
#│   ├── uiox_us_subsys.h      # Layer 4  — Pipeline, zones, multi-sensor
#│   └── uiox_us_device.h      # Layer 5  — Application-facing API
#└── src/
#    ├── uiox_us_hw.c
#    ├── uiox_us_buf.c
#    ├── uiox_us_if.c
#    ├── uiox_us_sensor.c
#    ├── uiox_us_dsp.c
#    ├── uiox_us_subsys.c
#    ├── uiox_us_device.c
#    └── uiox_us_demo.c

# =============================================================================
# UIOX Ultrasonic Sensor Stack — Makefile
#
# Targets:
#   all          Build demo binary (default)
#   lib          Build libuiox_us.a static library only
#   demo         Build demo binary (depends on lib)
#   clean        Remove current platform/mode build artefacts
#   distclean    Remove entire build/ tree
#   install      Install headers + library + binary to PREFIX
#   uninstall    Remove installed files
#   size         Print section-size breakdown
#   dump         Generate disassembly of demo binary
#   lint         Run cppcheck static analysis
#   format       Run clang-format over all sources
#   docs         Generate Doxygen HTML
#   tags         Generate ctags index
#   info         Print build configuration summary
#
# Configuration (override on command line):
#   PLATFORM     ARM64 | ARM32 | X86_64   (default: auto-detect)
#   BUILD        debug | release           (default: debug)
#   PREFIX       install prefix            (default: /usr/local)
#   CROSS        cross-compiler prefix     (default: empty = native)
#   V            1 = verbose               (default: 0 = quiet)
#
# Examples:
#   make
#   make BUILD=release
#   make PLATFORM=ARM64 CROSS=aarch64-linux-gnu-
#   make PLATFORM=ARM32 CROSS=arm-linux-gnueabihf-
#   make install PREFIX=/opt/uiox
#   make V=1
# =============================================================================

# -----------------------------------------------------------------------------
# Version
# -----------------------------------------------------------------------------

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0
VERSION       := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# -----------------------------------------------------------------------------
# Verbosity
# -----------------------------------------------------------------------------

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
# Platform detection
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
  ARCH_FLAGS := -march=x86-64 -DPLATFORM_X86_64
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
INST_INC := $(PREFIX)/include/uiox/ultrasonic
INST_LIB := $(PREFIX)/lib
INST_BIN := $(PREFIX)/bin

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

LIB_SRCS := \
    $(SRCDIR)/uiox_us_hw.c      \
    $(SRCDIR)/uiox_us_buf.c     \
    $(SRCDIR)/uiox_us_if.c      \
    $(SRCDIR)/uiox_us_sensor.c  \
    $(SRCDIR)/uiox_us_dsp.c     \
    $(SRCDIR)/uiox_us_subsys.c  \
    $(SRCDIR)/uiox_us_device.c

DEMO_SRC  := $(SRCDIR)/uiox_us_demo.c

HEADERS   := \
    $(INCDIR)/uiox_us_hw.h      \
    $(INCDIR)/uiox_us_buf.h     \
    $(INCDIR)/uiox_us_if.h      \
    $(INCDIR)/uiox_us_sensor.h  \
    $(INCDIR)/uiox_us_dsp.h     \
    $(INCDIR)/uiox_us_subsys.h  \
    $(INCDIR)/uiox_us_device.h

LIB_OBJS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))
DEMO_OBJ  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(DEMO_SRC))

LIB_NAME  := libuiox_us.a
LIB_OUT   := $(LIBDIR)/$(LIB_NAME)
DEMO_BIN  := $(BINDIR)/uiox_us_demo

# -----------------------------------------------------------------------------
# Compiler flags
# -----------------------------------------------------------------------------

CFLAGS := \
    -std=c11                              \
    -Wall                                 \
    -Wextra                               \
    -Wpedantic                            \
    -Wshadow                              \
    -Wstrict-prototypes                   \
    -Wmissing-prototypes                  \
    -Wdouble-promotion                    \
    -Wformat=2                            \
    -Wundef                               \
    -ffunction-sections                   \
    -fdata-sections                       \
    -I$(INCDIR)                           \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH) \
    $(ARCH_FLAGS)                         \
    $(OPT_FLAGS)

LDFLAGS := \
    -Wl,--gc-sections                     \
    -Wl,--as-needed

# DSP layer needs libm
LDLIBS  := -L$(LIBDIR) -luiox_us -lm

DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

# -----------------------------------------------------------------------------
# Phony targets
# -----------------------------------------------------------------------------

.PHONY: all lib demo clean distclean install uninstall \
        size dump lint format docs tags info

# -----------------------------------------------------------------------------
# Default target
# -----------------------------------------------------------------------------

all: lib demo
	$(Q)echo ""
	$(Q)echo "  Build complete [PLATFORM=$(PLATFORM)  BUILD=$(BUILD)  VERSION=$(VERSION)]"
	$(Q)echo "  Library : $(LIB_OUT)"
	$(Q)echo "  Binary  : $(DEMO_BIN)"
	$(Q)echo ""

# -----------------------------------------------------------------------------
# Output directories
# -----------------------------------------------------------------------------

$(OBJDIR) $(LIBDIR) $(BINDIR):
	$(Q)mkdir -p $@

# -----------------------------------------------------------------------------
# Compile rule
# -----------------------------------------------------------------------------

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(LOGCC)
	$(Q)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Auto-dependency includes
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
	$(Q)echo "  DISTCLEAN — removed build/"

# -----------------------------------------------------------------------------
# Install / Uninstall
# -----------------------------------------------------------------------------

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
	$(Q)rm -f  $(INST_BIN)/uiox_us_demo
	$(Q)rm -rf $(INST_INC)
	$(Q)echo "  UNINSTALL done"

# -----------------------------------------------------------------------------
# Size report
# -----------------------------------------------------------------------------

size: $(LIB_OUT) $(DEMO_BIN)
	$(Q)echo ""
	$(Q)echo "=== Library section sizes ==="
	$(Q)$(SIZE) --format=SysV $(LIB_OUT)
	$(Q)echo ""
	$(Q)echo "=== Demo binary sizes ==="
	$(Q)$(SIZE) $(DEMO_BIN)

# -----------------------------------------------------------------------------
# Disassembly
# -----------------------------------------------------------------------------

dump: $(DEMO_BIN)
	$(Q)$(OBJDUMP) -d -S $(DEMO_BIN) > $(BUILDDIR)/uiox_us_demo.asm
	$(Q)echo "  DUMP -> $(BUILDDIR)/uiox_us_demo.asm"

# -----------------------------------------------------------------------------
# Static analysis
# -----------------------------------------------------------------------------

lint:
	$(Q)echo "  LINT"
	$(Q)cppcheck                          \
	    --enable=all                      \
	    --std=c11                         \
	    --platform=unix64                 \
	    --suppress=missingIncludeSystem   \
	    -I $(INCDIR)                      \
	    $(SRCDIR)/

# -----------------------------------------------------------------------------
# Code formatting
# -----------------------------------------------------------------------------

format:
	$(Q)echo "  FORMAT"
	$(Q)clang-format -i --style=file      \
	    $(SRCDIR)/*.c $(INCDIR)/*.h
	$(Q)echo "  FORMAT done"

# -----------------------------------------------------------------------------
# Doxygen
# -----------------------------------------------------------------------------

docs:
	$(Q)echo "  DOCS"
	$(Q)doxygen Doxyfile
	$(Q)echo "  DOCS -> docs/html/index.html"

# -----------------------------------------------------------------------------
# ctags
# -----------------------------------------------------------------------------

tags:
	$(Q)ctags -R --c-kinds=+defgmpstux $(SRCDIR) $(INCDIR)
	$(Q)echo "  TAGS -> tags"

# -----------------------------------------------------------------------------
# Build info summary
# -----------------------------------------------------------------------------

info:
	$(Q)echo ""
	$(Q)echo "  UIOX Ultrasonic Sensor Stack $(VERSION)"
	$(Q)echo "  ─────────────────────────────────────────"
	$(Q)echo "  PLATFORM  : $(PLATFORM)"
	$(Q)echo "  BUILD     : $(BUILD)"
	$(Q)echo "  CC        : $(CC)"
	$(Q)echo "  AR        : $(AR)"
	$(Q)echo "  CFLAGS    : $(CFLAGS)"
	$(Q)echo "  LDFLAGS   : $(LDFLAGS)"
	$(Q)echo "  LDLIBS    : $(LDLIBS)"
	$(Q)echo "  BUILDDIR  : $(BUILDDIR)"
	$(Q)echo "  PREFIX    : $(PREFIX)"
	$(Q)echo ""
	$(Q)echo "  Library sources:"
	$(Q)$(foreach s,$(LIB_SRCS),echo "    $(s)";)
	$(Q)echo ""
	$(Q)echo "  Headers:"
	$(Q)$(foreach h,$(HEADERS),echo "    $(h)";)
	$(Q)echo ""

