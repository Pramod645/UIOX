#uiox_sched_mm/
#├── include/
#│   ├── scheduler.h
#│   ├── clock.h
#│   ├── swapper.h
#│   └── page_fault.h
#├── src/
#│   ├── scheduler.c
#│   ├── clock.c
#│   ├── swapper.c
#│   ├── page_fault.c
#│   └── main.c
#└── Makefile
CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -Iinclude
OBJ_DIR := obj
LIB_DIR := lib
SRC_DIR := src
#BIN     := sched_demo

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

STATIC := $(LIB_DIR)/libschedmm.a
SHARED := $(LIB_DIR)/libschedmm.so

.PHONY: all static shared clean

#all: $(BIN)
all: static shared

static: $(STATIC)

shared: $(SHARED)

#$(BIN): $(OBJS)
#	$(CC) $(CFLAGS) $^ -o $@
#	@echo "Built binary: $@"

#static: $(STATIC)
$(STATIC): $(OBJS) | $(LIB_DIR)
	ar rcs $@ $^
	@echo "Built: $@"

#shared: $(SHARED)
$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC -o $@ $^
	@echo "Built: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

#clean:
#	rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN)
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
