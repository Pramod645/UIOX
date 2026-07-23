# =============================================================================
# 30_KIX/33_ProcessControlSubsystem/Makefile
#
# UIOX Kernel — Process Control Subsystem (PCS).
#
# Directory layout:
#   33_ProcessControlSubsystem/
#   ├── include/                  public headers
#   ├── src/                      top-level implementation (*.c)
#   ├── 00_inter-process-communication/   IPC: pipes, signals, semaphores
#   ├── 01_schedular/             CPU scheduler (CFS / round-robin)
#   ├── 02_memory-managment/      virtual memory, page tables, mmap
#   ├── 40_procStruct/            process/thread descriptor tables
#   └── 50_scps/                  syscall dispatch table
#
# The umbrella Makefile calls:
#   make -C 33_ProcessControlSubsystem CC=<cross> AR=<ar> CFLAGS="…" static
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

SRCS    := $(wildcard $(SRC_DIR)/*.c)                               \
           $(wildcard 00_inter-process-communication/src/*.c)        \
           $(wildcard 01_schedular/src/*.c)                          \
           $(wildcard 02_memory-managment/src/*.c)                   \
           $(wildcard 40_procStruct/src/*.c)                         \
           $(wildcard 50_scps/src/*.c)
OBJS    := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS)))

STATIC  := $(LIB_DIR)/libuiox_pcs.a
SHARED  := $(LIB_DIR)/libuiox_pcs.so

CFLAGS  += -Iinclude \
           -I00_inter-process-communication/include \
           -I01_schedular/include                   \
           -I02_memory-managment/include            \
           -I40_procStruct/include                  \
           -I50_scps/include

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

vpath %.c $(SRC_DIR)                             \
          00_inter-process-communication/src     \
          01_schedular/src                       \
          02_memory-managment/src                \
          40_procStruct/src                      \
          50_scps/src

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "  [CC]  $<"
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
