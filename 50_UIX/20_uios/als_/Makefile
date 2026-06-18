# UIOX ALS — Ambient Light Sensor Stack — Makefile

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
else
  ARCH_FLAGS := -DPLATFORM_X86_64
  CROSS      ?=
endif

CC     := $(CROSS)gcc
AR     := $(CROSS)ar
RANLIB := $(CROSS)ranlib
SIZE   := $(CROSS)size
STRIP  := $(CROSS)strip

BUILD ?= debug
ifeq ($(BUILD),debug)
  OPT_FLAGS  := -O0 -g3 -DDEBUG
  STRIP_FLAG :=
else
  OPT_FLAGS  := -O2 -DNDEBUG
  STRIP_FLAG := $(STRIP) --strip-unneeded
endif

SRCDIR   := src
INCDIR   := include
BUILDDIR := build/$(PLATFORM)/$(BUILD)
OBJDIR   := $(BUILDDIR)/obj
LIBDIR   := $(BUILDDIR)/lib
BINDIR   := $(BUILDDIR)/bin

PREFIX   ?= /usr/local
INST_INC := $(PREFIX)/include/uiox/als
INST_LIB := $(PREFIX)/lib
INST_BIN := $(PREFIX)/bin

LIB_SRCS := \
    $(SRCDIR)/uiox_als_hw.c      \
    $(SRCDIR)/uiox_als_buf.c     \
    $(SRCDIR)/uiox_als_if.c      \
    $(SRCDIR)/uiox_als_cal.c     \
    $(SRCDIR)/uiox_als_subsys.c  \
    $(SRCDIR)/uiox_als_device.c

DEMO_SRC := $(SRCDIR)/uiox_als_demo.c

HEADERS := \
    $(INCDIR)/uiox_als_hw.h      \
    $(INCDIR)/uiox_als_buf.h     \
    $(INCDIR)/uiox_als_if.h      \
    $(INCDIR)/uiox_als_cal.h     \
    $(INCDIR)/uiox_als_subsys.h  \
    $(INCDIR)/uiox_als_device.h

LIB_OBJS  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SRCS))
DEMO_OBJ  := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(DEMO_SRC))

LIB_NAME  := libuiox_als.a
LIB_OUT   := $(LIBDIR)/$(LIB_NAME)
DEMO_BIN  := $(BINDIR)/uiox_als_demo

CFLAGS := \
    -std
