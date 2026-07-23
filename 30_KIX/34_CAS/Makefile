# =============================================================================
# 30_KIX/34_CAS/Makefile
#
# UIOX Kernel — Communication Abstraction Subsystem (CAS).
#
# Directory layout:
#   34_CAS/
#   ├── include/          public headers
#   ├── src/              top-level implementation (*.c)
#   ├── 01_Com/           communication primitives (sockets, pipes, channels)
#   ├── 02_Sensors/       sensor abstraction interfaces
#   └── 03_NonSecnsors/   non-sensor peripheral interfaces
#
# The umbrella Makefile calls:
#   make -C 34_CAS CC=<cross> AR=<ar> CFLAGS="…" static
#
# @version 1.0.0
# @date    2026-07-23
# =============================================================================

CC      ?= gcc
AR      ?= ar
CFLAGS  ?= -Wall -Wextra -O2 -std=c11
ARFLAGS := rcs

SRC_DIR := src
OBJ_DIR := obj
LIB_DIR := lib

SRCS    := $(wildcard $(SRC_DIR)/*.c)            \
           $(wildcard 01_Com/src/*.c)             \
           $(wildcard 02_Sensors/src/*.c)         \
           $(wildcard 03_NonSecnsors/src/*.c)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

STATIC  := $(LIB_DIR)/libuiox_cas.a
SHARED  := $(LIB_DIR)/libuiox_cas.so

CFLAGS  += -Iinclude         \
           -I01_Com/include  \
           -I02_Sensors/include \
           -I03_NonSecnsors/include

.PHONY: all static shared clean

all: static shared

static: $(STATIC)
shared: $(SHARED)

$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "  [AR]  $@"

$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC -o $@ $^
	@echo "  [SO]  $@"

vpath %.c $(SRC_DIR)        \
          01_Com/src         \
          02_Sensors/src     \
          03_NonSecnsors/src

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "  [CC]  $<"
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
