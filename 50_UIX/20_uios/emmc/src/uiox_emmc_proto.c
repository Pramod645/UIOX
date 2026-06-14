/**
 * @file  uiox_emmc_proto.c
 * @brief UIOX eMMC protocol — init, EXT_CSD, HS400, R/W, health.
 * @date  2026-06-12
 */

 #include "uiox_emmc_proto.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 /* -------------------------------------------------------------------------
  * EXT_CSD parsing
  * ---------------------------------------------------------------------- */
 
 void uiox_emmc_proto_parse_ext_csd(uiox_emmc_ident_t *id,
                                      const uint8_t *ext_csd)
 {
     if (!id || !ext_csd) return;
     memcpy(id->ext_csd, ext_csd, UIOX_EMMC_EXT_CSD_LEN);
 
     /* Sector count (LE 32-bit at byte 212) */
     id->capacity_sectors =
         (uint64_t)ext_csd[EXT_CSD_SEC_COUNT]     |
         ((uint64_t)ext_csd[EXT_CSD_SEC_COUNT + 1] << 8u)  |
         ((uint64_t)ext_csd[EXT_CSD_SEC_COUNT + 2] << 16u) |
         ((uint64_t)ext_csd[EXT_CSD_SEC_COUNT + 3] << 24u);
     id->capacity_bytes = id->capacity_sectors * UIOX_EMMC_BLOCK_SIZE;
 
     /* Cache size (LE 32-bit KB at byte 249) */
     id->cache_size_kb =
         (uint32_t)ext_csd[EXT_CSD_CACHE_SIZE]     |
         ((uint32_t)ext_csd[EXT_CSD_CACHE_SIZE + 1] << 8u)  |
         ((uint32_t)ext_csd[EXT_CSD_CACHE_SIZE + 2] << 16u) |
         ((uint32_t)ext_csd[EXT_CSD_CACHE_SIZE + 3] << 24u);
 
     id->device_type = ext_csd[EXT_CSD_DEVICE_TYPE];
     id->pre_eol_info = ext_csd[EXT_CSD_PRE_EOL_INFO];
     id->life_est_a   = ext_csd[EXT_CSD_DEVICE_LIFE_EST_A];
     id->life_est_b   = ext_csd[EXT_CSD_DEVICE_LIFE_EST_B];
 
     /* HC erase group size (512 KB units) */
     id->erase_group_sectors =
         (uint32_t)ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 1024u;
 
     /* Feature flags */
     id->cache_supported      = (id->cache_size_kb > 0u);
     id->trim_supported       = !!(ext_csd[EXT_CSD_CLASS_6_CTRL] & 0x01u);
     id->hpi_supported        = !!(ext_csd[EXT_CSD_CMDQ_MODE_EN] & 0x01u);
     id->bkops_supported      = !!(ext_csd[EXT_CSD_BKOPS_STATUS] & 0x01u);
     id->pon_supported        = !!(ext_csd[EXT_CSD_POWER_OFF_NOTIF] & 0x01u);
     id->packed_cmd_supported = !!(ext_csd[EXT_CSD_PACKED_CMD_STATUS] & 0x01u);
 
     /* Partition sizes */
     id->parts[UIOX_EMMC_PART_USER].id           = UIOX_EMMC_PART_USER;
     id->parts[UIOX_EMMC_PART_USER].size_bytes    = id->capacity_bytes;
     id->parts[UIOX_EMMC_PART_USER].enabled       = true;
 
     uint64_t boot_bytes =
         (uint64_t)ext_csd[EXT_CSD_BOOT_SIZE_MULT] * 128u * 1024u;
     id->parts[UIOX_EMMC_PART_BOOT1].id           = UIOX_EMMC_PART_BOOT1;
     id->parts[UIOX_EMMC_PART_BOOT1].size_bytes    = boot_bytes;
     id->parts[UIOX_EMMC_PART_BOOT1].enabled       = (boot_bytes > 0u);
 
     id->parts[UIOX_EMMC_PART_BOOT2].id           = UIOX_EMMC_PART_BOOT2;
     id->parts[UIOX_EMMC_PART_BOOT2].size_bytes    = boot_bytes;
     id->parts[UIOX_EMMC_PART_BOOT2].enabled       = (boot_bytes > 0u);
 
     uint64_t rpmb_bytes =
         (uint64_t)ext_csd[EXT_CSD_RPMB_SIZE_MULT] * 128u * 1024u;
     id->parts[UIOX_EMMC_PART_RPMB].id            = UIOX_EMMC_PART_RPMB;
     id->parts[UIOX_EMMC_PART_RPMB].size_bytes     = rpmb_bytes;
     id->parts[UIOX_EMMC_PART_RPMB].enabled        = (rpmb_bytes > 0u);
 }
 
 uint64_t uiox_emmc_proto_capacity(const uiox_emmc_ident_t *id)
 { return id ? id->capacity_bytes : 0u; }
 
 int uiox_emmc_proto_init(uiox_emmc_proto_t *proto, uiox_emmc_if_t *eif)
 {
     if (!proto || !eif) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->eif        = eif;
     proto->init_state = UIOX_EMMC_INIT_IDLE;
     return 0;
 }
 
 /* -------------------------------------------------------------------------
  * Full device initialisation sequence
  * ---------------------------------------------------------------------- */
 
 int uiox_emmc_proto_dev_init(uiox_emmc_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     uiox_emmc_if_t    *eif = proto->eif;
     uiox_emmc_hw_t    *hw  = eif->hw;
     uiox_emmc_ident_t *id  = &hw->ident;
     uint32_t resp[4];
     int rc;
 
     /* --- Hardware reset via RST_N GPIO (if supported) --- */
     proto->init_state = UIOX_EMMC_INIT_RESET;
     if (hw->caps & UIOX_EMMC_CAP_RST_N) {
         const uiox_emmc_hw_ops_t *ops =
             (const uiox_emmc_hw_ops_t *)hw->priv;
         if (ops->gpio_write) {
             ops->gpio_write(hw, UIOX_EMMC_GPIO_RST_N, false);
            /* tRSTW >= 1 us — simulated with nop */
            ops->gpio_write(hw, UIOX_EMMC_GPIO_RST_N, true);
        }
    }

    /* CMD0: GO_IDLE_STATE */
    rc = uiox_emmc_if_send_cmd(eif, MMC_CMD0_GO_IDLE, 0u,
                                EMMC_RESP_NONE, NULL);
    if (rc < 0) goto err;
    printf("  [proto] CMD0 GO_IDLE OK\n");

    /* CMD1: SEND_OP_COND — repeat until card ready */
    proto->init_state = UIOX_EMMC_INIT_CMD1;
    for (uint32_t i = 0u; i < 1000u; i++) {
        rc = uiox_emmc_if_send_cmd(eif, MMC_CMD1_SEND_OP_COND,
                                    MMC_CMD1_HCS | 0x00FF8080u,
                                    EMMC_RESP_R3, resp);
        if (rc < 0) goto err;
        if (resp[0] & MMC_CMD1_OCR_BUSY) {
            id->parts[0].enabled = true;
            printf("  [proto] CMD1 OCR=0x%08X  CCS=%d\n",
                   resp[0],
                   !!(resp[0] & MMC_CMD1_CCS));
            break;
        }
    }

    /* CMD2: ALL_SEND_CID */
    proto->init_state = UIOX_EMMC_INIT_CID;
    rc = uiox_emmc_if_send_cmd(eif, MMC_CMD2_ALL_SEND_CID, 0u,
                                EMMC_RESP_R2, resp);
    if (rc < 0) goto err;
    memcpy(id->cid, resp, UIOX_EMMC_CID_LEN);
    /* Product name: CID bytes 3–8 */
    for (int i = 0; i < 6; i++)
        id->product_name[i] = (char)id->cid[3 + i];
    id->product_name[6] = '\0';
    printf("  [proto] CID product=%s\n", id->product_name);

    /* CMD3: SET_RELATIVE_ADDR (RCA = 1 for eMMC) */
    proto->init_state = UIOX_EMMC_INIT_RCA;
    hw->rca = 1u;
    rc = uiox_emmc_if_send_cmd(eif, MMC_CMD3_SET_REL_ADDR,
                                (uint32_t)hw->rca << 16u,
                                EMMC_RESP_R1, resp);
    if (rc < 0) goto err;
    printf("  [proto] RCA=0x%04X\n", hw->rca);

    /* CMD9: SEND_CSD */
    proto->init_state = UIOX_EMMC_INIT_CSD;
    rc = uiox_emmc_if_send_cmd(eif, MMC_CMD9_SEND_CSD,
                                (uint32_t)hw->rca << 16u,
                                EMMC_RESP_R2, resp);
    if (rc < 0) goto err;
    memcpy(id->csd, resp, UIOX_EMMC_CSD_LEN);

    /* CMD7: SELECT_CARD */
    proto->init_state = UIOX_EMMC_INIT_SELECT;
    rc = uiox_emmc_if_send_cmd(eif, MMC_CMD7_SELECT_CARD,
                                (uint32_t)hw->rca << 16u,
                                EMMC_RESP_R1B, resp);
    if (rc < 0) goto err;
    printf("  [proto] Card selected\n");

    /* CMD8: READ_EXT_CSD */
    proto->init_state = UIOX_EMMC_INIT_EXT_CSD;
    {
        uint8_t ext_csd[UIOX_EMMC_EXT_CSD_LEN];
        rc = uiox_emmc_if_read_ext_csd(eif, ext_csd);
        if (rc < 0) goto err;
        uiox_emmc_proto_parse_ext_csd(id, ext_csd);
        printf("  [proto] EXT_CSD: cap=%llu GB  cache=%u KB"
               "  dtype=0x%02X\n",
               (unsigned long long)(id->capacity_bytes >> 30u),
               id->cache_size_kb,
               id->device_type);
    }

    /* --- Switch bus width to 8-bit --- */
    proto->init_state = UIOX_EMMC_INIT_BUS_WIDTH;
    if (hw->caps & UIOX_EMMC_CAP_8BIT) {
        /* EXT_CSD[183] = BUS_WIDTH = 2 (8-bit SDR) */
        rc = uiox_emmc_if_switch(eif,
                                  MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_BUS_WIDTH,
                                  EXT_CSD_BUS_WIDTH_8, 0u);
        if (rc < 0) goto err;
        rc = uiox_emmc_if_set_bus_width(eif, 8u);
        if (rc < 0) goto err;
        proto->negotiated_width = 8u;
        printf("  [proto] Bus width: 8-bit\n");
    } else if (hw->caps & UIOX_EMMC_CAP_4BIT) {
        rc = uiox_emmc_if_switch(eif,
                                  MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_BUS_WIDTH,
                                  EXT_CSD_BUS_WIDTH_4, 0u);
        if (rc < 0) goto err;
        rc = uiox_emmc_if_set_bus_width(eif, 4u);
        if (rc < 0) goto err;
        proto->negotiated_width = 4u;
        printf("  [proto] Bus width: 4-bit\n");
    } else {
        proto->negotiated_width = 1u;
    }

    /* --- Switch speed mode --- */
    proto->init_state = UIOX_EMMC_INIT_SPEED;
    if ((hw->caps & UIOX_EMMC_CAP_HS400) &&
        (id->device_type & EXT_CSD_DEVICE_TYPE_HS400)) {
        /* Step 1: HS200 first */
        rc = uiox_emmc_if_switch(eif, MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_HS_TIMING,
                                  EXT_CSD_HS_TIMING_HS200, 0u);
        if (rc == 0) {
            rc = uiox_emmc_if_set_speed(eif, UIOX_EMMC_SPEED_HS200);
            if (rc == 0) {
                uiox_emmc_hw_set_clock(hw, 200000000u);
                /* Tuning */
                uiox_emmc_hw_tuning(hw);
                /* Step 2: HS400 */
                rc = uiox_emmc_if_switch(eif, MMC_SWITCH_WRITE_BYTE,
                                          EXT_CSD_HS_TIMING,
                                          EXT_CSD_HS_TIMING_HS400, 0u);
                if (rc == 0) {
                    rc = uiox_emmc_if_set_speed(eif,
                                                 UIOX_EMMC_SPEED_HS400);
                    if (rc == 0) {
                        proto->negotiated_speed = UIOX_EMMC_SPEED_HS400;
                        printf("  [proto] Speed: HS400 (200 MHz DDR)\n");
                    }
                }
            }
        }
    } else if ((hw->caps & UIOX_EMMC_CAP_HS200) &&
               (id->device_type & EXT_CSD_DEVICE_TYPE_HS200)) {
        rc = uiox_emmc_if_switch(eif, MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_HS_TIMING,
                                  EXT_CSD_HS_TIMING_HS200, 0u);
        if (rc == 0) {
            rc = uiox_emmc_if_set_speed(eif, UIOX_EMMC_SPEED_HS200);
            if (rc == 0) {
                uiox_emmc_hw_set_clock(hw, 200000000u);
                uiox_emmc_hw_tuning(hw);
                proto->negotiated_speed = UIOX_EMMC_SPEED_HS200;
                printf("  [proto] Speed: HS200 (200 MHz SDR)\n");
            }
        }
    } else if (id->device_type & EXT_CSD_DEVICE_TYPE_HS_52) {
        rc = uiox_emmc_if_switch(eif, MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_HS_TIMING,
                                  EXT_CSD_HS_TIMING_HS, 0u);
        if (rc == 0) {
            rc = uiox_emmc_if_set_speed(eif, UIOX_EMMC_SPEED_HS52);
            if (rc == 0) {
                uiox_emmc_hw_set_clock(hw, 52000000u);
                proto->negotiated_speed = UIOX_EMMC_SPEED_HS52;
                printf("  [proto] Speed: HS52 (52 MHz)\n");
            }
        }
    }
    /* Non-fatal if speed switch fails — stay at DS */
    if (proto->negotiated_speed == UIOX_EMMC_SPEED_IDENT)
        proto->negotiated_speed = UIOX_EMMC_SPEED_DS;

    /* --- Enable write cache --- */
    proto->init_state = UIOX_EMMC_INIT_CACHE_EN;
    if (id->cache_supported) {
        rc = uiox_emmc_proto_cache_ctrl(proto, true);
        if (rc == 0) {
            proto->cache_enabled = true;
            printf("  [proto] Write cache enabled (%u KB)\n",
                   id->cache_size_kb);
        }
        rc = 0;  /* non-fatal */
    }

    proto->init_state  = UIOX_EMMC_INIT_DONE;
    proto->initialized = true;
    hw->dev_ready      = true;
    printf("  [proto] eMMC init DONE\n");
    return 0;

err:
    proto->init_state = UIOX_EMMC_INIT_ERROR;
    printf("  [proto] eMMC init ERROR  state=%d  rc=%d\n",
           (int)proto->init_state, rc);
    return rc;
}

