/*
 * main.c — uiox_hw demonstration
 *
 * Exercises the entire hardware control layer in the same
 * banner-driven style as uiox_fs/main.c and uiox_dev/main.c:
 *
 *   MMIO init / region registration
 *   MMIO 8/16/32/64-bit read/write
 *   x86_64 port I/O  (inb/outb)
 *   Memory barriers
 *   DMA descriptor chain
 *   IRQ table init / request / dispatch / free
 *   Timer init / tick simulation
 *   CPU context save / restore  (setjmp / longjmp equivalent)
 *   CPU interrupt enable / disable / restore
 *   CPU identification
 *   Architecture-specific register access
 */

 #include <stdio.h>
 #include <string.h>
 #include "hw_types.h"
 #include "mmio.h"
 #include "irq.h"
 #include "cpu.h"
 
 static void banner(const char *s)
 {
     printf("\n══════════════════════════════════════════\n");
     printf("  %s\n", s);
     printf("══════════════════════════════════════════\n");
 }
 
 /* =============================================================
  * Sample IRQ handlers
  * ============================================================= */
 static void uart_irq_handler(int irq, hw_context_t *ctx, void *dev_id)
 {
     (void)ctx; (void)dev_id;
     printf("  [uart_irq] IRQ%d: UART data ready\n", irq);
 
     /* Read received byte from UART data register (MMIO) */
     uint8_t ch = mmio_read8(MMIO_UART_BASE + 0x00);
     printf("  [uart_irq] received byte: 0x%02x ('%c')\n",
            ch, ch >= 0x20 && ch < 0x7F ? ch : '.');
 }
 
 static void disk_irq_handler(int irq, hw_context_t *ctx, void *dev_id)
 {
     (void)ctx; (void)dev_id;
     printf("  [disk_irq] IRQ%d: disk I/O complete\n", irq);
 
     /* Read status register from disk controller */
     uint32_t status = mmio_read32(MMIO_DISK_BASE + 0x04);
     printf("  [disk_irq] disk status: 0x%08x\n", status);
 }
 
 /* =============================================================
  * main
  * ============================================================= */
 int main(void)
 {
     /* ── Initialise subsystems ──────────────────────────────── */
     banner("Hardware Control Layer Init");
     printf("[hw] target architecture: %s\n", UIOX_ARCH_NAME);
 
     mmio_init();
     irq_init();
 
     /* ── MMIO read / write ──────────────────────────────────── */
     banner("MMIO Register Access  (8/16/32/64-bit)");
 
     /* Write UART line control register (8-bit) */
     mmio_write8(MMIO_UART_BASE + 0x0C, 0x03);  /* 8N1            */
     uint8_t lcr = mmio_read8(MMIO_UART_BASE + 0x0C);
     printf("[main] UART LCR = 0x%02x\n", lcr);
 
     /* Write timer load value (32-bit) */
     mmio_write32(MMIO_TIMER_BASE + 0x00, 10000u);
     uint32_t load = mmio_read32(MMIO_TIMER_BASE + 0x00);
     printf("[main] TIMER LOAD = %u\n", load);
 
     /* Write GIC distributor enable (32-bit) */
     mmio_write32(MMIO_INTC_BASE + 0x000, 0x1u);
 
     /* 64-bit DMA address register */
     mmio_write64(MMIO_DMA_BASE + 0x00, 0x0000000100002000ULL);
     uint64_t dma_addr = mmio_read64(MMIO_DMA_BASE + 0x00);
     printf("[main] DMA base addr = 0x%016llx\n",
            (unsigned long long)dma_addr);
 
     /* ── x86_64 port I/O ────────────────────────────────────── */
     banner("x86_64 Port I/O  (inb / outb)");
 
     port_outb(0x3F8, 'H');   /* COM1 data register               */
     port_outb(0x3F8, 'i');
     uint8_t com1 = port_inb(0x3F8);
     printf("[main] COM1 inb = 0x%02x\n", com1);
 
     /* ── Memory barriers ────────────────────────────────────── */
     banner("Memory Barriers  (mb / rmb / wmb / isb / dsb)");
 
     hw_mb();
     hw_rmb();
     hw_wmb();
     cpu_isb();
     cpu_dsb();
     printf("[main] all barriers executed\n");
 
     /* ── DMA descriptor chain ───────────────────────────────── */
     banner("DMA Descriptor Chain");
 
     dma_desc_t descs[3];
     dma_desc_init(descs, 3);
 
     descs[0].dma_src   = 0x80001000;
     descs[0].dma_dst   = 0x90001000;
     descs[0].dma_len   = 512;
     descs[0].dma_flags = DMA_FLAG_IRQ;
 
     descs[1].dma_src   = 0x80001200;
     descs[1].dma_dst   = 0x90001200;
     descs[1].dma_len   = 512;
     descs[1].dma_flags = 0;
 
     descs[2].dma_src   = 0x80001400;
     descs[2].dma_dst   = 0x90001400;
     descs[2].dma_len   = 256;
     descs[2].dma_flags = DMA_FLAG_LAST | DMA_FLAG_IRQ;
 
     dma_submit(MMIO_DMA_BASE, descs, 3);
     dma_poll_done(descs, 3, 1000000);
     dma_print(descs, 3);
 
     /* ── IRQ request / dispatch ─────────────────────────────── */
     banner("IRQ Vector Table — request / enable / dispatch");
 
     irq_request(IRQ_UART,  uart_irq_handler, NULL, "uart");
     irq_request(IRQ_DISK,  disk_irq_handler, NULL, "disk");
     irq_enable (IRQ_UART);
     irq_enable (IRQ_DISK);
 
     /* Simulate UART interrupt: put a byte in the UART RX register */
     mmio_write8(MMIO_UART_BASE + 0x00, 'A');
 
     /* Simulate interrupt dispatch (in real hw: triggered by hardware) */
     hw_context_t ctx;
     memset(&ctx, 0, sizeof ctx);
     ctx.pc = 0x0000000000401000ULL;   /* simulated interrupted PC   */
     ctx.sp = 0x0000FFFF0000ULL;
 
     irq_dispatch(IRQ_UART, &ctx);
     irq_dispatch(IRQ_DISK, &ctx);
     irq_dispatch(99,        &ctx);    /* spurious IRQ               */
 
     irq_print_table();
 
     /* ── Timer ──────────────────────────────────────────────── */
     banner("Timer — init + simulated ticks");
 
     timer_init(100);   /* 100 Hz */
 
     /* Simulate several timer ticks */
     int i;
     for (i = 0; i < 5; i++) {
         memset(&ctx, 0, sizeof ctx);
         irq_dispatch(IRQ_TIMER, &ctx);
     }
     printf("[main] jiffies = %llu\n",
            (unsigned long long)timer_jiffies());
 
     /* ── IRQ enable / disable / restore ────────────────────── */
     banner("CPU IRQ Enable / Disable / Restore");
 
     printf("[main] IRQs enabled: %d\n", cpu_irq_enabled());
     uint64_t flags = cpu_irq_disable();
     printf("[main] IRQs after disable: %d\n", cpu_irq_enabled());
     cpu_irq_restore(flags);
     printf("[main] IRQs after restore: %d\n", cpu_irq_enabled());
 
     /* ── CPU context save / restore  (setjmp equivalent) ────── */
     banner("CPU Context Save / Restore  (device-open setjmp)");
 
     hw_context_t saved_ctx;
     memset(&saved_ctx, 0, sizeof saved_ctx);
 
     /*
      * This models the device-open algorithm's setjmp usage:
      *
      *   save context in case of long jump from driver;
      *   call driver open;
      *   if open fails in driver → longjmp back here;
      *   decrement file table, inode counts;
      */
     if (cpu_context_save(&saved_ctx) == 0) {
         printf("[main] first entry — simulating driver open\n");
 
         /* Simulate a driver that fails and calls longjmp */
         int simulate_failure = 1;
         if (simulate_failure) {
             printf("[main] driver open failed — restoring context\n");
             cpu_context_restore(&saved_ctx);
             /* does not return */
         }
     } else {
         /* Returned from cpu_context_restore (longjmp return) */
         printf("[main] returned from context restore — "
                "decrement file table and inode counts\n");
     }
 
     /* ── Architecture-specific registers ───────────────────── */
     banner("Architecture-Specific Register Access");
 
 #if defined(UIOX_ARCH_ARM64)
     printf("[main] ARM64 DAIF     = 0x%016llx\n",
            (unsigned long long)arm64_read_daif());
     printf("[main] ARM64 MPIDR    = 0x%016llx\n",
            (unsigned long long)arm64_read_mpidr());
     printf("[main] ARM64 CNTPCT   = 0x%016llx\n",
            (unsigned long long)arm64_read_cntpct());
 
 #elif defined(UIOX_ARCH_ARM32)
     printf("[main] ARM32 CPSR     = 0x%08x\n",
            arm32_read_cpsr());
 
 #elif defined(UIOX_ARCH_X86_64)
     printf("[main] x86_64 RFLAGS  = 0x%016llx\n",
            (unsigned long long)x86_read_rflags());
     printf("[main] x86_64 CPUID   = 0x%08x\n",
            x86_cpuid_family());
 #endif
 
     /* ── CPU identification ─────────────────────────────────── */
     banner("CPU Identification");
 
     cpu_info_t info;
     cpu_identify(&info);
     cpu_print_info(&info);
 
     /* ── IRQ free ───────────────────────────────────────────── */
     banner("IRQ Free");
 
     irq_free(IRQ_UART);
     irq_free(IRQ_DISK);
     irq_free(IRQ_TIMER);
     irq_print_table();
 
     return 0;
 }
 