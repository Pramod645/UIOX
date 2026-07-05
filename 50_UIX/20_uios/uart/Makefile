# UIOX UART Stack — Makefile

VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

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

CC     := gcc
AR     := ar
RANLIB := ranlib

BUILD ?= debug
ifeq ($(BUILD),debug)
  OPT_FLAGS := -O0 -g3 -DDEBUG
else
  OPT_FLAGS := -O2 -DNDEBUG
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
LDLIBS  := -L$(LIBDIR) -luiox_uart

DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d

LIB_SRCS := \
    $(SRCDIR)/uiox_uart_hw.c      \
    $(SRCDIR)/uiox_uart_buf.c     \
    $(SRCDIR)/uiox_uart_if.c      \
    $(SRCDIR)/uiox_uart_proto.c   \
    $(SRCDIR)/uiox_uart_subsys.c  \
    $(SRCDIR)/uiox_uart_device.c

DEMO_SRC  := $(SRCDIR)/uiox_uart_demo.c
LIB_OBJS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))
DEMO_OBJ  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(DEMO_SRC))

LIB_NAME  := libuiox_uart.a
LIB_OUT   := $(LIBDIR)/$(LIB_NAME)
DEMO_BIN  := $(BINDIR)/uiox_uart_demo

.PHONY: all lib demo clean run size

all: lib demo
	$(Q)echo "  ✓  Build complete [BUILD=$(BUILD)]"
	$(Q)echo "     Library : $(LIB_OUT)"
	$(Q)echo "     Binary  : $(DEMO_BIN)"

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

demo: $(DEMO_BIN)
$(DEMO_BIN): $(DEMO_OBJ) $(LIB_OUT) | $(BINDIR)
	$(LOGLD)
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

run: demo
	$(Q)$(DEMO_BIN)

size: $(LIB_OUT) $(DEMO_BIN)
	$(Q)size $(LIB_OUT) $(DEMO_BIN)

clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)
