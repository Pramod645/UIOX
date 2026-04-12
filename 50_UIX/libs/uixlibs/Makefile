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

# Lib config
BUILD_SHARED = 0
BUILD_STATIC = 1

# Sources and objects
SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Output targets
ifneq ($(BUILD_SHARED),0)
    SHARED   := $(LIB_DIR)/lib$(LIB_NAME).so
endif
ifneq ($(BUILD_STATIC),0)
    STATIC   := $(LIB_DIR)/lib$(LIB_NAME).a
endif
# ─────────────────────────────────────────────
.PHONY: all static shared clean

all: static shared

ifneq ($(BUILD_SHARED),0)
    shared: $(SHARED)
endif
ifneq ($(BUILD_STATIC),0)
    static: $(STATIC)
endif

# Static library
ifneq ($(BUILD_STATIC),0)
    $(STATIC): $(OBJS) | $(LIB_DIR)
		$(AR) $(ARFLAGS) $@ $^
		@echo "Built static library: $@"
endif
# Shared library
ifneq ($(BUILD_SHARED),0)
    $(SHARED): $(OBJS) | $(LIB_DIR)
		$(CC) -shared -o $@ $^
		@echo "Built shared library: $@"
endif

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
