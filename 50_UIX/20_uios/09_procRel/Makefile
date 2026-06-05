#proc_rel/
#├── include/
#│   ├── proc_group.h
#│   ├── proc_session.h
##│   ├── proc_job.h
#│   └── proc_orphan.h
#├── src/
#│   ├── proc_group.c
#│   ├── proc_session.c
#│   ├── proc_job.c
#│   ├── proc_orphan.c
#│   └── main.c
#└── Makefile

CC      := gcc
CFLAGS  := -Wall -Wextra -std=gnu99 -O2 -Iinclude
BIN     := proc_rel
SRC_DIR := src
OBJ_DIR := obj

SRCS := $(SRC_DIR)/proc_group.c   \
        $(SRC_DIR)/proc_session.c \
        $(SRC_DIR)/proc_job.c     \
        $(SRC_DIR)/proc_orphan.c  \
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