/* -------------------------------------------------------------------------
 * Block I/O
 * ---------------------------------------------------------------------- */

int uiox_emmc_proto_read(uiox_emmc_proto_t *proto,
                          uiox_emmc_part_t part, uint32_t lba,
                          uint8_t *buf, uint32_t sectors)
{
    if (!proto || !proto->initialized || !buf || !sectors)
        return -EINVAL;
    /* Switch partition if needed */
    if (proto->eif->hw->active_part != part) {
        int rc = uiox_emmc_if_select_part(proto->eif, part);
        if (rc < 0) return rc;
    }
    return uiox_emmc_if_read(proto->eif, lba, buf, sectors);
}

int uiox_emmc_proto_write(uiox_emmc_proto_t *proto,
                           uiox_emmc_part_t part, uint32_t lba,
                           const uint8_t *buf, uint32_t sectors)
{
    if (!proto || !proto->initialized || !buf || !sectors)
        return -EINVAL;
    if (proto->eif->hw->active_part != part) {
        int rc = uiox_emmc_if_select_part(proto->eif, part);
        if (rc < 0) return rc;
    }
    return uiox_emmc_if_write(proto->eif, lba, buf, sectors);
}

int uiox_emmc_proto_flush(uiox_emmc_proto_t *proto)
{
    if (!proto || !proto->initialized) return -EINVAL;
    if (!proto->cache_enabled) return 0;
    /* EXT_CSD[32] FLUSH_CACHE = 1 */
    int rc = uiox_emmc_if_switch(proto->eif, MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_FLUSH_CACHE, 1u, 0u);
    printf("  [proto] Cache flush  rc=%d\n", rc);
    return rc;
}

