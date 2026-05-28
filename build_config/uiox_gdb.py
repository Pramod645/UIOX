#!/usr/bin/env python3
# =============================================================
# build_config/uiox_gdb.py
# GDB helper script for UIOX kernel debugging via QEMU.
# Usage:
#   # Terminal 1 — start QEMU with GDB stub:
#   qemu-system-aarch64 -machine virt -cpu cortex-a53 -m 64M \
#       -nographic -kernel build/bin/uiox_arm64.elf \
#       -S -gdb tcp::1234
#
#   # Terminal 2 — launch GDB:
#   gdb-multiarch build/bin/uiox_arm64.elf \
#       -ex "source build_config/uiox_gdb.py"
# =============================================================

import gdb
import os

UIOX_ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_BIN  = os.path.join(UIOX_ROOT, "build", "bin")

# --- Colour helpers ------------------------------------------
RED    = "\033[31m"
GREEN  = "\033[32m"
YELLOW = "\033[33m"
CYAN   = "\033[36m"
RESET  = "\033[0m"

def banner(msg):
    print(f"{CYAN}{'='*60}{RESET}")
    print(f"{CYAN}  {msg}{RESET}")
    print(f"{CYAN}{'='*60}{RESET}")

# --- Connect to QEMU GDB stub --------------------------------
class UIXConnect(gdb.Command):
    """Connect to QEMU GDB stub on localhost:1234"""
    def __init__(self):
        super().__init__("uiox-connect", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        port = arg.strip() if arg.strip() else "1234"
        banner(f"UIOX: connecting to QEMU on port {port}")
        gdb.execute(f"target remote localhost:{port}")
        gdb.execute("set architecture aarch64")
        gdb.execute("layout asm")
        print(f"{GREEN}Connected. Use 'c' to continue, 'si' to step.{RESET}")

UIXConnect()

# --- Load symbols for a given arch ---------------------------
class UIXSymbols(gdb.Command):
    """Load UIOX kernel symbols: uiox-symbols arm64|arm32|x86_64"""
    def __init__(self):
        super().__init__("uiox-symbols", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        arch = arg.strip() or "arm64"
        elf  = os.path.join(BUILD_BIN, f"uiox_{arch}.elf")
        if not os.path.exists(elf):
            print(f"{RED}ELF not found: {elf}{RESET}")
            return
        gdb.execute(f"symbol-file {elf}")
        print(f"{GREEN}Symbols loaded: {elf}{RESET}")

UIXSymbols()

# --- Break at common kernel entry points ---------------------
class UIXBreakpoints(gdb.Command):
    """Set standard UIOX kernel breakpoints"""
    def __init__(self):
        super().__init__("uiox-bp", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        bps = [
            "_start",
            "arch_init",
            "buf_init",
            "inode_cache_init",
            "fs_mkfs",
            "x86_exc_gp",
            "x86_exc_pf",
            "arm64_exc_sync",
            "arm_exc_data_abort",
        ]
        banner("UIOX: setting kernel breakpoints")
        for bp in bps:
            try:
                gdb.execute(f"break {bp}")
                print(f"  {GREEN}bp{RESET} → {bp}")
            except gdb.error:
                print(f"  {YELLOW}skip{RESET} → {bp} (symbol not found)")

UIXBreakpoints()

# --- Print CPU registers in UIOX format ----------------------
class UIXRegs(gdb.Command):
    """Print UIOX CPU registers"""
    def __init__(self):
        super().__init__("uiox-regs", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        banner("UIOX: CPU registers")
        gdb.execute("info registers")

UIXRegs()

# --- Print UIOX memory map -----------------------------------
class UIXMemMap(gdb.Command):
    """Print UIOX kernel memory map symbols"""
    def __init__(self):
        super().__init__("uiox-memmap", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        banner("UIOX: kernel memory map")
        syms = [
            "_text_start", "_text_end",
            "_data_start", "_data_end",
            "_bss_start",  "_bss_end",
            "__stack_top__", "__heap_start__", "__heap_end__",
        ]
        for s in syms:
            try:
                val = gdb.parse_and_eval(s)
                print(f"  {CYAN}{s:<20}{RESET} = 0x{int(val):016X}")
            except gdb.error:
                print(f"  {YELLOW}{s:<20}{RESET} = (not found)")

UIXMemMap()

# --- Startup banner ------------------------------------------
banner("UIOX GDB Helper Loaded")
print(f"  {GREEN}uiox-connect [port]{RESET}   connect to QEMU GDB stub")
print(f"  {GREEN}uiox-symbols [arch]{RESET}   load kernel ELF symbols")
print(f"  {GREEN}uiox-bp{RESET}               set kernel breakpoints")
print(f"  {GREEN}uiox-regs{RESET}             print CPU registers")
print(f"  {GREEN}uiox-memmap{RESET}           print memory map symbols")
print("")
