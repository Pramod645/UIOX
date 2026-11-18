/*
 * 50_UIX/21_apps/app_nvme.c
 *
 * NVMe application — EL0.  Reads identity, then does one transfer.
 *
 *   21_apps/app_nvme.c → dev/nvme/ → dev/uiox_dev_block.c (shape)
 *      → sys_read / sys_write / sys_lseek → SCFS table → the NVMe driver
 *
 * No 30_KIX header.
 *
 * ── IDENTITY + ONE TRANSFER ─────────────────────────────────────────
 * The device layer also exposes smart_log and format_ns, and reading a
 * smart log is not something this app can do: the vocabulary has no
 * command for it.  Same for ns_info — no command returns a namespace
 * descriptor, only its id.
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "nvme/uiox_dev_nvme.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 
 static void print_u64(const char *label, unsigned long long v)
 {
     unsigned int hi = (unsigned int)(v >> 32);
     unsigned int lo = (unsigned int)(v & 0xFFFFFFFFull);
     uix_printf("%s", label);
     if (hi != 0u) uix_printf("%u%09u", hi, lo);
     else          uix_printf("%u", lo);
 }
 
 int app_nvme_main(void)
 {
     uiox_dev_nvme_t       n;
     uiox_dev_nvme_ident_t id;
     uiox_dev_err_t        rc;
     unsigned int          i, blocks = 0u, nsid = 0u;
     unsigned long long    bytes = 0ull;
 
     uix_printf("nvme: opening /dev/nvme\n");
 
     rc = uiox_dev_nvme_open(&n, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("nvme: open failed (%d) — no /dev/nvme.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_nvme_probe() run?\n");
         return 1;
     }
 
     /* ── one GET_IDENT: model, serial, firmware, nsid, capacity ──── */
     if (uiox_dev_nvme_get_ident(&n, &id) != UIOX_DEV_OK) {
         uix_printf("nvme: GET_IDENT failed\n");
         (void)uiox_dev_nvme_close(&n);
         return 1;
     }
 
     uix_printf("nvme: model  %.40s\n", id.model);
     uix_printf("nvme: serial %.20s\n", id.serial);
     uix_printf("nvme: fw     %.8s\n",  id.fw_rev);
     uix_printf("nvme: nsid   %u\n",    id.nsid);
     print_u64("nvme: lba cap ", id.capacity_lba);
     uix_printf(" LBAs\n");
     print_u64("nvme: bytes   ", id.capacity_bytes);
     uix_printf(" bytes\n");
     uix_printf("nvme: wcache %s  trim=%s\n",
                id.volatile_wc ? "YES" : "no",
                id.trim_supported ? "YES" : "no");
 
     /* ── the single-value commands ───────────────────────────────── */
     if (uiox_dev_nvme_get_ns(&n, &nsid) == UIOX_DEV_OK)
         uix_printf("nvme: GET_NS 0xE6 → active nsid %u\n", nsid);
 
     if (uiox_dev_nvme_get_bytes(&n, &bytes) == UIOX_DEV_OK) {
         print_u64("nvme: GET_BYTES  ", bytes);
         uix_printf("\n");
     }
     if (uiox_dev_nvme_get_blocks(&n, &blocks) == UIOX_DEV_OK)
         uix_printf("nvme: GET_BLOCKS %u blocks\n", blocks);
 
     /* ── one positioned transfer — the point of this class ───────── */
     {
         unsigned char sec[512];
         uix_ssize_t   got;
 
         for (i = 0u; i < sizeof sec; i++) sec[i] = 0u;
 
         got = uiox_dev_nvme_pread(&n, sec, sizeof sec, 0ull);
         if (got == (uix_ssize_t)sizeof sec) {
             uix_printf("nvme: pread 512 B at LBA 0 — first bytes "
                        "%02X %02X %02X %02X\n",
                        sec[0], sec[1], sec[2], sec[3]);
         } else {
             uix_printf("nvme: pread returned %d (expected 512)\n", (int)got);
         }
 
         /* Write the same sector back unchanged: exercises the write
          * path without altering the device's contents. */
         got = uiox_dev_nvme_pwrite(&n, sec, sizeof sec, 0ull);
         uix_printf("nvme: pwrite 512 B at LBA 0 -> %s\n",
                    (got == (uix_ssize_t)sizeof sec) ? "ok" : "failed");
     }
 
     /* ── the two controller-level commands ───────────────────────── */
     if (uiox_dev_nvme_flush(&n) == UIOX_DEV_OK)
         uix_printf("nvme: FLUSH 0xF2 ok (write cache flushed)\n");
     else
         uix_printf("nvme: FLUSH failed\n");
 
     if (uiox_dev_nvme_refresh(&n) == UIOX_DEV_OK)
         uix_printf("nvme: REFRESH 0xF3 ok (identify re-run)\n");
     else
         uix_printf("nvme: REFRESH failed\n");
 
     /* ── stats ───────────────────────────────────────────────────── */
     {
         unsigned char stats[128];
         for (i = 0u; i < sizeof stats; i++) stats[i] = 0u;
         if (uiox_dev_nvme_get_stats(&n, stats) == UIOX_DEV_OK)
             uix_printf("nvme: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("nvme: GET_STATS failed\n");
     }
 
     uix_printf("nvme: NOTE — smart_log and format_ns have no command,\n");
     uix_printf("      and no command returns a namespace descriptor.\n");
 
     (void)uiox_dev_nvme_close(&n);
     uix_printf("nvme: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_nvme_main();
 }
 