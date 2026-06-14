# UIOX Bootloader Makefile
# make ARCH=arm64 | arm32 | x86_64
# make all

ARCH ?= x86_64

CC_arm64  = aarch64-linux-gnu-gcc
LD_arm64  = aarch64-linux-gnu-ld
OC_arm64  = aarch64-linux-gnu-objcopy
CC_arm32  = arm-linux-gnueabihf-gcc
LD_arm32  = arm-linux-gnueabihf-ld
OC_arm32  = arm-linux-gnueabihf-objcopy
CC_x86_64 = gcc
LD_x86_64 = ld
OC_x86_64 = objcopy

CC     = $(CC_$(ARCH))
LD     = $(LD_$(ARCH))
OBJCPY = $(OC_$(ARCH))

ifeq ($(ARCH),arm64)
  MARCH=-march=armv8-a; DEF=-D__aarch64__
  LD_SCRIPT=linker/uiox_boot_arm64.ld
  ENTRY_S=src/arch/arm64/uiox_boot_entry_arm64.S
  HW_C=src/arch/arm64/uiox_boot_hw_arm64.c
else ifeq ($(ARCH),arm32)
  MARCH=-march=armv7-a -marm; DEF=-D__arm__
  LD_SCRIPT=linker/uiox_boot_arm32.ld
  ENTRY_S=src/arch/arm32/uiox_boot_entry_arm32.S
  HW_C=src/arch/arm32/uiox_boot_hw_arm32.c
else
  MARCH=-m64 -mno-red-zone; DEF=-D__x86_64__
  LD_SCRIPT=linker/uiox_boot_x86_64.ld
  ENTRY_S=src/arch/x86_64/uiox_boot_entry_x86.S
  HW_C=src/arch/x86_64/uiox_boot_hw_x86.c
endif

CFLAGS = -Wall -Wextra -std=c99 -ffreestanding -fno-builtin \
         -fno-stack-protector -fno-common -fno-pic \
         -nostdlib -nostdinc -O2 -g3 \
         $(MARCH) $(DEF) -Iinclude

SRCS = src/uiox_boot_main.c src/uiox_boot_mem.c \
       src/uiox_boot_console.c src/uiox_boot_fs.c \
       src/uiox_boot_verify.c src/uiox_boot_handoff.c \
       $(HW_C)

B    = build/$(ARCH)
OBJS = $(patsubst src/%.c,$(B)/%.o,$(SRCS)) \
       $(B)/entry.o

.PHONY: all clean arm64 arm32 x86_64
all: ; $(MAKE) ARCH=arm64; $(MAKE) ARCH=arm32; $(MAKE) ARCH=x86_64
arm64:  ; $(MAKE) ARCH=arm64  $(B)/uiox_boot.bin
arm32:  ; $(MAKE) ARCH=arm32  $(B)/uiox_boot.bin
x86_64: ; $(MAKE) ARCH=x86_64 $(B)/uiox_boot.bin

$(B)/entry.o: $(ENTRY_S)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(B)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(B)/uiox_boot.elf: $(OBJS)
	$(LD) --no-undefined --build-id=none -nostdlib -static \
	      -T $(LD_SCRIPT) -o $@ $^

$(B)/uiox_boot.bin: $(B)/uiox_boot.elf
	$(OBJCPY) -O binary $< $@
	@echo "Built: $@ ($(shell wc -c < $@) bytes)"

clean: ; rm -rf build/
