#uiox_proc/
#├── include/
#│   ├── process.h
#│   ├── region.h
#│   ├── context.h
#│   └── proc_algo.h
#├── src/
#│   ├── process.c
#│   ├── region.c
#│   ├── context.c
#│   ├── sleep_wakeup.c
#│   └── syscall.c
#└── Makefile
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -Iinclude
AR      := ar
ARFLAGS := rcs

SRC_DIR := src
OBJ_DIR := obj
LIB_DIR := lib

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

STATIC  := $(LIB_DIR)/libuioxproc.a
SHARED  := $(LIB_DIR)/libuioxproc.so

.PHONY: all static shared clean

all: static shared

static: $(STATIC)

shared: $(SHARED)

$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built: $@"

$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC -o $@ $^
	@echo "Built: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
