#uiox_ipc/
#├── include/
#│   ├── ipc_types.h
#│   ├── ptrace.h
#│   ├── msg.h
#│   ├── shm.h
#│   ├── sem.h
#│   └── socket.h
#├── src/
#│   ├── ptrace.c
#│   ├── msg.c
#│   ├── shm.c
#│   ├── sem.c
#│   └── socket.c
#├── main.c
#└── Makefile

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11 -Iinclude
AR      := ar
ARFLAGS := rcs

SRC_DIR := src
OBJ_DIR := obj
LIB_DIR := lib
#TARGET  := uiox_ipc

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

STATIC  := $(LIB_DIR)/libuiox_ipc.a
SHARED  := $(LIB_DIR)/libuiox_ipc.so

.PHONY: all static shared clean run

#all: $(TARGET)
all: static shared

static: $(STATIC)

shared: $(SHARED)

#$(TARGET): $(OBJS) main.c | $(OBJ_DIR)
#	$(CC) $(CFLAGS) -fPIC $^ -o $@
#	@echo "Built: $@"

#static: $(STATIC)
$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
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

run: $(TARGET)
	./$(TARGET)

#clean:
#	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET)
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
