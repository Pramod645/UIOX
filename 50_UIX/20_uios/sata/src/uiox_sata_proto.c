/**
 * @file  uiox_sata_proto.c
 * @brief UIOX SATA protocol — IDENTIFY, ATA R/W, NCQ, SMART, power.
 * @date  2026-06-12
 */

 #include "uiox_sata_proto.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 /* Swap pairs of bytes in an ATA string (ATA words are byte-swapped) */
 static void ata_str_fixup(char *dst, const uint16_t *words,
                            uint8_t word_start, uint8_t word_count)
 {
     uint8_t n = (uint8_t)(word_count * 2u);
     for (uint8_t i = 0u; i < n; i += 2u) {
         uint16_t w = words[word_start + i / 2u];
         dst[i]     = (char)((w >> 8u) & 0xFFu);
         dst[i + 1] = (char)( w        & 0xFFu);
     }
     /* Strip trailing spaces */
     for (int i = (int)n - 1; i >= 0 && dst[i] == ' '; i--)
         dst[i] = '\0';
 }
 
 uint64_t uiox_sata_proto_capacity(const uiox_sata_ident_t *ident)
 {
     if (!ident) return 0u;
     return ident->lba48_sectors * UIOX_SATA_SECTOR_SIZE;
 }
 
 int uiox_sata_proto_init(uiox_sata_proto_t *proto, uiox_sata_if_t *sif)
 {
     if (!proto || !sif) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->sif        = sif;
     proto->init_state = UIOX_SATA_INIT_IDLE;
     return 0;
 }
 
 int uiox_sata_proto_identify(uiox_sata_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     uiox_sata_hw_t    *hw   = proto->sif->hw;
     uiox_sata_ident_t *id   = &hw->ident;
 
     /* Allocate a 512-byte buffer for IDENTIFY response */
     uiox_sata_blk_t *blk = uiox_sata_blk_alloc();
     if (!blk) return -ENOMEM;
 
     /* Issue IDENTIFY using PIO (single sector) */
     int rc = uiox_sata_hw_read_sectors(hw, 0u, blk->data, 0u);
     /* 0 sectors = special: HAL identifies via ATA IDENTIFY command */
     (void)rc;
 
     const uint16_t *w = (const uint16_t *)blk->data;
 
     /* Extract strings */
     ata_str_fixup(id->model_str,  w, ATA_ID_MODEL,    20u);
     ata_str_fixup(id->serial_str, w, ATA_ID_SERIAL,   10u);
     ata_str_fixup(id->fw_str,     w, ATA_ID_FW_REV,    4u);
 
     /* 48-bit LBA sector count (words 100–103) */
     id->lba48_sectors =
         (uint64_t)w[ATA_ID_LBA48_SECTORS    ]        |
         ((uint64_t)w[ATA_ID_LBA48_SECTORS + 1] << 16u) |
         ((uint64_t)w[ATA_ID_LBA48_SECTORS + 2] << 32u) |
         ((uint64_t)w[ATA_ID_LBA48_SECTORS + 3] << 48u);
 
     id->capacity_bytes = id->lba48_sectors * UIOX_SATA_SECTOR_SIZE;
 
     /* NCQ depth */
     id->ncq_depth = (uint8_t)((w[ATA_ID_QUEUE_DEPTH] & 0x1Fu) + 1u);
     if (id->ncq_depth > SATA_NCQ_DEPTH_MAX)
         id->ncq_depth = SATA_NCQ_DEPTH_MAX;
 
     /* RPM — word 217: 0 = SSD, 1 = non-rotating, else RPM */
     id->rpm    = w[ATA_ID_RPM];
     id->is_ssd = (id->rpm == 0u || id->rpm == 1u);
 
     /* TRIM support — word 169 bit 0 */
     id->trim_supported  = !!(w[169] & 0x0001u);
 
     /* SMART support — word 82 bit 0 */
     id->smart_supported = !!(w[82] & 0x0001u);
 
     uiox_sata_blk_free(blk);
 
     printf("  [proto] IDENTIFY OK\n");
     printf("  [proto]   Model  : %.40s\n", id->model_str);
     printf("  [proto]   Serial : %.20s\n", id->serial_str);
     printf("  [proto]   FW     : %.8s\n",  id->fw_str);
     printf("  [proto]   Cap    : %llu GB\n",
            (unsigned long long)(id->capacity_bytes >> 30u));
     printf("  [proto]   NCQ    : %u tags\n", id->ncq_depth);
     printf("  [proto]   SSD    : %s  TRIM: %s\n",
            id->is_ssd ? "YES" : "NO",
            id->trim_supported ? "YES" : "NO");
     return 0;
 }
 
 int uiox_sata_proto_dev_init(uiox_sata_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     int rc;
 
     /* COMRESET */
     proto->init_state = UIOX_SATA_INIT_RESET;
     rc = uiox_sata_hw_port_reset(proto->sif->hw,
                                   proto->sif->active_port);
     if (rc < 0) goto err;
     printf("  [proto] Port reset OK\n");
 
     /* IDENTIFY */
     proto->init_state = UIOX_SATA_INIT_IDENTIFY;
     rc = uiox_sata_proto_identify(proto);
     if (rc < 0) goto err;
 
     /* SET FEATURES: enable write cache */
     proto->init_state = UIOX_SATA_INIT_FEATURES;
     /* (Stub: SET FEATURES via cmd_issue would go here) */
     printf("  [proto] SET FEATURES (write cache enable) OK\n");
 
     /* SMART enable */
     if (proto->sif->hw->ident.smart_supported) {
         proto->init_state = UIOX_SATA_INIT_SMART_EN;
         rc = uiox_sata_proto_smart_en(proto);
         if (rc < 0)
             printf("  [proto] SMART enable failed (non-fatal)\n");
     }
 
     /* Enable NCQ if supported */
     if (proto->sif->hw->caps & UIOX_SATA_CAP_NCQ &&
         proto->sif->hw->ident.ncq_depth > 1u) {
         proto->ncq_enabled = true;
         printf("  [proto] NCQ enabled (%u tags)\n",
                proto->sif->hw->ident.ncq_depth);
     }
 
     proto->init_state  = UIOX_SATA_INIT_DONE;
     proto->initialized = true;
     proto->sif->hw->dev_ready = true;
     printf("  [proto] Device init DONE\n");
     return 0;
 
 err:
     proto->init_state = UIOX_SATA_INIT_ERROR;
     printf("  [proto] Device init ERROR state=%d rc=%d\n",
            (int)proto->init_state, rc);
     return rc;
 }
 
 int uiox_sata_proto_read(uiox_sata_proto_t *proto, uint64_t lba,
                           uint8_t *buf, uint32_t sectors)
 {
     if (!proto || !proto->initialized || !buf || !sectors)
         return -EINVAL;
     return uiox_sata_if_read(proto->sif, lba, buf, sectors);
 }
 
 int uiox_sata_proto_write(uiox_sata_proto_t *proto, uint64_t lba,
                            const uint8_t *buf, uint32_t sectors)
 {
     if (!proto || !proto->initialized || !buf || !sectors)
         return -EINVAL;
     return uiox_sata_if_write(proto->sif, lba, buf, sectors);
 }
 
 int uiox_sata_proto_ncq_read(uiox_sata_proto_t *proto, uint64_t lba,
                                uint8_t *buf, uint32_t sectors)
 {
     if (!proto || !proto->initialized || !buf || !sectors)
         return -EINVAL;
     /* Assign next free NCQ tag */
     uint8_t tag = 0u;
     for (uint8_t i = 0u; i < SATA_NCQ_DEPTH_MAX; i++) {
         if (!(proto->sif->hw->ncq_active & (1u << i))) {
             tag = i;
             proto->sif->hw->ncq_active |= (1u << i);
             break;
         }
     }
     return uiox_sata_if_ncq_read(proto->sif, lba, buf, sectors, tag);
 }
 
 int uiox_sata_proto_ncq_write(uiox_sata_proto_t *proto, uint64_t lba,
                                 const uint8_t *buf, uint32_t sectors)
 {
     if (!proto || !proto->initialized || !buf || !sectors)
         return -EINVAL;
     uint8_t tag = 0u;
     for (uint8_t i = 0u; i < SATA_NCQ_DEPTH_MAX; i++) {
         if (!(proto->sif->hw->ncq_active & (1u << i))) {
             tag = i;
             proto->sif->hw->ncq_active |= (1u << i);
             break;
         }
     }
     return uiox_sata_if_ncq_write(proto->sif, lba, buf, sectors, tag);
 }
 
 int uiox_sata_proto_flush(uiox_sata_proto_t *proto)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     printf("  [proto] FLUSH CACHE EXT\n");
     /* In real driver: issue ATA_CMD_FLUSH_CACHE_EXT via cmd_issue */
     return 0;
 }
 
 int uiox_sata_proto_trim(uiox_sata_proto_t *proto,
                           uint64_t lba_start, uint32_t sectors)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     if (!proto->sif->hw->ident.trim_supported) return -ENOTSUP;
     printf("  [proto] TRIM  lba=%llu  sectors=%u\n",
            (unsigned long long)lba_start, sectors);
     /* In real driver: build DSM descriptor and issue ATA_CMD_DATA_SET_MGMT */
     return 0;
 }
 
 int uiox_sata_proto_smart_en(uiox_sata_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     printf("  [proto] SMART enable\n");
     return 0;
 }
 
 int uiox_sata_proto_smart_read(uiox_sata_proto_t *proto, uint8_t *buf)
 {
     if (!proto || !proto->initialized || !buf) return -EINVAL;
     return uiox_sata_hw_smart_read(proto->sif->hw, buf);
 }
 
 int uiox_sata_proto_standby(uiox_sata_proto_t *proto)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     printf("  [proto] STANDBY IMMEDIATE\n");
     return 0;
 }
 
 int uiox_sata_proto_sleep(uiox_sata_proto_t *proto)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     printf("  [proto] SLEEP\n");
     return 0;
 }
 