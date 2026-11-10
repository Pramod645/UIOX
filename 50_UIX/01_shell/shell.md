Shell layer: This layer provides a command-line interface for interacting with the system. 
The shell is a program that interprets user commands and executes them on the system.

Shell in Operating System:
The Shell is an interface that allows users to interact with the OS. It acts as a bridge between the 
user and the kernel by processing commands.

Types of Shells:

Shell Type                                    Description                           Examples
1.Command line INterfaces(CLI)            Text based interfaces,efficient           Linux(Bash),Windows(PowerShell)
2.Graphical User Interfaces(GUI)          Visual Interfase with icons               Window, macOS
3.Voice-Based INterfacse                  User voice command for interaction        Siri, Alexa
4.Network-Based INterfacse(nli)                  
=====================================================================================================

50_UIX/01_shell — the shell layer
The layer that turns a console into something a person can talk to.

Read this before grepping for a Makefile. It exists now — it did not for the whole life of this tree, and [KIX] 50_UIX/01_shell was reported skipped (no Makefile) on every build, on every architecture, while the directory sat populated and uncompiled.

Shape
Four front-ends, none of which contains a console.



50_UIX/01_shell/
├── Makefile            recurses into the four modalities
├── shell.md            this file
├── 10_cli/             command line — the one with sources
│   ├── Makefile
│   └── src/uiox_sh.c
├── 20_gui/             graphical      — empty
├── 30_vbi/             voice          — empty
├── 40_nli/             natural language — empty
├── termio/             user-side tty vocabulary
└── pseudoTerminals/
The numbering is the design: 10–40 are peers that differ only in how a person speaks to the front-end. They do not differ in what sits underneath.

The console is not here


50_UIX/01_shell          this layer  — read(0,…) / write(1,…)
      ┊
      ┊  ← the SCFS boundary  (uiox_kix_router.c intercepts fd 0/1/2)
      ┊
30_KIX/32_FS/10_scfs     fd table, mount table, syscall bodies
      │
30_KIX/30_DSS/20_DD/
   ├── 01_Com/uart/      uiox_uart_if.c    the discipline
   └── 01_Com/console/   uiox_console.c    the binding
      │
30_KIX/30_DSS/10_DI/     uiox_uart_fwops.c  the HAL
      │
   PL011 @ 0x101F1000
A front-end reaches the console through two syscalls and nothing else:

c


write(1, "uiox> ", 6);              /* prompt out  */
int n = read(0, buf, sizeof buf);   /* whole line in */
Both are intercepted by descriptor number in uiox_kix_router.c, ahead of the SCFS table:

c


if (a0 <= 2u) {
    if (nr == SYS_READ)  return uiox_console_readline(...);
    if (nr == SYS_WRITE) return uiox_console_write(...);
}
That is why a CMD inode never has to answer SCFS_ENOTTY: the console is reached before the file table is consulted, and 10_scfs never learns it exists — the rule uiox_devtable.c states.

ioctl(fd, UIOX_TTY_IOC_*) keeps its own route — SCFS → registry → dev->ops → uiox_uart_if_ioctl — and does not pass through the console.

The rule, enforced
Layer	Owns	May include
10_cli 20_gui 30_vbi 40_nli	prompt, builtins, the read loop	uiox_termios.h, uiox_kix_scfs.h
01_Com/console (kernel)	the binding, the WINCH hook	— it is below the boundary
01_Com/uart (kernel)	termios state, canon_feed()	uiox_uart_hw.h
A front-end may not name a UART symbol and may not include uiox_console.h. If one does, the layering has leaked and the fix belongs in the router, not here. 10_cli/Makefile carries a check-layering target:

sh


make -C 50_UIX/01_shell/10_cli check-layering
make -C 50_UIX/01_shell check-layering
What the kernel does for the shell
The line editing is not the shell's. canon_feed() runs inside uiox_uart_if_read():

Byte	Kernel's response
printable	append, echo back
ERASE (DEL)	BS SP BS, and back the cursor up
KILL (^U)	drop the line the same way
VEOF (^D)	end the line
\n	complete the line
VINTR (^C)	ISIG → uiox_tty_signal_from_char(), byte consumed
Output conversion is not the shell's either: tx_char() applies OPOST|ONLCR, so a "\n" from a front-end reaches the wire as CRLF.

So uiox_sh_readline() is a read loop and nothing more — which is what uiox_sh.c's own header claims at lines 113–118, and it is now literally true rather than aspirational.

What was decided, and why
Option A — a kernel-side tty. The front-end calls read(0,…) and blocks; the kernel does the echoing, erasing, killing and CR→LF translation. Chosen because uiox_termios.h and uiox_uart_ioctl.h already declare a tty, and because uiox_uart_if.c's discipline was written for exactly this.

The console is a sibling of the front-ends, not a child. It lives in the kernel, below the boundary, so a GUI front-end can use it exactly as the CLI does.

cli / gui / vbi / nli are peers. They differ in modality, not in substrate. A peer layout would give each its own copy of the plumbing.

Open, and what each blocks
Unknown	Blocks	Fix
uiox_sh.c still at the layer root	everything here	move to 10_cli/src/, fix its header path (50_UIX/01_shell/src/uiox_sh.c has never existed)
sh_fd1 initialised to 0 (uiox_sh.c:147)	output routed through SYS_READ	correct to 1
output via uiox_kix_scfs_write(sh_fd1, …)	a direct 32_FS reach	use write(1, …) through the router
uiox_shell_start() commented (uiox_kernel_main.c:485)	the shell never runs	uncomment once it compiles
uiox_shell_start stub (uiox_kernel_main.c:211)	duplicate symbol	delete or make weak
Order of work
Move uiox_sh.c → 10_cli/src/, correct its header path
sh_fd1 = 0 → 1; uiox_kix_scfs_write(sh_fd1, …) → write(1, …)
Build: make -C 50_UIX/01_shell
Drop the || echo from the umbrella's 01_shell line — a compile failure must stop the build, not be reported as skipped
Uncomment uiox_kernel_main.c:485, delete the stub at :211
Not in scope
Pipes, redirection, job control, $PATH, external programs — uiox_sh.c's own header says so. Every command is a builtin, and "not built in" is reported as such rather than silently ignored, which is the rule the rest of the tree follows.

=================

make -C 50_UIX/01_shell/01_cli arm64 \
     CC=aarch64-elf-gcc ARCH_CFLAGS="-march=armv8-a -mabi=lp64" \
     ARCH_CPUDEF="-D__aarch64__"
