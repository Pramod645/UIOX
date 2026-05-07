#05_stdio/
#├── include/
#│   └── stdio_lib.h
#├── src/
#│   └── stdio_lib.c
#├── main.c
#└── Makefile

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -g -std=c11 \
           -D_GNU_SOURCE \
           -D_POSIX_C_SOURCE=200809L \
           -D_XOPEN_SOURCE=700 \
           -Iinclude

AR      := ar
ARFLAGS := rcs

SRC_DIR := src
OBJ_DIR := obj
LIB_DIR := lib

TARGET  := stdio_demo
STATIC  := $(LIB_DIR)/libuiox_stdio.a
SHARED  := $(LIB_DIR)/libuiox_stdio.so

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
MAIN_O  := $(OBJ_DIR)/main.o

.PHONY: all static shared run clean dirs

all: dirs $(TARGET) static shared

$(TARGET): $(OBJS) $(MAIN_O)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Built: $@"

static: $(STATIC)

$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built: $@"

shared: $(SHARED)

$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC -o $@ $^
	@echo "Built: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/main.o: main.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

dirs: $(OBJ_DIR) $(LIB_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET)
	rm -f /tmp/uiox_stdio_*.txt /tmp/uiox_stdio_*.bin /tmp/uiox_stdio_*
