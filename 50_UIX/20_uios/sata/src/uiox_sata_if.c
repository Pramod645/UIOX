/**
 * @file  uiox_sata_if.c
 * @brief UIOX SATA interface driver — FIS framing, slots, IRQ.
 * @date  2026-06-12
 */

 #include "uiox_sata_if.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_sata_if_config(uiox_sata_if_t *sif, uiox_sata_hw_t *hw)
 {
     if (!sif || !hw) return -EINVAL;
     memset(sif, 0, sizeof(*sif));
     sif->hw          = hw;
     sif->primed      = true;
     sif->active_port = hw->port;
     sif->slot_bitmap = 0u;
     uiox_sata_buf_init();
     return 0;
 }
 
 int uiox_sata_if_start(uiox_sata_if_t *sif)
 {
     if (!sif || !sif->primed) return -EINVAL;
 
     /* Enable AHCI mode in GHC */
     uint32_t ghc = uiox_sata_hw_ghc_read(sif->hw, AHCI_GHC_GHC);
     ghc |= AHCI_GHC_GHC_AHCI_EN | AHCI_GHC_GHC_IE;
     uiox_sata_hw_ghc_write(sif->hw, AHCI_GHC_GHC, ghc);
 
     /* Start the active port */
     int rc = uiox_sata_hw_port_start(sif->hw, sif->active_port);
     if (rc < 0) return rc;
 
     /* Enable port interrupts */
     uiox_sata_hw_px_write(sif->hw, sif->active_port, AHCI_PX_IE,
                            AHCI_PX_IS_DHRS | AHCI_PX_IS_PSS  |
                            AHCI_PX_IS_DSS  | AHCI_PX_IS_SDBS |
                            AHCI_PX_IS_PCS  | AHCI_PX_IS_TFES |
                            AHCI_PX_IS_HBFS | AHCI_PX_IS_HBDS);
     return 0;
 }
 
 void uiox_sata_if_stop(uiox_sata_if_t *sif)
 {
     if (!sif) return;
     /* Disable port interrupts */
     uiox_sata_hw_px_write(sif->hw, sif->active_port, AHCI_PX_IE, 0u);
     /* Disable GHC interrupts */
     uint32_t ghc = uiox_sata_hw_ghc_read(sif->hw, AHCI_GHC_GHC);
     ghc &= ~AHCI_GHC_GHC_IE;
     uiox_sata_hw_ghc_write(sif->hw, AHCI_GHC_GHC, ghc);
     uiox_sata_hw_port_stop(sif->hw, sif->active_port);
 }
 
 int uiox_sata_if_alloc_slot(uiox_sata_if_t *sif)
 {
     if (!sif) return -EINVAL;
     for (uint8_t i = 0u; i < SATA_NCQ_DEPTH_MAX; i++) {
         if (!(sif->slot_bitmap & (1u << i))) {
             sif->slot_bitmap |= (1u << i);
             return (int)i;
         }
     }
     return -EBUSY;
 }
 
 void uiox_sata_if_free_slot(uiox_sata_if_t *sif, uint8_t slot)
 {
     if (!sif || slot >= SATA_NCQ_DEPTH_MAX) return;
     sif->slot_bitmap &= ~(1u << slot);
 }
 
 int uiox_sata_if_issue_cmd(uiox_sata_if_t *sif, uiox_sata_cmd_t *cmd)
 {
     if (!sif || !cmd) return -EINVAL;
 
     int slot = uiox_sata_if_alloc_slot(sif);
     if (slot < 0) return slot;
     cmd->slot = (uint8_t)slot;
 
     int rc = uiox_sata_hw_cmd_issue(sif->hw, sif->active_port,
                                      cmd->slot, &cmd->fis,
                                      cmd->write,
                                      (uintptr_t)cmd->prdt[0].dba,
                                      cmd->sector_count * UIOX_SATA_SECTOR_SIZE);
     if (rc < 0) {
         uiox_sata_if_free_slot(sif, cmd->slot);
         cmd->state = UIOX_SATA_CMD_ERROR;
         sif->stats.errors++;
         return rc;
     }
     sif->stats.cmds_issued++;
     return 0;
 }
 
 int uiox_sata_if_read(uiox_sata_if_t *sif, uint64_t lba,
                        uint8_t *buf, uint32_t sectors)
 {
     if (!sif || !buf || !sectors) return -EINVAL;
     int rc = uiox_sata_hw_read_sectors(sif->hw, lba, buf, sectors);
     if (rc == 0) {
         sif->stats.sectors_read += sectors;
         sif->stats.bytes_read   += (uint64_t)sectors * UIOX_SATA_SECTOR_SIZE;
     } else {
         sif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_sata_if_write(uiox_sata_if_t *sif, uint64_t lba,
                         const uint8_t *buf, uint32_t sectors)
 {
     if (!sif || !buf || !sectors) return -EINVAL;
     int rc = uiox_sata_hw_write_sectors(sif->hw, lba, buf, sectors);
     if (rc == 0) {
         sif->stats.sectors_written += sectors;
         sif->stats.bytes_written   += (uint64_t)sectors * UIOX_SATA_SECTOR_SIZE;
     } else {
         sif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_sata_if_ncq_read(uiox_sata_if_t *sif, uint64_t lba,
                             uint8_t *buf, uint32_t sectors, uint8_t tag)
 {
     if (!sif || !buf || !sectors) return -EINVAL;
     int rc = 0;
     if (OPS_VALID(sif->hw) && ((const uiox_sata_hw_ops_t *)sif->hw->priv)->ncq_read)
         rc = ((const uiox_sata_hw_ops_t *)sif->hw->priv)->ncq_read(
                  sif->hw, lba, buf, sectors, tag);
     else
         rc = uiox_sata_hw_read_sectors(sif->hw, lba, buf, sectors);
 
     if (rc == 0) {
         sif->stats.ncq_cmds_issued++;
         sif->stats.sectors_read += sectors;
         sif->stats.bytes_read   += (uint64_t)sectors * UIOX_SATA_SECTOR_SIZE;
     } else {
         sif->stats.errors++;
     }
     return rc;
 }
 
 int uiox_sata_if_ncq_write(uiox_sata_if_t *sif, uint64_t lba,
                              const uint8_t *buf, uint32_t sectors,
                              uint8_t tag)
 {
     if (!sif || !buf || !sectors) return -EINVAL;
     int rc = 0;
     if (OPS_VALID(sif->hw) && ((const uiox_sata_hw_ops_t *)sif->hw->priv)->ncq_write)
         rc = ((const uiox_sata_hw_ops_t *)sif->hw->priv)->ncq_write(
                  sif->hw, lba, buf, sectors, tag);
     else
         rc = uiox_sata_hw_write_sectors(sif->hw, lba, buf, sectors);
 
     if (rc == 0) {
         sif->stats.ncq_cmds_issued++;
         sif->stats.sectors_written += sectors;
         sif->stats.bytes_written   += (uint64_t)sectors * UIOX_SATA_SECTOR_SIZE;
     } else {
         sif->stats.errors++;
     }
     return rc;
 }
 
 uiox_sata_evt_t *uiox_sata_if_irq_handle(uiox_sata_if_t *sif,
                                            uint32_t now_ms)
 {
     if (!sif) return NULL;
     uint32_t irq = sif->hw->pending_irq;
     if (!irq) return NULL;
     sif->hw->pending_irq = 0u;
     sif->stats.irq_count++;
 
     /* Acknowledge port IS register */
     uint32_t px_is = uiox_sata_hw_px_read(sif->hw,
                                             sif->active_port, AHCI_PX_IS);
     uiox_sata_hw_px_write(sif->hw, sif->active_port,
                            AHCI_PX_IS, px_is);
     /* Acknowledge GHC IS */
     uint32_t ghc_is = uiox_sata_hw_ghc_read(sif->hw, AHCI_GHC_IS);
     uiox_sata_hw_ghc_write(sif->hw, AHCI_GHC_IS, ghc_is);
 
     uiox_sata_evt_t *e = uiox_sata_evt_alloc();
     if (!e) { sif->stats.errors++; return NULL; }
 
     e->timestamp_ms = now_ms;
     e->error_reg    = 0u;
 
     if (irq & UIOX_SATA_IRQ_ERROR) {
         e->type      = UIOX_SATA_EVT_ERROR;
         e->status    = -EIO;
         e->error_reg = uiox_sata_hw_px_read(sif->hw,
                                               sif->active_port,
                                               AHCI_PX_SERR);
         sif->stats.errors++;
     } else if (irq & UIOX_SATA_IRQ_HOTPLUG) {
         uint32_t ssts = uiox_sata_hw_px_read(sif->hw,
                                                sif->active_port,
                                                AHCI_PX_SSTS);
         uint8_t det = (uint8_t)(ssts & AHCI_PX_SSTS_DET_MASK);
         e->type  = (det == AHCI_PX_SSTS_DET_COMM)
                    ? UIOX_SATA_EVT_DEV_ATTACH
                    : UIOX_SATA_EVT_DEV_DETACH;
     } else if (irq & UIOX_SATA_IRQ_NCQ_DONE) {
         e->type   = UIOX_SATA_EVT_NCQ_DONE;
         e->status = 0;
         /* Clear completed NCQ tags via SACT */
         uint32_t sact = uiox_sata_hw_px_read(sif->hw,
                                                sif->active_port,
                                                AHCI_PX_SACT);
         (void)sact;
     } else {
         e->type   = UIOX_SATA_EVT_CMD_DONE;
         e->status = (px_is & AHCI_PX_IS_TFES) ? -EIO : 0;
     }
     return e;
 }
 
 void uiox_sata_if_stats_get(const uiox_sata_if_t *sif,
                               uiox_sata_if_stats_t *out)
 { if (!sif || !out) return; memcpy(out, &sif->stats, sizeof(*out)); }
 
 /* Internal helper — check ops pointer */
 #define OPS_VALID(hw)  ((hw) && (hw)->priv)
 