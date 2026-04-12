# Compiler and flags
CC       := gcc
CFLAGS   := -Wall -Wextra -fPIC -O2
AR       := ar
ARFLAGS  := rcs

# Project config
LIB_NAME := UIXlib
SRC_DIR  := ../uixlibs
OBJ_DIR  := $(SRC_DIR)/obj
LIB_DIR  := $(SRC_DIR)/lib

# Sources and objects
SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Output targets
STATIC   := $(LIB_DIR)/lib$(LIB_NAME).a
SHARED   := $(LIB_DIR)/lib$(LIB_NAME).so

# ─────────────────────────────────────────────
.PHONY: all static shared clean

all: static shared

static: $(STATIC)

shared: $(SHARED)

# Static library
$(STATIC): $(OBJS) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built static library: $@"

# Shared library
$(SHARED): $(OBJS) | $(LIB_DIR)
	$(CC) -shared -o $@ $^
	@echo "Built shared library: $@"

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if missing
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR)
