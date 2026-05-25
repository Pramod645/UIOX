#10_Arch/arm64/
#├── include/
#│   ├── arch_defs.h
#│   ├── arm64_arch.h
#│   ├── arm64_types.h
#│   ├── arm64_registers.h
#│   ├── arm64_psr.h
#│   ├── arm64_memory.h
#│   ├── arm64_exceptions.h
#│   ├── arm64_macros.h
#│   ├── arm64_instr_format.h
#│   ├── arm64_opcodes.h
#│   └── arm64_sysreg.h
#├── src/
#│   ├── arch_init.c
#│   ├── arm64_decode.c
#│   ├── arm64_exceptions.c
#│   ├── arm64_execute.c
#│   └── arm64_memory.c
#├── Makefile
#└── arm64.md

# arm64/
# include/
#   arm64_types.h          ← base bit-width types
#   arm64_registers.h      ← register definitions
#   arm64_psr.h            ← PSTATE / SPSR bits
#   arm64_instr_format.h   ← all instruction encoding structs
#   arm64_opcodes.h        ← opcode constants
#   arm64_macros.h         ← per-instruction address macros
#   arm64_memory.h         ← memory map / address space
#   arm64_exceptions.h     ← exception vectors
#   arm64_sysregs.h        ← system register interface
#   arm64_arch.h           ← master include
# src/
#   arm64_decode.c         ← instruction decoder
#   arm64_execute.c        ← instruction executor (stub)
#   arm64_memory.c         ← memory access implementation
#   arm64_exceptions.c     ← exception handler stubs
#
# brew install aarch64-elf-gcc   (macOS)
# apt install gcc-aarch64-linux-gnu   (Linux)
# aarch64-linux-gnu-gcc --version

#CC     = aarch64-linux-gnu-gcc
#AR     = aarch64-linux-gnu-ar
CC     = arm-none-eabi-gcc
CFLAGS = -Wall -Wextra -std=c99 -ffreestanding \
         -I./include -march=armv8-a

OBJS = src/arm64_decode.o    \
       src/arm64_execute.o   \
       src/arm64_memory.o    \
       src/arm64_exceptions.o

LIB = libarm64_arch.a

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LIB)
