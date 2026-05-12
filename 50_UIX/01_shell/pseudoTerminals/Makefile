#pty/
#├── include/
#│   ├── pty.h
#│   ├── pty_open.h
#│   ├── pty_fork.h
#│   ├── pty_loop.h
##│   └── pty_driver.h
#├── src/
#│   ├── pty_open.c
#│   ├── pty_fork.c
#│   ├── pty_loop.c
#│   ├── pty_driver.c
#│   ├── pty_main.c
#│   └── pty_sigtest.c
#└── Makefile

CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu99 -O2 -Iinclude
LDFLAGS :=

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

# ── Targets ─────────────────────────────────────────────────
PTY_BIN     := $(BIN_DIR)/pty
SIGTEST_BIN := $(BIN_DIR)/pty_sigtest

# ── Source groups ───────────────────────────────────────────
PTY_SRCS := $(SRC_DIR)/pty_open.c    \
            $(SRC_DIR)/pty_fork.c    \
            $(SRC_DIR)/pty_loop.c    \
            $(SRC_DIR)/pty_driver.c  \
            $(SRC_DIR)/pty_main.c

SIGTEST_SRCS := $(SRC_DIR)/pty_open.c   \
                $(SRC_DIR)/pty_fork.c   \
                $(SRC_DIR)/pty_sigtest.c

PTY_OBJS     := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(PTY_SRCS))
SIGTEST_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SIGTEST_SRCS))

.PHONY: all clean

all: $(PTY_BIN) $(SIGTEST_BIN)

$(PTY_BIN): $(PTY_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built: $@"

$(SIGTEST_BIN): $(SIGTEST_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