int uiox_emmc_proto_trim(uiox_emmc_proto_t *proto,
                          uint32_t lba, uint32_t sectors)
{
    if (!proto || !proto->initialized) return -EINVAL;
    if (!proto->eif->hw->ident.trim_supported) return -ENOTSUP;
    /* CMD35 ERASE_GROUP_START, CMD36 ERASE_GROUP_END, CMD38 ERASE(TRIM) */
    uint32_t resp;
    int rc = uiox_emmc_if_send_cmd(proto->eif, MMC_CMD35_ERASE_START,
                                    lba, EMMC_RESP_R1, &resp);
    if (rc < 0) return rc;
    rc = uiox_emmc_if_send_cmd(proto->eif, MMC_CMD36_ERASE_END,
                                lba + sectors - 1u,
                                EMMC_RESP_R1, &resp);
    if (rc < 0) return rc;
    rc = uiox_emmc_if_send_cmd(proto->eif, MMC_CMD38_ERASE,
                                0x00000001u,   /* arg=1: TRIM */
                                EMMC_RESP_R1B, &resp);
    printf("  [proto] TRIM  lba=%u  sectors=%u  rc=%d\n",
           lba, sectors, rc);
    return rc;
}

int uiox_emmc_proto_cache_ctrl(uiox_emmc_proto_t *proto, bool enable)
{
    if (!proto) return -EINVAL;
    int rc = uiox_emmc_if_switch(proto->eif, MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_CACHE_CTRL,
                                  enable ? 1u : 0u, 0u);
    if (rc == 0) proto->cache_enabled = enable;
    return rc;
}

