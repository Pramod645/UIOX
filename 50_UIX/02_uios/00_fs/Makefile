# =============================================================
# Makefile for Uiox File I/O System
# Based on file systems
# =============================================================

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2 -g \
           -D_POSIX_C_SOURCE=200809L \
           -D_DEFAULT_SOURCE \
           -Iinclude

AR      := ar
ARFLAGS := rcs

# ── Directories ──────────────────────────────────────────────
SRC_DIR := src
INC_DIR := include
OBJ_DIR := obj
LIB_DIR := lib
TARGET  := file_io_test

# ── Source files ─────────────────────────────────────────────
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
MAIN_O  := $(OBJ_DIR)/main.o

STATIC  := $(LIB_DIR)/libfileio.a
SHARED  := $(LIB_DIR)/libfileio.so

ALL_OBJS := $(OBJS) $(MAIN_O)

# ── Targets ───────────────────────────────────────────────────
.PHONY: all static shared test run clean help

all: dirs $(TARGET) static shared

$(TARGET): $(ALL_OBJS)
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

# ── Object file rules ─────────────────────────────────────────
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(OBJ_DIR)/main.o: main.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Directory creation ────────────────────────────────────────
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

.PHONY: dirs
dirs: $(OBJ_DIR) $(LIB_DIR)

# ── Test programs ─────────────────────────────────────────────
test: $(TARGET)
	@echo "=== Running file I/O tests ==="
	./$(TARGET)

# Examples 
test_seek: static
	$(CC) $(CFLAGS) -L$(LIB_DIR) tests/test_seek.c -lfileio -o test_seek
	@echo "Testing seek capability:"
	./test_seek

test_hole: static  
	$(CC) $(CFLAGS) -L$(LIB_DIR) tests/test_hole.c -lfileio -o test_hole
	./test_hole
	@echo "File created with hole:"
	@ls -ls file.hole 2>/dev/null || echo "  (file.hole not found)"

test_flags: static
	$(CC) $(CFLAGS) -L$(LIB_DIR) tests/test_flags.c -lfileio -o test_flags
	@echo "Testing file flags on stdout:"
	./test_flags 1

# ── Performance test (like Figure 3.6) ────────────────────────
perf_test: static
	$(CC) $(CFLAGS) -L$(LIB_DIR) tests/perf_test.c -lfileio -o perf_test
	@echo "Running I/O efficiency test..."
	./perf_test

# ── Copy utility (like Figure 3.5) ────────────────────────────
copy: static
	$(CC) $(CFLAGS) -L$(LIB_DIR) tests/copy.c -lfileio -o copy
	@echo "Built copy utility"

# ── Run all tests ─────────────────────────────────────────────
run: test test_seek test_hole test_flags

# ── Clean ─────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(TARGET)
	rm -f test_seek test_hole test_flags perf_test copy
	rm -f file.hole temp.foo *.txt

# ── Help ──────────────────────────────────────────────────────
help:
	@echo "Uiox File I/O System Build"
	@echo ""
	@echo "Targets:"
	@echo "  make              build main test program + libraries"
	@echo "  make test         run comprehensive file I/O test"
	@echo "  make test_seek    build/run seek capability test"
	@echo "  make test_hole    build/run file holes test"
	@echo "  make test_flags   build/run file flags test"
	@echo "  make perf_test    build/run I/O efficiency test"
	@echo "  make copy         build simple copy utility"
	@echo "  make run          run all tests"
	@echo "  make clean        remove all build artifacts"
	@echo ""
	@echo "Functions implemented:"
	@echo "  open, openat      file opening with all POSIX flags"
	@echo "  read, write       data transfer with EOF/error handling"
	@echo "  lseek             seek with SEEK_SET/CUR/END + holes"
	@echo "  close             cleanup with reference counting"
	@echo "  creat             file creation (equiv to open+flags)"
	@echo "  dup, dup2         file descriptor duplication"
	@echo "  pread, pwrite     atomic seek+I/O operations"
	@echo "  fcntl             file control (F_DUPFD, F_GETFL, etc.)"
	@echo "  sync, fsync       buffer cache synchronization"
	@echo "  ioctl             device control operations"
	@echo "  /dev/fd/n         special file descriptor access"
