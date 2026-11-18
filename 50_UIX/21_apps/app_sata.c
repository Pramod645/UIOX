/*
 * 50_UIX/21_apps/app_sata.c
 *
 * SATA application — EL0.  Reads identity, then does one transfer.
 *
 *   21_apps/app_sata.c → dev/sata/ → dev/uiox_dev_block.c (shape)
 *      → sys_read / sys_write / sys_lseek → SCFS table → the SATA driver
 *
 * No 30_KIX header.
 *
 * ── NO_DEV IS NOT A FAILURE ─────────────────────────────────────────
 * A SATA port with nothing attached is a normal condition.  The state
 * enum has NO_DEV for it, and an app should say so rather than treat
 * an empty port as an error.  This one does.
 *
 * ── WHAT IT CANNOT DO ───────────────────────────────────────────────
 *   smart_read(buf)   no ioctl among the nine
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "sata/uiox_dev_sata.h"
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
 
 int app_sata_main(void)
 {
     uiox_dev_sata_t       s;
     uiox_dev_sata_ident_t id;
     uiox_dev_err_t        rc;
     unsigned int          i, blocks = 0u;
     unsigned long long    bytes = 0ull;
     int                   is_ssd = 0;
 
     uix_printf("sata: opening /dev/sata\n");
 
     rc = uiox_dev_sata_open(&s, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("sata: open failed (%d) — no /dev/sata.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_sata_probe() run?\n");
         return 1;
     }
 
     /* ── one GET_IDENT: the whole identity in a single call ──────── */
     if (uiox_dev_sata_get_ident(&s, &id) != UIOX_DEV_OK) {
         uix_printf("sata: GET_IDENT failed\n");
         (void)uiox_dev_sata_close(&s);
         return 1;
     }
 
     /* A zero capacity means the port is empty — NO_DEV — which is a
      * normal state, not an error.  Say so and stop. */
     if (id.capacity_bytes == 0ull) {
         uix_printf("sata: no device attached on this port (NO_DEV)\n");
         uix_printf("      A SATA port with nothing attached is normal.\n");
         (void)uiox_dev_sata_close(&s);
         return 0;
     }
 
     uix_printf("sata: model  %.40s\n", id.model_str);
     uix_printf("sata: serial %.20s\n", id.serial_str);
     uix_printf("sata: fw     %.8s\n",  id.fw_str);
     print_u64("sata: bytes   ", id.capacity_bytes);
     uix_printf("\n");
     print_u64("sata: sectors ", id.lba48_sectors);
     uix_printf(" (48-bit LBA)\n");
     uix_printf("sata: media  %s  rpm=%u  ncq_depth=%u\n",
                id.is_ssd ? "SSD" : "HDD", id.rpm, id.ncq_depth);
     uix_printf("sata: trim=%s  smart=%s\n",
                id.trim_supported ? "YES" : "no",
                id.smart_supported ? "YES" : "no");
 
     /* ── the single-value commands ───────────────────────────────── */
     if (uiox_dev_sata_get_media(&s, &is_ssd) == UIOX_DEV_OK)
         uix_printf("sata: GET_MEDIA 0xE5 → %s\n", is_ssd ? "SSD" : "HDD");
 
     if (uiox_dev_sata_get_bytes(&s, &bytes) == UIOX_DEV_OK) {
         print_u64("sata: GET_BYTES  ", bytes);
         uix_printf("\n");
     }
     if (uiox_dev_sata_get_blocks(&s, &blocks) == UIOX_DEV_OK)
         uix_printf("sata: GET_BLOCKS %u x %u bytes\n",
                    blocks, UIOX_DEV_SATA_SECTOR_SIZE);
 
     /* ── one positioned transfer ─────────────────────────────────── */
     {
         unsigned char sec[UIOX_DEV_SATA_SECTOR_SIZE];
         uix_ssize_t   got;
 
         for (i = 0u; i < sizeof sec; i++) sec[i] = 0u;
 
         got = uiox_dev_sata_pread(&s, sec, sizeof sec, 0ull);
         if (got == (uix_ssize_t)sizeof sec) {
             uix_printf("sata: pread 512 B at LBA 0 — first bytes "
                        "%02X %02X %02X %02X\n",
                        sec[0], sec[1], sec[2], sec[3]);
         } else {
             uix_printf("sata: pread returned %d (expected 512)\n", (int)got);
         }
 
         got = uiox_dev_sata_pwrite(&s, sec, sizeof sec, 0ull);
         uix_printf("sata: pwrite 512 B at LBA 0 -> %s\n",
                    (got == (uix_ssize_t)sizeof sec) ? "ok" : "failed");
     }
 
     /* ── the writes no other class has ───────────────────────────── */
     if (uiox_dev_sata_flush(&s) == UIOX_DEV_OK)
         uix_printf("sata: FLUSH 0xF1 ok\n");
 
     if (uiox_dev_sata_get_media(&s, &is_ssd) == UIOX_DEV_OK && is_ssd) {
         /* TRIM is meaningless on rotating media, and the ident says
          * whether this is one. */
         if (uiox_dev_sata_trim(&s, 0ull, 1u) == UIOX_DEV_OK)
             uix_printf("sata: TRIM 0xF4 ok (1 sector at LBA 0)\n");
         else
             uix_printf("sata: TRIM rejected (or its argument shape differs)\n");
     } else {
         uix_printf("sata: TRIM skipped — rotating media\n");
     }
 
     if (uiox_dev_sata_refresh(&s) == UIOX_DEV_OK)
         uix_printf("sata: REFRESH 0xF2 ok (IDENTIFY re-run)\n");
 
     /* ── stats ───────────────────────────────────────────────────── */
     {
         unsigned char stats[128];
         for (i = 0u; i < sizeof stats; i++) stats[i] = 0u;
         if (uiox_dev_sata_get_stats(&s, stats) == UIOX_DEV_OK)
             uix_printf("sata: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("sata: GET_STATS failed\n");
     }
 
     uix_printf("sata: NOTE — smart_read has no command; STANDBY was not\n");
     uix_printf("      called here because it spins the disk down.\n");
 
     (void)uiox_dev_sata_close(&s);
     uix_printf("sata: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_sata_main();
 }
 