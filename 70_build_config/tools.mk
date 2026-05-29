# =============================================================
# build_config/tools.mk
# Toolchain detection and validation for UIOX build system.
# Detects whether cross-compilers are available; falls back
# to native gcc for x86_64.
# =============================================================

# --- Host detection ------------------------------------------
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# --- ARM64 toolchain -----------------------------------------
ARM64_CC_CANDIDATES := aarch64-linux-gnu-gcc \
                       aarch64-unknown-elf-gcc \
                       aarch64-elf-gcc

ARM64_CC := $(firstword $(foreach t,$(ARM64_CC_CANDIDATES),\
              $(if $(shell which $(t) 2>/dev/null),$(t),)))

ifeq ($(ARM64_CC),)
  ARM64_CC      = aarch64-linux-gnu-gcc
  ARM64_MISSING = 1
endif

ARM64_AS  := $(subst gcc,as,  $(ARM64_CC))
ARM64_AR  := $(subst gcc,ar,  $(ARM64_CC))
ARM64_LD  := $(subst gcc,ld,  $(ARM64_CC))
ARM64_OBJCOPY := $(subst gcc,objcopy, $(ARM64_CC))
ARM64_OBJDUMP := $(subst gcc,objdump, $(ARM64_CC))
ARM64_NM      := $(subst gcc,nm,      $(ARM64_CC))
ARM64_SIZE    := $(subst gcc,size,    $(ARM64_CC))
ARM64_STRIP   := $(subst gcc,strip,   $(ARM64_CC))

# --- ARM32 toolchain -----------------------------------------
ARM32_CC_CANDIDATES := arm-linux-gnueabihf-gcc \
                       arm-none-eabi-gcc \
                       arm-unknown-elf-gcc

ARM32_CC := $(firstword $(foreach t,$(ARM32_CC_CANDIDATES),\
              $(if $(shell which $(t) 2>/dev/null),$(t),)))

ifeq ($(ARM32_CC),)
  ARM32_CC      = arm-linux-gnueabihf-gcc
  ARM32_MISSING = 1
endif

ARM32_AS  := $(subst gcc,as,      $(ARM32_CC))
ARM32_AR  := $(subst gcc,ar,      $(ARM32_CC))
ARM32_LD  := $(subst gcc,ld,      $(ARM32_CC))
ARM32_OBJCOPY := $(subst gcc,objcopy, $(ARM32_CC))
ARM32_OBJDUMP := $(subst gcc,objdump, $(ARM32_CC))
ARM32_NM      := $(subst gcc,nm,      $(ARM32_CC))
ARM32_SIZE    := $(subst gcc,size,    $(ARM32_CC))
ARM32_STRIP   := $(subst gcc,strip,   $(ARM32_CC))

# --- x86_64 toolchain (native gcc preferred) -----------------
X86_CC_CANDIDATES := gcc x86_64-linux-gnu-gcc x86_64-elf-gcc

X86_CC := $(firstword $(foreach t,$(X86_CC_CANDIDATES),\
             $(if $(shell which $(t) 2>/dev/null),$(t),)))

ifeq ($(X86_CC),)
  X86_CC      = gcc
  X86_MISSING = 1
endif

X86_AS  := $(subst gcc,as,      $(X86_CC))
X86_AR  := $(subst gcc,ar,      $(X86_CC))
X86_LD  := $(subst gcc,ld,      $(X86_CC))
X86_OBJCOPY := $(subst gcc,objcopy, $(X86_CC))
X86_OBJDUMP := $(subst gcc,objdump, $(X86_CC))
X86_NM      := $(subst gcc,nm,      $(X86_CC))
X86_SIZE    := $(subst gcc,size,    $(X86_CC))
X86_STRIP   := $(subst gcc,strip,   $(X86_CC))

# --- Toolchain availability report ---------------------------
.PHONY: check_tools
check_tools:
	@echo "=== UIOX Toolchain Check ==="
	@echo -n "ARM64  CC : $(ARM64_CC) ... "
	@which $(ARM64_CC) > /dev/null 2>&1 && echo "OK" || echo "MISSING"
	@echo -n "ARM32  CC : $(ARM32_CC) ... "
	@which $(ARM32_CC) > /dev/null 2>&1 && echo "OK" || echo "MISSING"
	@echo -n "x86_64 CC : $(X86_CC) ... "
	@which $(X86_CC)   > /dev/null 2>&1 && echo "OK" || echo "MISSING"
	@echo "==========================="
