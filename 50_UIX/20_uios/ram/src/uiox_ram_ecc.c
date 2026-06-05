/**
 * @file    uiox_ram_ecc.c
 * @brief   UIOX RAM ECC engine implementation.
 * @date    2026-06-03
 */

 #include "uiox_ram_ecc.h"
 #include <string.h>
 #include <stdio.h>
 #include <errno.h>
 
 int uiox_ram_ecc_init(uiox_ram_ecc_t *ecc, uiox_ram_if_t *rif)
 {
     if (!ecc || !rif) return -EINVAL;
     memset(ecc, 0, sizeof(*ecc));
     ecc->rif              = rif;
     ecc->scrub_chunk_bytes= 64u * 1024u;  /* 64 KB per tick */
     return 0;
 }
 
 int uiox_ram_ecc_start_scrub(uiox_ram_ecc_t *ecc,
                               uint64_t phys_start, uint64_t size,
                               uint32_t chunk_bytes)
 {
     if (!ecc || !size) return -EINVAL;
     ecc->scrub_pos          = phys_start;
     ecc->scrub_end          = phys_start + size;
     ecc->scrub_chunk_bytes  = chunk_bytes ? chunk_bytes : 64u * 1024u;
     ecc->scrub_running      = true;
     return 0;
 }
 
 void uiox_ram_ecc_poll(uiox_ram_ecc_t *ecc)
 {
     if (!ecc || !ecc->rif) return;
     uint32_t ce = 0, ue = 0;
     uint64_t addr = 0;
     int rc = uiox_ram_hw_ecc_status(ecc->rif->hw, &ce, &ue, &addr);
     if (rc < 0) return;
 
     if (ce > ecc->total_ce) {
         uint8_t idx = ecc->log_head % UIOX_RAM_ECC_LOG_SIZE;
         ecc->log[idx].type     = UIOX_RAM_ECC_CORRECTABLE;
         ecc->log[idx].addr     = addr;
         ecc->log[idx].syndrome = 0u;
         ecc->total_ce          = ce;
         ecc->log_head++;
     }
     if (ue > ecc->total_ue) {
         uint8_t idx = ecc->log_head % UIOX_RAM_ECC_LOG_SIZE;
         ecc->log[idx].type = UIOX_RAM_ECC_UNCORRECTABLE;
         ecc->log[idx].addr = addr;
         ecc->total_ue      = ue;
         ecc->log_head++;
     }
 }
 
 void uiox_ram_ecc_tick(uiox_ram_ecc_t *ecc, uint32_t now_ms)
 {
     if (!ecc) return;
     (void)now_ms;
     uiox_ram_ecc_poll(ecc);
 
     /* Background scrub */
     if (ecc->scrub_running && ecc->scrub_pos < ecc->scrub_end) {
         uint64_t chunk = ecc->scrub_chunk_bytes;
         if (ecc->scrub_pos + chunk > ecc->scrub_end)
             chunk = ecc->scrub_end - ecc->scrub_pos;
         uiox_ram_hw_ecc_scrub(ecc->rif->hw, ecc->scrub_pos, chunk);
         ecc->scrub_pos += chunk;
         if (ecc->scrub_pos >= ecc->scrub_end)
             ecc->scrub_running = false;
     }
 }
 
 void uiox_ram_ecc_print_log(const uiox_ram_ecc_t *ecc)
 {
     if (!ecc) return;
     printf("  ECC log  CE=%u  UE=%u\n", ecc->total_ce, ecc->total_ue);
     uint8_t n = ecc->log_head < UIOX_RAM_ECC_LOG_SIZE ?
                 ecc->log_head : UIOX_RAM_ECC_LOG_SIZE;
     for (uint8_t i = 0; i < n; i++) {
         const uiox_ram_ecc_entry_t *e = &ecc->log[i];
         printf("    [%2u] %s  addr=0x%016llX\n",
                i,
                e->type == UIOX_RAM_ECC_CORRECTABLE ?
                "CE" : "UE",
                (unsigned long long)e->addr);
     }
 }
 