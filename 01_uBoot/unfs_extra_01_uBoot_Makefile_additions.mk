# =============================================================================
# Additions to 01_uBoot/Makefile for UNFS support
# Add the following lines to the existing Makefile SRCS section:
# =============================================================================

# UNFS reader (read-only bootloader client)
SRCS_COMMON += src/unfs.c
SRCS_COMMON += src/unfs_boot_bridge.c

# Additional include path for unfs.h
CFLAGS_COMMON += -I$(MFDIR)include
