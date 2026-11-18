/*
 * 50_UIX/21_apps/app_emmc.c
 *
 * eMMC application — EL0.  Reads the device's identity, then transfers.
 *
 *   21_apps/app_emmc.c → dev/emmc/ → dev/uiox_dev_block.c (shape)
 *      → sys_read / sys_write / sys_lseek → SCFS table → the eMMC driver
 *
 * No 30_KIX header.
 *
 * ── THE BLOCK SHAPE, FINALLY USED ───────────────────────────────────
 * uiox_dev_block has been in the archive since early in this stretch
 * with no device behind it.  eMMC is its first: a byte-addressable
 * device you position within and transfer to.
 *
 * ── WHAT IT CANNOT DO ───────────────────────────────────────────────
 *   flush / trim / bkops / health   no command among the ten
 *   (the health counters do arrive — inside GET_IDENT)
 *
 * v1.0.0  @date 2026-11-18
 */

 #include "emmc/uiox_dev_emmc.h"
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
 
 static const char *part_name(int p)
 {
     switch (p) {
     case UIOX_DEV_EMMC_PART_USER:  return "USER";
     case UIOX_DEV_EMMC_PART_BOOT1: return "BOOT1";
     case UIOX_DEV_EMMC_PART_BOOT2: return "BOOT2";
     case UIOX_DEV_EMMC_PART_RPMB:  return "RPMB";
     case UIOX_DEV_EMMC_PART_GP1:   return "GP1";
     case UIOX_DEV_EMMC_PART_GP2:   return "GP2";
     case UIOX_DEV_EMMC_PART_GP3:   return "GP3";
     case UIOX_DEV_EMMC_PART_GP4:   return "GP4";
     default:                       return "?";
     }
 }
 
 static const char *eol_name(unsigned char p)
 {
     switch (p) {
     case UIOX_DEV_EMMC_EOL_NORMAL:  return "NORMAL";
     case UIOX_DEV_EMMC_EOL_WARNING: return "WARNING";
     case UIOX_DEV_EMMC_EOL_URGENT:  return "URGENT";
     default:                        return "UNKNOWN";
     }
 }
 
 int app_emmc_main(void)
 {
     uiox_dev_emmc_t        e;
     uiox_dev_emmc_ident_t  id;
     uiox_dev_emmc_part_info_t pi;
     uiox_dev_err_t         rc;
     unsigned int           i, blocks = 0u;
     unsigned long long     bytes = 0ull;
 
     uix_printf("emmc: opening /dev/emmc\n");
 
     rc = uiox_dev_emmc_open(&e, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("emmc: open failed (%d) — no /dev/emmc.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_emmc_probe() run?\n");
         return 1;
     }
 
     /* ── one GET_IDENT carries identity, health and the partition table */
     if (uiox_dev_emmc_get_ident(&e, &id) != UIOX_DEV_OK) {
         uix_printf("emmc: GET_IDENT failed\n");
         (void)uiox_dev_emmc_close(&e);
         return 1;
     }
 
     uix_printf("emmc: product %.6s  fw=0x%02X  mfr=0x%02X\n",
                id.product_name, id.fwrev, id.manufacturer_id);
     print_u64("emmc: capacity ", id.capacity_bytes);
     uix_printf(" bytes (");
     print_u64("", id.capacity_sectors);
     uix_printf(" sectors)\n");
     uix_printf("emmc: cache %u KB  trim=%s  bkops=%s\n",
                id.cache_size_kb,
                id.trim_supported ? "YES" : "no",
                id.bkops_supported ? "YES" : "no");
     uix_printf("emmc: pre-EOL %s  life A=%u/10  B=%u/10\n",
                eol_name(id.pre_eol_info), id.life_est_a, id.life_est_b);
 
     uix_printf("emmc: partitions\n");
     for (i = 0u; i < UIOX_DEV_EMMC_MAX_PARTS; i++) {
         if (!id.parts[i].enabled) continue;
         uix_printf("  [%u] %-6s ", i, part_name(id.parts[i].id));
         print_u64("", id.parts[i].size_bytes >> 10u);
         uix_printf(" KB\n");
     }
 
     /* ── the two single-value geometry commands ──────────────────── */
     if (uiox_dev_emmc_get_bytes(&e, &bytes) == UIOX_DEV_OK) {
         print_u64("emmc: GET_BYTES  ", bytes);
         uix_printf("\n");
     }
     if (uiox_dev_emmc_get_blocks(&e, &blocks) == UIOX_DEV_OK)
         uix_printf("emmc: GET_BLOCKS %u x %u bytes\n",
                    blocks, UIOX_DEV_EMMC_BLOCK_SIZE);
 
     /* ── GET_PART — the currently active partition's descriptor ──── */
     if (uiox_dev_emmc_get_part(&e, &pi) == UIOX_DEV_OK) {
         uix_printf("emmc: active part %s  enabled=%s\n",
                    part_name(pi.id), pi.enabled ? "YES" : "no");
     }
 
     /* ── the block shape: a positioned read, then a write back ────── */
     {
         unsigned char sec[UIOX_DEV_EMMC_BLOCK_SIZE];
         uix_ssize_t   n;
 
         for (i = 0u; i < sizeof sec; i++) sec[i] = 0u;
 
         n = uiox_dev_emmc_pread(&e, sec, sizeof sec, 0ull);
         if (n == (uix_ssize_t)sizeof sec) {
             uix_printf("emmc: pread 512 B at 0 — first bytes "
                        "%02X %02X %02X %02X\n",
                        sec[0], sec[1], sec[2], sec[3]);
         } else {
             uix_printf("emmc: pread returned %d (expected 512)\n", (int)n);
         }
 
         /* Write the same sector back unchanged: it exercises the write
          * path without altering the device's contents. */
         n = uiox_dev_emmc_pwrite(&e, sec, sizeof sec, 0ull);
         uix_printf("emmc: pwrite 512 B at 0 -> %s\n",
                    (n == (uix_ssize_t)sizeof sec) ? "ok" : "failed");
     }
 
     /* ── stats, opaque kernel type passed by address ─────────────── */
     {
         unsigned char stats[128];
         for (i = 0u; i < sizeof stats; i++) stats[i] = 0u;
         if (uiox_dev_emmc_get_stats(&e, stats) == UIOX_DEV_OK)
             uix_printf("emmc: stats read (%u bytes)\n",
                        (unsigned int)sizeof stats);
         else
             uix_printf("emmc: GET_STATS failed\n");
     }
 
     uix_printf("emmc: NOTE — flush/trim/bkops/health have no command.\n");
     uix_printf("      The health counters arrive inside GET_IDENT instead.\n");
 
     (void)uiox_dev_emmc_close(&e);
     uix_printf("emmc: closed\n");
     return 0;
 }
 
 int main(void)
 {
     return app_emmc_main();
 }
 