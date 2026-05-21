CC     = gcc
CFLAGS = -Wall -Wextra -std=c99 \
         -I../../40_SystemCallInterface \
         -I../../50_UIX/00_libs/00_uixlibs \
         -DSYS_CALL_ENBLE_DISABLE=1

OBJS = uios.o \
       ../../32_FileSystem/10_scfs/src/fs.o \
       ../../33_ProcessControlSubsystem/50_scps/src/proc.o \
       ../../33_ProcessControlSubsystem/50_scps/src/fork.o \
       ../../33_ProcessControlSubsystem/50_scps/src/exit_wait.o \
       ../../33_ProcessControlSubsystem/50_scps/src/exec.o \
       ../../33_ProcessControlSubsystem/50_scps/src/signal.o \
       ../../33_ProcessControlSubsystem/00_inter-process-communication/src/ipc.o \
       ../../33_ProcessControlSubsystem/01_schedular/src/sched.o \
       ../../33_ProcessControlSubsystem/02_memory-managment/src/mm.o

TARGET = uios

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
