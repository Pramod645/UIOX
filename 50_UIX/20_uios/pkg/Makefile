# UIOX Package Manager — Makefile
# Mirrors uiox-rtc / uiox-chg / uiox-als Makefile pattern exactly.

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
  Q :=
  LOGCC :=
  LOGAR :=
  LOGLD :=
  LOGCLEAN :=
endif

SRCDIR   := src
INCDIR   := include
BUILDDIR := build

# ── Compiler (native — package manager runs on the host kernel) ───────────────
CC     := gcc
AR     := ar
RANLIB := ranlib
SIZE   := size

BUILD ?= debug
ifeq ($(BUILD),debug)
  OPT_FLAGS  := -O0 -g3 -DDEBUG
else
  OPT_FLAGS  := -O2 -DNDEBUG
endif

OBJDIR := $(BUILDDIR)/obj
LIBDIR := $(BUILDDIR)/lib
BINDIR := $(BUILDDIR)/bin

CFLAGS := \
    -std=c11 -Wall -Wextra -Wpedantic \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes \
    -ffunction-sections -fdata-sections \
    -I$(INCDIR) \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH) \
    $(OPT_FLAGS)

LDFLAGS :=
LDLIBS  := -L$(LIBDIR) -luiox_pkg

DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

# ── Source files ──────────────────────────────────────────────────────────────
LIB_SRCS := \
    $(SRCDIR)/uiox_pkg_buf.c     \
    $(SRCDIR)/uiox_pkg_store.c   \
    $(SRCDIR)/uiox_pkg_resolve.c \
    $(SRCDIR)/uiox_pkg_subsys.c  \
    $(SRCDIR)/uiox_pkg_device.c

DEMO_SRC := $(SRCDIR)/uiox_pkg_demo.c

HEADERS := \
    $(INCDIR)/uiox_pkg_types.h   \
    $(INCDIR)/uiox_pkg_buf.h     \
    $(INCDIR)/uiox_pkg_store.h   \
    $(INCDIR)/uiox_pkg_resolve.h \
    $(INCDIR)/uiox_pkg_subsys.h  \
    $(INCDIR)/uiox_pkg_device.h

LIB_OBJS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))
DEMO_OBJ  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(DEMO_SRC))

LIB_NAME  := libuiox_pkg.a
LIB_OUT   := $(LIBDIR)/$(LIB_NAME)
DEMO_BIN  := $(BINDIR)/uiox_pkg_demo

PREFIX    ?= /usr/local
INST_INC  := $(PREFIX)/include/uiox/pkg
INST_LIB  := $(PREFIX)/lib
INST_BIN  := $(PREFIX)/bin

# ── Default target ────────────────────────────────────────────────────────────
.PHONY: all lib demo clean distclean install uninstall size help

all: lib demo
	$(Q)echo ""
	$(Q)echo "  ✓  Build complete [BUILD=$(BUILD)]"
	$(Q)echo "     Library : $(LIB_OUT)"
	$(Q)echo "     Binary  : $(DEMO_BIN)"

# ── Directory creation ────────────────────────────────────────────────────────
$(OBJDIR) $(LIBDIR) $(BINDIR):
	$(Q)mkdir -p $@

# ── Compile rule (with dependency tracking) ───────────────────────────────────
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(LOGCC)
	$(Q)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(LIB_OBJS:.o=.d)
-include $(DEMO_OBJ:.o=.d)

# ── Static library ────────────────────────────────────────────────────────────
lib: $(LIB_OUT)

$(LIB_OUT): $(LIB_OBJS) | $(LIBDIR)
	$(LOGAR)
	$(Q)$(AR) rcs $@ $^
	$(Q)$(RANLIB) $@

# ── Demo binary ───────────────────────────────────────────────────────────────
demo: $(DEMO_BIN)

$(DEMO_BIN): $(DEMO_OBJ) $(LIB_OUT) | $(BINDIR)
	$(LOGLD)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

# ── Run demo ──────────────────────────────────────────────────────────────────
.PHONY: run
run: demo
	$(Q)$(DEMO_BIN)

# ── Size report ───────────────────────────────────────────────────────────────
size: $(LIB_OUT) $(DEMO_BIN)
	$(Q)$(SIZE) $(LIB_OUT) $(DEMO_BIN)

# ── Install / uninstall ───────────────────────────────────────────────────────
install: lib demo
	$(Q)install -d $(INST_INC) $(INST_LIB) $(INST_BIN)
	$(Q)install -m 644 $(HEADERS) $(INST_INC)/
	$(Q)install -m 644 $(LIB_OUT) $(INST_LIB)/
	$(Q)$(RANLIB) $(INST_LIB)/$(LIB_NAME)
	$(Q)install -m 755 $(DEMO_BIN) $(INST_BIN)/

uninstall:
	$(Q)rm -f  $(INST_LIB)/$(LIB_NAME) $(INST_BIN)/uiox_pkg_demo
	$(Q)rm -rf $(INST_INC)

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)

distclean:
	$(Q)rm -rf build/

# ── Help ──────────────────────────────────────────────────────────────────────
help:
	@echo "UIOX Package Manager — Build System"
	@echo ""
	@echo "  make all              Build library + demo (default)"
	@echo "  make lib              Static library only"
	@echo "  make demo             Demo binary only"
	@echo "  make run              Build and run demo"
	@echo "  make size             Print section sizes"
	@echo "  make install          Install to PREFIX (default /usr/local)"
	@echo "  make uninstall        Remove installed files"
	@echo "  make clean            Remove build/ directory"
	@echo "  make BUILD=release    Release (optimised) build"
	@echo "  make V=1              Verbose build output"
