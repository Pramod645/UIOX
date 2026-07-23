# =============================================================================
# 30_KIX/32_FileSystem/Makefile
#
# UIOX Kernel — File System subsystem.
#
# Directory layout:
#   32_FileSystem/
#   ├── include/          public headers  (Filesystems.h, VirtualFileSystem.h …)
#   ├── src/              implementation  (*.c)
#   ├── 01_fsa/           File-System Abstraction layer
#   └── 10_scfs/          Simple Contiguous File System
#
# Targets:
#   make                  build static + shared libraries (default)
#   make static           build lib/libuiox_fs.a only
#   make shared           build lib/libuiox_fs.so only
#   make ARCH=arm64 CC=aarch64-elf-gcc    cross-compile
#   make ARCH=arm32 CC=arm-none-eabi-gcc
#   make ARCH=riscv64 CC=riscv64-elf-gcc
#   make ARCH=x86_64 CC=x86_64-elf-gcc
#   make clean
#
# Called by the umbrella 30_KIX/Makefile with:
#   make -C 32_FileSystem CC=<cross> AR=<ar> CFLAGS="<arch-flags>" static
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

SRCS    := $(wildcard $(SRC_DIR)/*.c) \
           $(wildcard 01_fsa/src/*.c)  \
           $(wildcard 10_scfs/src/*.c)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

STATIC  := $(LIB_DIR)/libuiox_fs.a
SHARED  := $(LIB_DIR)/libuiox_fs.so

# Include paths: own headers + adjacent subsystems
CFLAGS  += -Iinclude -I01_fsa/include -I10_scfs/include

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

# Pattern rule — find source files via vpath
vpath %.c $(SRC_DIR) 01_fsa/src 10_scfs/src

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "  [CC]  $<"
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
