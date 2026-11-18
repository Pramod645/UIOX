/*
 * 50_UIX/21_apps/app_gpu.c
 *
 * GPU application — EL0.  Observes the GPU; cannot submit work.
 *
 *   21_apps/app_gpu.c → dev/gpu/ → dev/uiox_dev_reg.c → uix_ioctl()
 *      → sys_ioctl → SCFS table → the GPU driver
 *
 * No 30_KIX header.
 *
 * ── FOUR READS, NO WRITE ────────────────────────────────────────────
 *   GET_INFO 0x01 · GET_STATS 0x02 · GET_WINDOW 0x03 · GET_VRAM 0x04
 *
 * The driver's submit path (uiox_gpu_if_submit) takes a command buffer
 * pointer and a completion callback; neither crosses a trap, and no
 * ioctl carries a command stream.  So this app reports what the GPU is
 * and where its memory is, and draws nothing.  Saying so here beats
 * printing a frame count that could never increase.
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "gpu/uiox_dev_gpu.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 /* 64-bit decimal without a long-long format: split into two 32-bit
  * halves.  uix_printf's formatter handles %u but this keeps the code
  * independent of %llu support. */
 static void print_u64(const char *label, unsigned long long v)
 {
     unsigned int hi = (unsigned int)(v >> 32);
     unsigned int lo = (unsigned int)(v & 0xFFFFFFFFull);
     if (hi != 0u) uix_printf("%s%u%09u", label, hi, lo);
     else          uix_printf("%s%u",     label, lo);
 }
 
 int app_gpu_main(void)
 {
     uiox_dev_gpu_t       g;
     uiox_dev_gpu_info_t  info;
     uiox_dev_gpu_stats_t st;
     uiox_dev_gpu_vram_t  vr;
     uiox_dev_err_t       rc;
     unsigned long        window = 0u;
 
     uix_printf("gpu: opening /dev/gpu\n");
 
     rc = uiox_dev_gpu_open(&g, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("gpu: open failed (%d) — no /dev/gpu.\n", (int)rc);
         uix_printf("     Is the node created, and did uiox_gpu_probe() run?\n");
         return 1;
     }
 
     /* ── GET_INFO ────────────────────────────────────────────────── */
     if (uiox_dev_gpu_get_info(&g, &info) == UIOX_DEV_OK) {
         uix_printf("gpu: mmio   0x%08X (low 32 bits of base)\n",
                    (unsigned int)(info.base & 0xFFFFFFFFu));
         uix_printf("gpu: irq    job=%u mmu=%u err=%u\n",
                    info.irq_job, info.irq_mmu, info.irq_err);
     } else {
         uix_printf("gpu: GET_INFO failed\n");
     }
 
     /* ── GET_VRAM ────────────────────────────────────────────────── */
     if (uiox_dev_gpu_get_vram(&g, &vr) == UIOX_DEV_OK) {
         print_u64("gpu: vram size  ", vr.size);
         uix_printf(" bytes\n");
         print_u64("gpu: vram used  ", vr.used);
         uix_printf(" bytes\n");
         print_u64("gpu: vram free  ", vr.free_bytes);
         uix_printf(" bytes\n");
     } else {
         uix_printf("gpu: GET_VRAM failed\n");
     }
 
     /* ── GET_WINDOW ──────────────────────────────────────────────── */
     if (uiox_dev_gpu_get_window(&g, &window) == UIOX_DEV_OK)
         uix_printf("gpu: window 0x%08X (low 32 bits)\n",
                    (unsigned int)(window & 0xFFFFFFFFu));
     else
         uix_printf("gpu: GET_WINDOW failed\n");
 
     /* ── GET_STATS ───────────────────────────────────────────────── */
     if (uiox_dev_gpu_get_stats(&g, &st) == UIOX_DEV_OK) {
         print_u64("gpu: jobs submitted ", st.jobs_submitted);
         uix_printf("\n");
         print_u64("gpu: jobs completed ", st.jobs_completed);
         uix_printf("\n");
         print_u64("gpu: fence waits   ", st.fence_waits);
         uix_printf("\n");
         print_u64("gpu: gpu faults    ", st.gpu_faults);
         uix_printf("\n");
     } else {
         uix_printf("gpu: GET_STATS failed\n");
     }
 
     uix_printf("gpu: NOTE — no submit command exists at EL0.\n");
     uix_printf("     uiox_gpu_if_submit() is kernel-side; drawing needs\n");
     uix_printf("     a mapped ring, a doorbell and a pollable fence.\n");
 
     (void)uiox_dev_gpu_close(&g);
     uix_printf("gpu: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_gpu_main();
 }

