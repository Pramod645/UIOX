#uiox_sched/
#├── include/
#│   ├── sched_types.h
#│   ├── scheduler.h
#│   ├── clock.h
#│   ├── timer.h
#│   ├── timekeeping.h
#│   ├── profiler.h
#│   └── syscall_time.h
#├── src/
#│   ├── scheduler.c
#│   ├── clock.c
#│   ├── timer.c
#│   ├── timekeeping.c
#│   ├── profiler.c
#│   └── syscall_time.c
#├── main.c
#└── Makefile

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -std=c11 -Iinclude
LDFLAGS := -lm
AR      := ar
ARFLAGS := rcs

SRC_DIR := src
OBJ_DIR := obj
LIB_DIR := lib
#  := uiox_sched

SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

STATIC  := $(LIB_DIR)/libuiox_sched.a
SHARED  := $(LIB_DIR)/libuiox_sched.so

.PHONY: all static shared clean run

#all: $(TARGET)
all: static shared


static: $(STATIC)

shared: $(SHARED)

#$(TARGET): $(OBJS) main.c | $(OBJ_DIR)
#	$(CC) $(CFLAGS) -fPIC $^ -o $@ $(LDFLAGS)
#	@echo "Built: $@"

#static: $(STATIC)
$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built: $@"

#shared: $(SHARED)
$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)
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