int uiox_emmc_proto_bkops(uiox_emmc_proto_t *proto)
{
    if (!proto || !proto->initialized) return -EINVAL;
    if (!proto->eif->hw->ident.bkops_supported) return -ENOTSUP;
    /* EXT_CSD[164] BKOPS_START = 1 */
    int rc = uiox_emmc_if_switch(proto->eif, MMC_SWITCH_WRITE_BYTE,
                                  164u, 1u, 0u);
    printf("  [proto] BKOPS start  rc=%d\n", rc);
    return rc;
}

int uiox_emmc_proto_pon(uiox_emmc_proto_t *proto)
{
    if (!proto || !proto->initialized) return -EINVAL;
    if (!proto->eif->hw->ident.pon_supported) return 0;
    /* EXT_CSD[34] POWER_OFF_NOTIFICATION = 2 (POWERED_OFF_LONG) */
    int rc = uiox_emmc_if_switch(proto->eif, MMC_SWITCH_WRITE_BYTE,
                                  EXT_CSD_POWER_OFF_NOTIF, 2u, 0u);
    printf("  [proto] Power-off notification sent  rc=%d\n", rc);
    return rc;
}

int uiox_emmc_proto_health_check(uiox_emmc_proto_t *proto,
                                   uint8_t *pre_eol,
                                   uint8_t *life_a,
                                   uint8_t *life_b)
{
    if (!proto || !proto->initialized) return -EINVAL;
    /* Re-read EXT_CSD to get fresh health data */
    uint8_t ext_csd[UIOX_EMMC_EXT_CSD_LEN];
    int rc = uiox_emmc_if_read_ext_csd(proto->eif, ext_csd);
    if (rc < 0) return rc;
    if (pre_eol) *pre_eol = ext_csd[EXT_CSD_PRE_EOL_INFO];
    if (life_a)  *life_a  = ext_csd[EXT_CSD_DEVICE_LIFE_EST_A];
    if (life_b)  *life_b  = ext_csd[EXT_CSD_DEVICE_LIFE_EST_B];
    /* Update cached ident */
    proto->eif->hw->ident.pre_eol_info = *pre_eol;
    proto->eif->hw->ident.life_est_a   = *life_a;
    proto->eif->hw->ident.life_est_b   = *life_b;
    return 0;
}
