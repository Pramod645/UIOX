# =============================================================
# build_config/common.mk
# Compiler flags shared across ALL architectures.
# Included by each arch-specific .mk file.
# =============================================================

# --- C standard ----------------------------------------------
C_STD        := -std=c99

# --- Warning flags -------------------------------------------
WARN_FLAGS   := -Wall \
                -Wextra \
                -Wshadow \
                -Wstrict-prototypes \
                -Wmissing-prototypes \
                -Wredundant-decls \
                -Wundef \
                -Wpointer-arith \
                -Wcast-align \
                -Wnested-externs \
                -Wno-unused-parameter

# --- Freestanding kernel flags (no libc) ---------------------
KERN_FLAGS   := -ffreestanding \
                -fno-builtin \
                -fno-stack-protector \
                -fno-common \
                -fno-pic \
                -fno-pie \
                -nostdlib \
                -nostdinc \
                -nodefaultlibs

# --- Debug / optimisation ------------------------------------
ifeq ($(DEBUG),1)
  OPT_FLAGS  := -O0 -g3 -DDEBUG=1
else
  OPT_FLAGS  := -O2 -DDEBUG=0
endif

# --- Common preprocessor defines ----------------------------
COMMON_DEFS  := -DUIOX=1 \
                -DUIOX_VERSION_MAJOR=0 \
                -DUIOX_VERSION_MINOR=1

# --- Combined common CFLAGS ----------------------------------
COMMON_CFLAGS := $(C_STD) $(WARN_FLAGS) $(KERN_FLAGS) \
                 $(OPT_FLAGS) $(COMMON_DEFS)

# --- Common LDFLAGS ------------------------------------------
COMMON_LDFLAGS := -nostdlib \
                  -static \
                  --build-id=none

# --- Build output directory ----------------------------------
BUILD_DIR    ?= build
OBJ_DIR      ?= $(BUILD_DIR)/obj
BIN_DIR      ?= $(BUILD_DIR)/bin
MAP_DIR      ?= $(BUILD_DIR)/map

# --- Utility: create directory if missing --------------------
MKDIR_P := mkdir -p

# --- Source tree root ----------------------------------------
UIOX_ROOT    := $(CURDIR)

# --- All subsystem source paths (relative to UIOX_ROOT) -----
DIR_ARCH     := 10_Arch
DIR_DRVIFACE := 20_DriverInterfaces
DIR_DRVDEV   := 30_DeviceDrivers
DIR_BUFCACHE := 31_BufferCache
DIR_FS       := 32_FileSystem
DIR_PROC     := 33_ProcessControlSubsystem
DIR_SYSCALL  := 40_SystemCallInterface
DIR_UIX      := 50_UIX
DIR_LINKER   := linker

# --- Subsystem include paths (all arches see these) ----------
SUBSYS_INCLUDES := \
    -I$(UIOX_ROOT)/$(DIR_DRVIFACE)/include \
    -I$(UIOX_ROOT)/$(DIR_DRVDEV)/include \
    -I$(UIOX_ROOT)/$(DIR_BUFCACHE)/include \
    -I$(UIOX_ROOT)/$(DIR_FS) \
    -I$(UIOX_ROOT)/$(DIR_PROC) \
    -I$(UIOX_ROOT)/$(DIR_SYSCALL) \
    -I$(UIOX_ROOT)/$(DIR_UIX)/00_libs

# --- Subsystem source globs (used per arch Makefile) ---------
SUBSYS_SRCS := \
    $(wildcard $(UIOX_ROOT)/$(DIR_DRVIFACE)/src/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_DRVDEV)/src/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_DRVDEV)/00_character/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_DRVDEV)/01_block/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_BUFCACHE)/src/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_FS)/01_fsa/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_FS)/10_scfs/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_PROC)/00_inter-process-communication/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_PROC)/01_schedular/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_PROC)/02_memory-managment/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_PROC)/40_procStruct/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_PROC)/50_scps/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_SYSCALL)/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_UIX)/00_libs/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_UIX)/01_shell/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_UIX)/20_uios/*.c) \
    $(wildcard $(UIOX_ROOT)/$(DIR_UIX)/21_apps/*.c)

# --- Rule: compile a .c to .o --------------------------------
# $(1) = compiler   $(2) = cflags   $(3) = obj_dir
define COMPILE_C
$(3)/%.o: %.c
	@$(MKDIR_P) $$(dir $$@)
	$(1) $(2) -c -o $$@ $$<
endef
