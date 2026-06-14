/**
 * @file  uiox_sd_proto.c
 * @brief UIOX SD protocol — card init state machine, CSD/CID, R/W.
 * @date  2026-06-11
 */

 #include "uiox_sd_proto.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_sd_proto_init(uiox_sd_proto_t *proto, uiox_sd_if_t *sif)
 {
     if (!proto || !sif) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->sif        = sif;
     proto->init_state = UIOX_SD_INIT_IDLE;
     return 0;
 }
 
 uint64_t uiox_sd_proto_capacity(const uiox_sd_card_t *card)
 {
     if (!card) return 0u;
     if (card->card_type == UIOX_SD_CARD_SDHC ||
         card->card_type == UIOX_SD_CARD_SDXC) {
         /*
          * CSD v2: C_SIZE [69:48] in bytes 7–9
          * capacity = (C_SIZE + 1) × 512 KB
          */
         uint32_t c_size = (uint32_t)(card->csd[7] & 0x3Fu) << 16u
                         | (uint32_t) card->csd[8]           << 8u
                         | (uint32_t) card->csd[9];
         return (uint64_t)(c_size + 1u) * 512u * 1024u;
     } else {
         /*
          * CSD v1: C_SIZE [73:62], C_SIZE_MULT [49:47], READ_BL_LEN [83:80]
          * capacity = (C_SIZE+1) × 2^(C_SIZE_MULT+2) × 2^READ_BL_LEN
          */
         uint32_t read_bl_len = card->csd[5] & 0x0Fu;
         uint32_t c_size      = ((uint32_t)(card->csd[6] & 0x03u) << 10u)
                              |  ((uint32_t) card->csd[7]          << 2u)
                              |  ((uint32_t)(card->csd[8] >> 6u)   & 0x03u);
         uint32_t c_size_mult = ((uint32_t)(card->csd[9]  & 0x03u) << 1u)
                              |  ((uint32_t)(card->csd[10] >> 7u)  & 0x01u);
         uint64_t block_len   = 1u << read_bl_len;
         uint64_t mult        = 1u << (c_size_mult + 2u);
         return (uint64_t)(c_size + 1u) * mult * block_len;
     }
 }
 
 int uiox_sd_proto_card_init(uiox_sd_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     uiox_sd_if_t   *sif  = proto->sif;
     uiox_sd_card_t *card = &sif->hw->card;
     uint32_t resp[4];
     int rc;
 
     proto->init_state = UIOX_SD_INIT_VOLTAGE_CHK;
     printf("  [proto] Card init start\n");
 
     /* CMD0: GO_IDLE_STATE */
     rc = uiox_sd_if_send_cmd(sif, SD_CMD0_GO_IDLE, 0u,
                               SD_RESP_NONE, NULL);
     if (rc < 0) goto err;
 
     /* CMD8: SEND_IF_COND — check voltage range */
     proto->init_state = UIOX_SD_INIT_VOLTAGE_CHK;
     rc = uiox_sd_if_send_cmd(sif, SD_CMD8_SEND_IF_COND,
                               SD_CMD8_VHS_27_36V | SD_CMD8_CHECK_PATTERN,
                               SD_RESP_R7, resp);
     bool v2_card = (rc == 0 &&
                     (resp[0] & 0xFFu) == SD_CMD8_CHECK_PATTERN);
     printf("  [proto] CMD8: v2_card=%d\n", (int)v2_card);
 
     /* ACMD41: SD_SEND_OP_COND — repeat until card ready */
     proto->init_state = UIOX_SD_INIT_ACMD41;
     for (uint32_t i = 0u; i < 1000u; i++) {
         /* CMD55: APP_CMD prefix */
         rc = uiox_sd_if_send_cmd(sif, SD_CMD55_APP_CMD, 0u,
                                   SD_RESP_R1, resp);
         if (rc < 0) goto err;
         uint32_t acmd41_arg = v2_card ? (SD_ACMD41_HCS | 0x00FF8000u)
                                        : 0x00FF8000u;
         rc = uiox_sd_if_send_cmd(sif, SD_ACMD41_SD_SEND_OP_COND,
                                   acmd41_arg, SD_RESP_R3, resp);
         if (rc < 0) goto err;
         card->ocr = resp[0];
         if (card->ocr & SD_ACMD41_OCR_BUSY) {
             /* Card powered up */
             card->card_type = (card->ocr & SD_ACMD41_CCS)
                               ? UIOX_SD_CARD_SDHC
                               : UIOX_SD_CARD_SDSC;
             break;
         }
     }
     printf("  [proto] OCR=0x%08X  type=%s\n",
            card->ocr, uiox_sd_card_type_name(card->card_type));
 
     /* CMD2: ALL_SEND_CID */
     proto->init_state = UIOX_SD_INIT_SEND_CID;
     rc = uiox_sd_if_send_cmd(sif, SD_CMD2_ALL_SEND_CID, 0u,
                               SD_RESP_R2, resp);
     if (rc < 0) goto err;
     memcpy(card->cid, resp, UIOX_SD_CID_LEN);
 
     /* CMD3: SEND_RELATIVE_ADDR — get RCA */
     proto->init_state = UIOX_SD_INIT_SEND_RCA;
     rc = uiox_sd_if_send_cmd(sif, SD_CMD3_SEND_REL_ADDR, 0u,
                               SD_RESP_R6, resp);
     if (rc < 0) goto err;
     card->rca = (uint16_t)(resp[0] >> 16u);
     printf("  [proto] RCA=0x%04X\n", card->rca);
 
     /* CMD9: SEND_CSD */
     proto->init_state = UIOX_SD_INIT_SEND_CSD;
     rc = uiox_sd_if_send_cmd(sif, SD_CMD9_SEND_CSD,
                               (uint32_t)card->rca << 16u,
                               SD_RESP_R2, resp);
     if (rc < 0) goto err;
     memcpy(card->csd, resp, UIOX_SD_CSD_LEN);
 
     /* Decode capacity */
     card->capacity_bytes  = uiox_sd_proto_capacity(card);
     card->capacity_blocks = card->capacity_bytes / UIOX_SD_BLOCK_SIZE;
     printf("  [proto] Capacity: %llu MB (%llu blocks)\n",
            (unsigned long long)(card->capacity_bytes >> 20u),
            (unsigned long long) card->capacity_blocks);
 
     /* CMD7: SELECT_CARD */
     proto->init_state = UIOX_SD_INIT_SELECT_CARD;
     rc = uiox_sd_if_send_cmd(sif, SD_CMD7_SELECT_CARD,
                               (uint32_t)card->rca << 16u,
                               SD_RESP_R1B, resp);
     if (rc < 0) goto err;
 
     /* CMD16: SET_BLOCKLEN (SDSC only) */
     proto->init_state = UIOX_SD_INIT_SET_BLKLEN;
     if (card->card_type == UIOX_SD_CARD_SDSC) {
         rc = uiox_sd_if_send_cmd(sif, SD_CMD16_SET_BLOCKLEN,
                                   UIOX_SD_BLOCK_SIZE,
                                   SD_RESP_R1, resp);
         if (rc < 0) goto err;
     }
 
     /* ACMD6: SET_BUS_WIDTH (4-bit if host supports it) */
     proto->init_state = UIOX_SD_INIT_SET_BUS_WIDTH;
     if (sif->hw->caps & UIOX_SD_CAP_SDIO_4BIT) {
         rc = uiox_sd_if_send_cmd(sif, SD_CMD55_APP_CMD,
                                   (uint32_t)card->rca << 16u,
                                   SD_RESP_R1, resp);
         if (rc == 0) {
             rc = uiox_sd_if_send_cmd(sif, SD_ACMD6_SET_BUS_WIDTH,
                                       0x02u, SD_RESP_R1, resp);
             if (rc == 0) {
                 uiox_sd_if_set_bus_width(sif, 4u);
                 printf("  [proto] Bus width: 4-bit\n");
             }
         }
     }
 
     /* Switch to transfer clock */
     uiox_sd_if_set_clock(sif, sif->hw->clk_xfer_hz);
     printf("  [proto] Clock: %u Hz\n", sif->hw->clk_xfer_hz);
 
     proto->init_state   = UIOX_SD_INIT_DONE;
     proto->initialized  = true;
     sif->hw->card_present = true;
     printf("  [proto] Card init DONE\n");
     return 0;
 
 err:
     proto->init_state = UIOX_SD_INIT_ERROR;
     printf("  [proto] Card init ERROR at state %d  rc=%d\n",
            (int)proto->init_state, rc);
     return rc;
 }
 
 int uiox_sd_proto_read(uiox_sd_proto_t *proto, uint32_t lba,
                         uint8_t *buf, uint32_t count)
 {
     if (!proto || !proto->initialized || !buf || !count) return -EINVAL;
     /* For SDSC, address is byte address; for SDHC/X it is block address */
     uint32_t addr = (proto->sif->hw->card.card_type == UIOX_SD_CARD_SDSC)
                     ? lba * UIOX_SD_BLOCK_SIZE : lba;
     if (count == 1u) {
         uint32_t resp;
         int rc = uiox_sd_if_send_cmd(proto->sif, SD_CMD17_READ_SINGLE_BLOCK,
                                       addr, SD_RESP_R1, &resp);
         if (rc < 0) return rc;
     }
     return uiox_sd_if_read(proto->sif, lba, buf, count);
 }
 
 int uiox_sd_proto_write(uiox_sd_proto_t *proto, uint32_t lba,
                          const uint8_t *buf, uint32_t count)
 {
     if (!proto || !proto->initialized || !buf || !count) return -EINVAL;
     if (proto->sif->hw->write_protect) return -EROFS;
     uint32_t addr = (proto->sif->hw->card.card_type == UIOX_SD_CARD_SDSC)
                     ? lba * UIOX_SD_BLOCK_SIZE : lba;
     if (count == 1u) {
         uint32_t resp;
         int rc = uiox_sd_if_send_cmd(proto->sif, SD_CMD24_WRITE_BLOCK,
                                       addr, SD_RESP_R1, &resp);
         if (rc < 0) return rc;
     }
     return uiox_sd_if_write(proto->sif, lba, buf, count);
 }
 
 int uiox_sd_proto_get_status(uiox_sd_proto_t *proto, uint32_t *status)
 {
     if (!proto || !status) return -EINVAL;
     uint32_t resp;
     int rc = uiox_sd_if_send_cmd(proto->sif, SD_CMD13_SEND_STATUS,
                                   (uint32_t)proto->sif->hw->card.rca << 16u,
                                   SD_RESP_R1, &resp);
     if (rc == 0) *status = resp;
     return rc;
 }
 
 int uiox_sd_proto_erase(uiox_sd_proto_t *proto,
                          uint32_t lba_start, uint32_t lba_end)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     if (proto->sif->hw->write_protect) return -EROFS;
     /* CMD32: ERASE_WR_BLK_START, CMD33: ERASE_WR_BLK_END, CMD38: ERASE */
     uint32_t resp;
     int rc = uiox_sd_if_send_cmd(proto->sif, 32u, lba_start,
                                   SD_RESP_R1, &resp);
     if (rc < 0) return rc;
     rc = uiox_sd_if_send_cmd(proto->sif, 33u, lba_end,
                               SD_RESP_R1, &resp);
     if (rc < 0) return rc;
     rc = uiox_sd_if_send_cmd(proto->sif, 38u, 0u,
                               SD_RESP_R1B, &resp);
     printf("  [proto] Erase LBA %u–%u  rc=%d\n", lba_start, lba_end, rc);
     return rc;
 }
 
 int uiox_sd_proto_switch_hs(uiox_sd_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     uint32_t resp;
     /* CMD6: SWITCH_FUNC — switch to high-speed */
     int rc = uiox_sd_if_send_cmd(proto->sif, SD_CMD6_SWITCH_FUNC,
                                   0x80FFFFF1u, SD_RESP_R1, &resp);
     if (rc == 0) {
         uiox_sd_if_set_clock(proto->sif, 50000000u);
         printf("  [proto] High-speed mode enabled (50 MHz)\n");
     }
     return rc;
 }
 