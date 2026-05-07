CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2 -g \
           -D_POSIX_C_SOURCE=200809L \
           -D_XOPEN_SOURCE=700
TARGET  := ch4_files_dirs
SRCS    := files_dirs.c main.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c files_dirs.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
