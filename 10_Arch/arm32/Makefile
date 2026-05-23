#32_Architecture/
#    include/
#        arm_types.h          ← base bit-width types
#        arm_registers.h      ← register definitions
#        arm_psr.h            ← Program Status Register
#        arm_instr_format.h   ← all instruction encoding structs
#        arm_opcodes.h        ← opcode constants
#        arm_macros.h         ← per-instruction address macros
#        arm_memory.h         ← memory map / address space
#        arm_exceptions.h     ← exception vectors
#        arm_coprocessor.h    ← coprocessor instruction structs
#        arm_arch.h           ← master include
#    src/
#        arm_decode.c         ← instruction decoder
#        arm_execute.c        ← instruction executor (stub)
#        arm_memory.c         ← memory access implementation
#        arm_exceptions.c     ← exception handler stubs

#brew install arm-none-eabi-gcc
#arm-none-eabi-gcc --version
#arm-none-eabi-gcc (GCC) 16.1.0
#Copyright (C) 2026 Free Software Foundation, Inc.
#This is free software; see the source for copying conditions.  There is NO
#warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

CC     = arm-none-eabi-gcc
CFLAGS = -Wall -Wextra -std=c99 -ffreestanding \
         -I./include -mcpu=arm926ej-s

OBJS = src/arm_decode.o \
       src/arm_execute.o \
       src/arm_memory.o \
       src/arm_exceptions.o

LIB = libarm_arch.a

all: $(LIB)

$(LIB): $(OBJS)
	arm-none-eabi-ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LIB)
