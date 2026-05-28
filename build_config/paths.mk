# all source path definitions
# =============================================================
# build_config/paths.mk
# All UIOX source path definitions.
# Every subsystem directory is listed here so every other
# Makefile fragment can simply include this file.
# =============================================================

# -- Repository root (resolved relative to this file) ---------
UIOX_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)

# -- Architecture layer ---------------------------------------
ARCH_DIR        := $(UIOX_ROOT)/10_Arch
ARCH_ARM64_DIR  := $(ARCH_DIR)/arm64
ARCH_ARM32_DIR  := $(ARCH_DIR)/arm32
ARCH_X86_64_DIR := $(ARCH_DIR)/x86_64

# -- Driver interfaces ----------------------------------------
DRV_IFACE_DIR   := $(UIOX_ROOT)/20_DriverInterfaces
DRV_IFACE_INC   := $(DRV_IFACE_DIR)/include
DRV_IFACE_SRC   := $(DRV_IFACE_DIR)/src

# -- Device drivers -------------------------------------------
DEV_DRV_DIR     := $(UIOX_ROOT)/30_DeviceDrivers
DEV_DRV_CHAR    := $(DEV_DRV_DIR)/00_character
DEV_DRV_BLK     := $(DEV_DRV_DIR)/01_block
DEV_DRV_INC     := $(DEV_DRV_DIR)/include
DEV_DRV_SRC     := $(DEV_DRV_DIR)/src

# -- Buffer cache ---------------------------------------------
BUFCACHE_DIR    := $(UIOX_ROOT)/31_BufferCache

# -- File system ----------------------------------------------
FS_DIR          := $(UIOX_ROOT)/32_FileSystem
FS_FSA_DIR      := $(FS_DIR)/01_fsa
FS_SCFS_DIR     := $(FS_DIR)/10_scfs

# -- Process control subsystem --------------------------------
PROC_DIR        := $(UIOX_ROOT)/33_ProcessControlSubsystem
PROC_IPC_DIR    := $(PROC_DIR)/00_inter-process-communication
PROC_SCHED_DIR  := $(PROC_DIR)/01_schedular
PROC_MM_DIR     := $(PROC_DIR)/02_memory-managment
PROC_STRUCT_DIR := $(PROC_DIR)/40_procStruct
PROC_SCPS_DIR   := $(PROC_DIR)/50_scps

# -- System call interface ------------------------------------
SYSCALL_DIR     := $(UIOX_ROOT)/40_SystemCallInterface

# -- UIX userspace layer --------------------------------------
UIX_DIR         := $(UIOX_ROOT)/50_UIX
UIX_LIBS_DIR    := $(UIX_DIR)/00_libs
UIX_SHELL_DIR   := $(UIX_DIR)/01_shell
UIX_UIOS_DIR    := $(UIX_DIR)/20_uios
UIX_APPS_DIR    := $(UIX_DIR)/21_apps

# -- Linker scripts -------------------------------------------
LINKER_DIR      := $(UIOX_ROOT)/linker

# -- Build output tree ----------------------------------------
BUILD_DIR       := $(UIOX_ROOT)/build
OBJ_DIR         := $(BUILD_DIR)/obj
LIB_DIR         := $(BUILD_DIR)/lib
BIN_DIR         := $(BUILD_DIR)/bin
MAP_DIR         := $(BUILD_DIR)/map

# -- Derived per-arch output dirs (filled in by arch mk) ------
# OBJ_ARCH  — set per-architecture fragment
# BIN_ARCH  — set per-architecture fragment
