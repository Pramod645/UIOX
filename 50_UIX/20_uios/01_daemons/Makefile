#daemon/
#├── include/
#│   ├── daemon.h
#│   ├── daemon_log.h
##│   ├── daemon_lock.h
#│   ├── daemon_signal.h
#│   └── daemon_server.h
#├── src/
#│   ├── daemon.c
##│   ├── daemon_log.c
#│   ├── daemon_lock.c
#│   ├── daemon_signal.c
#│   ├── daemon_signal_mt.c
#│   ├── daemon_server.c
#│   └── main.c
#└── Makefile

CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu99 -O2
LDFLAGS := -lpthread

SRC_DIR := src
OBJ_DIR := obj
BIN     := daemon_demo

SRCS := $(SRC_DIR)/daemon.c        \
        $(SRC_DIR)/daemon_log.c    \
        $(SRC_DIR)/daemon_lock.c   \
        $(SRC_DIR)/daemon_signal.c \
        $(SRC_DIR)/daemon_server.c \
        $(SRC_DIR)/main.c

OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean st mt

# Single-threaded build (default)
all: st

st: CFLAGS += -Iinclude
st: $(BIN)

# Multi-threaded build
mt: CFLAGS += -Iinclude -DDAEMON_MULTITHREADED
mt: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)
