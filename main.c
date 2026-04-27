/*
 * uiox_target/main.c
 *
 * Final integration main — boots the full UIOX stack on the
 * selected architecture:
 *
 *   arch_init()        — platform hw init (GIC/PIC, UART, timer)
 *   uiox_fs stack      — buf_init, inode_cache_init, sb_init, fs_mkfs
 *   uiox_dev stack     — clist_init, devsw_init
 *   uiox_hw stack      — already initialised by arch_init
 *   Integration test   — open device, write/read, irq dispatch,
 *                         tty exercise, login, DMA, context save
 *   arch_fini()        — platform teardown
 *
 * The same file compiles for all three architectures; the only
 * difference is which arch_defs.h and arch_init.c are linked in.
 */

 #include <stdio.h>
 #include <string.h>
 
 /* ── Architecture definitions (injected by build system) ──── */
 #if   defined(UIOX_ARCH_ARM64)
 #  include "arch/arm64/include/arch_defs.h"
 #elif defined(UIOX_ARCH_ARM32)
 #  include "arch/arm32/include/arch_defs.h"
 #elif defined(UIOX_ARCH_X86_64)
 #  include "arch/x86_64/include/arch_defs.h"
 #else
 #  include "10_Arch/x86_64/include/arch_defs.h"  /* default */
 #endif
 
 /* ── uiox_fs ─────────────────────────────────────────────── */
 #include "fs_types.h"
 #include "buffer.h"
 #include "inode.h"
 #include "superblock.h"
 #include "bmap.h"
 #include "namei.h"
 
 /* ── uiox_dev ────────────────────────────────────────────── */
 #include "dev_types.h"
 #include "clist.h"
 #include "devsw.h"
 #include "tty.h"
 #include "pty.h"
 
 /* ── uiox_hw ─────────────────────────────────────────────── */
 #include "hw_types.h"
 #include "mmio.h"
 #include "irq.h"
 #include "cpu.h"
 
 /* ── arch_init / arch_fini declared here ────────────────── */
 extern void arch_init(void);
 extern void arch_fini(void);
 
 /* =============================================================
  * Banner helper
  * ============================================================= */
 static void banner(const char *s)
 {
     printf("\n══════════════════════════════════════════\n");
     printf("  %s\n", s);
     printf("══════════════════════════════════════════\n");
 }
 
 /* =============================================================
  * main
  * ============================================================= */
 int main(void)
 {
     printf("╔══════════════════════════════════════════╗\n");
     printf("║  UIOX — Unified I/O and OS eXperiment   ║\n");
     printf("║  Target architecture: %-19s║\n", UIOX_ARCH_NAME);
     printf("╚══════════════════════════════════════════╝\n");
 
     /* ==========================================================
      * Stage 1: Architecture-specific hardware initialisation
      * ========================================================== */
     banner("Stage 1 — Platform / Hardware Init");
     arch_init();
 
     /* ==========================================================
      * Stage 2: uiox_fs subsystem
      * ========================================================== */
     banner("Stage 2 — Filesystem Init");
     buf_init();
     inode_cache_init();
     sb_init();
     fs_mkfs();
     sb_print();
 
     /* Create a test file */
     InCoreInode *root = iget(ROOT_INO);
     InCoreInode *f1   = ialloc(FT_REGULAR,
                                 PERM_UR|PERM_UW|PERM_GR|PERM_OR,
                                 0, 0);
     if (f1) {
         f1->nlink = 1;
         f1->flags |= IFLAG_CHANGED;
         iupdate(f1);
         dir_add(root, "test.txt", f1->ino);
         printf("[fs] created test.txt → ino=%u\n", f1->ino);
     }
 
     /* namei path lookup */
     InCoreInode *found = namei("/test.txt", NULL, 0, 0);
     if (found) {
         printf("[fs] namei('/test.txt') → ino=%u\n", found->ino);
         iput(found);
     }
 
     if (f1)   iput(f1);
     if (root) iput(root);
 
     /* ==========================================================
      * Stage 3: uiox_dev subsystem
      * ========================================================== */
     banner("Stage 3 — Device Driver Init");
     clist_init();
     devsw_init();
 
     /* Open console device */
     uint32_t con_dev  = DEV_MAKE(0, 0);
     uint32_t disk_dev = DEV_MAKE(0, 0);
 
     dev_open(con_dev,  DEV_O_RDWR, DEV_TYPE_CHAR);
     dev_open(disk_dev, DEV_O_RDWR, DEV_TYPE_BLOCK);
 
     /* Write to console */
     uint32_t pos = 0;
     dev_write(con_dev, DEV_TYPE_CHAR,
               "Hello from UIOX on " UIOX_ARCH_NAME "\n",
               (int)(19 + sizeof(UIOX_ARCH_NAME)), &pos);
 
     /* Block device strategy */
     BufHdr bp;
     memset(&bp, 0, sizeof bp);
     bp.b_dev    = disk_dev;
     bp.b_flags  = B_WRITE;
     bp.b_blkno  = 0;
     bp.b_bcount = BLOCK_SIZE;
     dev_strategy(&bp);
     printf("[dev] strategy B_DONE=%d\n", !!(bp.b_flags & B_DONE));
 
     /* ioctl */
     dev_ioctl(con_dev, TIOCSETCANON, 0);
 
     /* Close */
     dev_close(con_dev,  DEV_TYPE_CHAR,  0);
     dev_close(disk_dev, DEV_TYPE_BLOCK, 0);
 
     /* ==========================================================
      * Stage 4: TTY / login
      * ========================================================== */
     banner("Stage 4 — TTY / Login");
 
     tty_dev_t tty;
     tty_init(&tty, 0, 0);
     tty.td_open = 1;
 
     tty_write(&tty, "arch=" UIOX_ARCH_NAME "\n",
               (int)(5 + sizeof(UIOX_ARCH_NAME)));
 
     tty_receive_chars(&tty, "root\n", 5);
     char linebuf[64] = {0};
     int  n = tty_read(&tty, linebuf, 63);
     linebuf[n] = '\0';
     printf("[tty] read line: '%s'\n", linebuf);
 
     do_login(&tty);
 
     /* ==========================================================
      * Stage 5: uiox_hw — IRQ dispatch + DMA + context save
      * ========================================================== */
     banner("Stage 5 — Hardware Control");
 
     /* Simulate device interrupts */
     hw_context_t ctx;
     memset(&ctx, 0, sizeof ctx);
     ctx.pc = 0xCAFEBABECAFEBABEULL;
 
 #if defined(UIOX_ARCH_ARM64)
     irq_dispatch(UART0_IRQ,  &ctx);
     irq_dispatch(TIMER0_IRQ, &ctx);
 #elif defined(UIOX_ARCH_ARM32)
     irq_dispatch(UART0_IRQ,  &ctx);
     irq_dispatch(TIMER0_IRQ, &ctx);
 #else
     irq_dispatch(COM1_IRQ, &ctx);
     irq_dispatch(PIT_IRQ,  &ctx);
 #endif
 
     irq_print_table();
 
     /* DMA transfer */
     dma_desc_t descs[2];
     dma_desc_init(descs, 2);
     descs[0].dma_src   = PHYS_DRAM_BASE;
     descs[0].dma_dst   = PHYS_DRAM_BASE + 0x1000;
     descs[0].dma_len   = 512;
     descs[0].dma_flags = DMA_FLAG_IRQ;
     descs[1].dma_src   = PHYS_DRAM_BASE + 0x0200;
     descs[1].dma_dst   = PHYS_DRAM_BASE + 0x1200;
     descs[1].dma_len   = 512;
     descs[1].dma_flags = DMA_FLAG_LAST | DMA_FLAG_IRQ;
     dma_submit(MMIO_DMA_BASE, descs, 2);
     dma_poll_done(descs, 2, 100000);
     dma_print(descs, 2);
 
     /* CPU context save/restore (setjmp/longjmp model) */
     banner("Stage 5b — CPU Context Save/Restore");
     hw_context_t saved;
     memset(&saved, 0, sizeof saved);
 
     if (cpu_context_save(&saved) == 0) {
         printf("[main] first entry — driver open simulation\n");
         cpu_context_restore(&saved);
     } else {
         printf("[main] context restored — "
                "file-table/inode decremented\n");
     }
 
     /* ==========================================================
      * Stage 6: STREAMS / PTY
      * ========================================================== */
     banner("Stage 6 — STREAMS / PTY Multiplexer");
 
     mpx_state_t mpx;
     mpx_init(&mpx);
     mpx.mx_phys_open = 1;
 
     int idx = pty_alloc(&mpx);
     if (idx >= 0) {
         pty_slave_write(&mpx.mx_pairs[idx],
                         "window output on " UIOX_ARCH_NAME "\n",
                         (int)(17 + sizeof(UIOX_ARCH_NAME)));
         pty_print(&mpx);
         pty_free(&mpx, idx);
     }
     mpx_loop(&mpx);
 
     /* ==========================================================
      * Stage 7: buf_sync + final state
      * ========================================================== */
     banner("Stage 7 — Sync and Final State");
     buf_sync();
     buf_print();
     sb_print();
     clist_print_pool();
 
     /* ==========================================================
      * Stage 8: Architecture teardown
      * ========================================================== */
     banner("Stage 8 — Platform Teardown");
     arch_fini();
 
     printf("\n[main] UIOX on %s — all stages complete\n",
            UIOX_ARCH_NAME);
     return 0;
 }
 