50_UIX/01_shell/           front-ends — cli, gui, vbi, nli
      │  (boot context today: in-kernel calls; later: syscalls)
      ▼
30_KIX/32_FS/10_scfs       fd table, mount table, syscall bodies
      │  dev->ops  ← the seam uiox_uart_blk.c provides
      ▼
30_KIX/30_DSS/20_DD/       uart/  (the discipline)   console/ (the name)
      │
      ▼
30_KIX/30_DSS/10_DI/       HAL — putc, rx_pop, registers
      │
      ▼
    silicon
======

uiox_dss_init()  →  uiox_uart_probe()  →  uiox_console_bind()
                                        →  uiox_uart_if_*  (the discipline)
                                        →  uiox_uart_hw_*  (the HAL)
                                        →  PL011 @ 0x101F1000
=================
The console is kernel-side, and SCFS is the boundary above it


50_UIX/01_shell        user-space intent — NOT BUILT (skipped, no Makefile)
      ┊
      ┊  ← the SCFS boundary, not crossed yet
      ┊
30_KIX/32_FS/10_scfs   fd table, mount table, syscall bodies  (uiox_kix_scfs_dispatch.c)
      │  dev->ops  ← the seam uiox_uart_blk.c provides
      ▼
30_KIX/30_DSS/20_DD/
   ├── 01_Com/uart/       uiox_uart_if.c  — the discipline
   └── 01_Com/console/    uiox_console.c  — the binding       ← in the image
      │  uiox_uart_if_*
      ▼
30_KIX/30_DSS/10_DI/     uiox_uart_fwops.c — putc / rx_pop / registers
      │
      ▼
   PL011 @ 0x101F1000
The console lives below SCFS, in 20_DD. It holds a uiox_uart_if_t * and forwards. Nothing above it knows it exists.

What happens today, at boot
uiox_kernel_main() → uiox_dss_init() → uiox_uart_probe(), and step 4xx in that probe:

c


(void)uiox_console_bind(&s_uart_if);
That sets s_console.tif = &s_uart_if. So from that moment the console is bound and live in the kernel, and uiox_console_ready() returns 1.

The two ways in, and which one SCFS serves
The ioctl path — this is the SCFS boundary, and it does NOT go through the console.

A userspace ioctl(fd, UIOX_TTY_IOC_GETATTR, &tio) travels:



10_scfs ioctl  →  uiox_dev_lookup((CMD,15), 0)
               →  dev->ops->ioctl            [uiox_uart_blk.c — the seam]
               →  uiox_uart_if_ioctl()
The console is bypassed entirely. That's correct: a (cls, unit) lookup through the registry is more direct than going up to a console and back down. uiox_console_ioctl() exists only for an in-kernel caller that already holds the uiox_uart_if_t.

The byte path — this is the one that has no SCFS route.

uiox_console_readline() / puts() reach s_console.tif → uiox_uart_if_read() / _write(). That works in-kernel, from anything holding the handle. There is no read(2) on /dev/ttyS0 today — 10_scfs's read path serves EVENT and REG inodes and answers SCFS_ENOTTY for a CMD inode. uiox_uart_blk.c's own header says so.





So the honest answer to "how does it work now"
Path	Route	State
boot binding	uiox_dss_init → uiox_uart_probe → uiox_console_bind	✅ live
termios ioctl	SCFS → registry → dev->ops → uiox_uart_if_ioctl	✅ serves, bypasses console
read/write from userspace	— no CMD-inode route in SCFS	❌ absent
read/write in-kernel	uiox_console_readline() → uiox_uart_if_read()	✅ callable, nothing calls it
WINCH	discipline counts swinsz_changes → uiox_console_winch()	✅ hook present, none registered
The console is bound, in the image, and inert. T uiox_console_bind at 0x40211528 and s_console at 0x40dfed70 are in all four kernels. Nothing has called readline().

--------------------------

What that opened
The SCFS boundary now has a door for the console's byte stream:



read(0, buf, n)   →  syscall_dispatch → a0 <= 2 && nr == SYS_READ
                  →  uiox_console_readline()   →  canon_feed() does ERASE/KILL/VEOF/ECHO
write(1, buf, n)  →  syscall_dispatch → a0 <= 2 && nr == SYS_WRITE
                  →  uiox_console_write()      →  tx_char() does OPOST|ONLCR
While ioctl(fd, UIOX_TTY_IOC_*) keeps its existing route — SCFS → registry → dev->ops → uiox_uart_if_ioctl — so termios and winsize never touch the console. Two paths, each doing the thing it's shaped for.

Both readline and write are already T in the ELF, so nothing new needed linking.