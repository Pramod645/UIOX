# UIOX Thermal Sensor Stack — Makefile
VERSION_MAJOR:=1
VERSION_MINOR:=0
VERSION_PATCH:=0
VERSION:=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
V ?= 0
ifeq ($(V),0)
  Q:=@
  LOGCC:=@echo "  CC      $<"
  LOGAR:=@echo "  AR      $@"
  LOGLD:=@echo "  LD      $@"
  LOGCLEAN:=@echo "  CLEAN"
else
  Q:= LOGCC:= LOGAR:= LOGLD:= LOGCLEAN:=
endif
ifndef PLATFORM
  MACHINE:=$(shell uname -m)
  ifeq ($(MACHINE),aarch64)
    PLATFORM:=ARM64
  else ifeq ($(MACHINE),armv7l)
    PLATFORM:=ARM32
  else
    PLATFORM:=X86_64
  endif
endif
ifeq ($(PLATFORM),ARM64)
  ARCH_FLAGS:=-march=armv8-a -DPLATFORM_ARM64
  CROSS?=aarch64-linux-gnu-
else ifeq ($(PLATFORM),ARM32)
  ARCH_FLAGS:=-march=armv7-a -mfpu=neon -mfloat-abi=hard -DPLATFORM_ARM32
  CROSS?=arm-linux-gnueabihf-
else
  ARCH_FLAGS:=-DPLATFORM_X86_64
  CROSS?=
endif
CC:=$(CROSS)gcc
AR:=$(CROSS)ar
RANLIB:=$(CROSS)ranlib
SIZE:=$(CROSS)size
STRIP:=$(CROSS)strip
BUILD?=debug
ifeq ($(BUILD),debug)
  OPT_FLAGS:=-O0 -g3 -DDEBUG
  STRIP_FLAG:=
else
  OPT_FLAGS:=-O2 -DNDEBUG
  STRIP_FLAG:=$(STRIP) --strip-unneeded
endif
SRCDIR:=src
INCDIR:=include
BUILDDIR:=build/$(PLATFORM)/$(BUILD)
OBJDIR:=$(BUILDDIR)/obj
LIBDIR:=$(BUILDDIR)/lib
BINDIR:=$(BUILDDIR)/bin
PREFIX?=/usr/local
INST_INC:=$(PREFIX)/include/uiox/thermal
INST_LIB:=$(PREFIX)/lib
INST_BIN:=$(PREFIX)/bin
LIB_SRCS:= \
    $(SRCDIR)/uiox_therm_hw.c      \
    $(SRCDIR)/uiox_therm_buf.c     \
    $(SRCDIR)/uiox_therm_if.c      \
    $(SRCDIR)/uiox_therm_sensor.c  \
    $(SRCDIR)/uiox_therm_policy.c  \
    $(SRCDIR)/uiox_therm_subsys.c  \
    $(SRCDIR)/uiox_therm_device.c
DEMO_SRC:=$(SRCDIR)/uiox_therm_demo.c
HEADERS:= \
    $(INCDIR)/uiox_therm_hw.h      \
    $(INCDIR)/uiox_therm_buf.h     \
    $(INCDIR)/uiox_therm_if.h      \
    $(INCDIR)/uiox_therm_sensor.h  \
    $(INCDIR)/uiox_therm_policy.h  \
    $(INCDIR)/uiox_therm_subsys.h  \
    $(INCDIR)/uiox_therm_device.h
LIB_OBJS:=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))
DEMO_OBJ:=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(DEMO_SRC))
LIB_NAME:=libuiox_therm.a
LIB_OUT:=$(LIBDIR)/$(LIB_NAME)
DEMO_BIN:=$(BINDIR)/uiox_therm_demo
CFLAGS:= \
    -std=c11 -Wall -Wextra -Wpedantic \
    -Wshadow -Wstrict-prototypes -Wmissing-prototypes \
    -ffunction-sections -fdata-sections \
    -I$(INCDIR) \
    -DUIOX_VERSION_MAJOR=$(VERSION_MAJOR) \
    -DUIOX_VERSION_MINOR=$(VERSION_MINOR) \
    -DUIOX_VERSION_PATCH=$(VERSION_PATCH) \
    $(ARCH_FLAGS) $(OPT_FLAGS)
LDFLAGS:=
LDLIBS:=-L$(LIBDIR) -luiox_therm -lm
DEPFLAGS=-MT $@ -MMD -MP -MF $(OBJDIR)/$*.d
.PHONY: all lib demo clean distclean install uninstall size lint format tags
all: lib demo
	$(Q)echo "  Build complete [PLATFORM=$(PLATFORM) BUILD=$(BUILD)]"
	$(Q)echo "  Library : $(LIB_OUT)"
	$(Q)echo "  Binary  : $(DEMO_BIN)"
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
clean:
	$(LOGCLEAN)
	$(Q)rm -rf $(BUILDDIR)
distclean:
	$(Q)rm -rf build/
install: lib demo
	$(Q)install -d $(INST_INC) $(INST_LIB) $(INST_BIN)
	$(Q)install -m 644 $(HEADERS) $(INST_INC)/
	$(Q)install -m 644 $(LIB_OUT) $(INST_LIB)/
	$(Q)$(RANLIB) $(INST_LIB)/$(LIB_NAME)
	$(Q)install -m 755 $(DEMO_BIN) $(INST_BIN)/
uninstall:
	$(Q)rm -f $(INST_LIB)/$(LIB_NAME) $(INST_BIN)/uiox_therm_demo
	$(Q)rm -rf $(INST_INC)
size: $(LIB_OUT) $(DEMO_BIN)
	$(Q)$(SIZE) $(LIB_OUT) $(DEMO_BIN)
lint:
	$(Q)cppcheck --enable=all --std=c11 \
	    --suppress=missingIncludeSystem -I $(INCDIR) $(SRCDIR)/
format:
	$(Q)clang-format -i --style=file $(SRCDIR)/*.c $(INCDIR)/*.h
tags:
	$(Q)ctags -R --c-kinds=+defgmpstux $(SRCDIR) $(INCDIR)
