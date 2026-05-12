#termio/
#├── include/
#│   ├── tty.h
#│   ├── tty_mode.h
#│   ├── tty_id.h
#│   ├── tty_pass.h
#│   └── tty_winsize.h
#├── src/
#│   ├── tty_mode.c
#│   ├── tty_id.c
#│   ├── tty_pass.c
#│   ├── tty_winsize.c
#│   └── main.c
#└── Makefile

CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu99 -O2 -Iinclude
BIN     := termio_demo
SRC_DIR := src
OBJ_DIR := obj

SRCS := $(SRC_DIR)/tty_mode.c    \
        $(SRC_DIR)/tty_id.c      \
        $(SRC_DIR)/tty_pass.c    \
        $(SRC_DIR)/tty_winsize.c \
        $(SRC_DIR)/main.c

OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Built: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN)
