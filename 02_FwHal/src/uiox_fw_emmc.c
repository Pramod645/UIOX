/* ── uiox_fw_emmc.c ─────────────────────────────────────────── */
#include "../include/uiox_fw_emmc.h"
#define OPS_EMMC(d) ((const uiox_emmc_ops_t *)(d)->priv)
/* SDIO host register helpers */
static inline void emmc_wr(uintptr_t b, uint32_t o, uint32_t v)
{ *((volatile uint32_t*)(b+o))=v; }
static inline uint32_t emmc_rd(uintptr_t b, uint32_t o)
{ return *((volatile uint32_t*)(b+o)); }
static uiox_fw_err_t sdhost_send_cmd(uiox_emmc_dev_t *dev,
                                       uint8_t cmd, uint32_t arg,
                                       uint32_t *resp)
{
    uintptr_t b = dev->base;
    /* Wait CMD inhibit clear */
    for (uint32_t i=0u;i<100000u;i++)
        if (!(emmc_rd(b, EMMC_HC_PRESENT_STATE) & EMMC_PS_CMD_INHIBIT)) break;
    emmc_wr(b, EMMC_HC_ARG, arg);
    emmc_wr(b, EMMC_HC_CMD, ((uint32_t)cmd << 8u) | 0x1Au);  /* R1 */
    /* Poll for completion */
    for (uint32_t i=0u;i<100000u;i++) {
        uint32_t s = emmc_rd(b, EMMC_HC_INT_STATUS);
        if (s & EMMC_INT_CMD_COMPLETE) {
            emmc_wr(b, EMMC_HC_INT_STATUS, EMMC_INT_CMD_COMPLETE);
            if (resp) *resp = emmc_rd(b, EMMC_HC_RESP0);
            return UIOX_FW_OK;
        }
        if (s & EMMC_INT_ERROR) {
            emmc_wr(b, EMMC_HC_INT_STATUS, s);
            return UIOX_FW_ERR_IO;
        }
    }
    return UIOX_FW_ERR_TIMEOUT;
}
static uiox_fw_err_t sdhost_init(uiox_emmc_dev_t *dev)
{
    uintptr_t b = dev->base;
    emmc_wr(b, EMMC_HC_CLK_CTRL, EMMC_CLK_INT_EN);
    for (uint32_t i=0u;i<10000u;i++)
        if (emmc_rd(b, EMMC_HC_CLK_CTRL) & EMMC_CLK_STABLE) break;
    emmc_wr(b, EMMC_HC_CLK_CTRL,
            EMMC_CLK_INT_EN | EMMC_CLK_STABLE | EMMC_CLK_SD_EN);
    emmc_wr(b, EMMC_HC_INT_EN, EMMC_INT_CMD_COMPLETE |
            EMMC_INT_XFER_COMPLETE | EMMC_INT_ERROR);
    /* CMD0 → CMD1 → CMD2 → CMD3 → CMD7 sequence */
    uint32_t resp = 0u;
    sdhost_send_cmd(dev, MMC_CMD0_IDLE,   0u, NULL);
    for (uint32_t i=0u;i<1000u;i++) {
        sdhost_send_cmd(dev, MMC_CMD1_SEND_OP_COND,
                        0x40FF8080u, &resp);
        if (resp & (1u<<31)) break;
    }
    sdhost_send_cmd(dev, MMC_CMD2_ALL_CID, 0u, NULL);
    dev->rca = 1u;
    sdhost_send_cmd(dev, MMC_CMD3_SET_RCA,
                    (uint32_t)dev->rca << 16u, &resp);
    sdhost_send_cmd(dev, MMC_CMD7_SELECT,
                    (uint32_t)dev->rca << 16u, &resp);
    dev->capacity_sectors = 0x1000000u;  /* 8 GB placeholder */
    dev->capacity_bytes   = dev->capacity_sectors * EMMC_BLOCK_SIZE;
    dev->initialized      = true;
    return UIOX_FW_OK;
}
static void sdhost_deinit(uiox_emmc_dev_t *dev)
{ emmc_wr(dev->base, EMMC_HC_CLK_CTRL, 0u); dev->initialized=false; }
static uiox_fw_err_t sdhost_read(uiox_emmc_dev_t *dev,
                                    uiox_emmc_part_t part,
                                    uint32_t lba,
                                    uint8_t *buf, uint32_t sectors)
{
    (void)part;
    for (uint32_t s=0u; s<sectors; s++) {
        uint32_t resp=0u;
        uiox_fw_err_t rc = sdhost_send_cmd(dev, MMC_CMD17_READ,
                                             lba+s, &resp);
        if (rc != UIOX_FW_OK) return rc;
        /* In real HW: wait XFER_COMPLETE + copy data port */
        (void)buf; buf += EMMC_BLOCK_SIZE;
    }
    return UIOX_FW_OK;
}
static uiox_fw_err_t sdhost_write(uiox_emmc_dev_t *dev,
                                    uiox_emmc_part_t part,
                                    uint32_t lba,
                                    const uint8_t *buf, uint32_t sectors)
{
    (void)part; (void)buf;
    for (uint32_t s=0u; s<sectors; s++) {
        uint32_t resp=0u;
        uiox_fw_err_t rc = sdhost_send_cmd(dev, MMC_CMD24_WRITE,
                                             lba+s, &resp);
        if (rc != UIOX_FW_OK) return rc;
    }
    return UIOX_FW_OK;
}
static uiox_fw_err_t sdhost_flush(uiox_emmc_dev_t *dev)
{ (void)dev; return UIOX_FW_OK; }
static uiox_fw_err_t sdhost_switch_speed(uiox_emmc_dev_t *dev,
                                           uiox_emmc_speed_t speed)
{ dev->speed=speed; return UIOX_FW_OK; }
static uiox_fw_err_t sdhost_select_part(uiox_emmc_dev_t *dev,
                                          uiox_emmc_part_t part)
{
    /* SWITCH command to PARTITION_CONFIG EXT_CSD[179] */
    uint8_t cfg = (uint8_t)part & 0x07u;
    uint32_t arg = (0x03u<<24u)|(179u<<16u)|((uint32_t)cfg<<8u);
    uint32_t resp=0u;
    uiox_fw_err_t rc = sdhost_send_cmd(dev, MMC_CMD6_SWITCH, arg, &resp);
    if (rc == UIOX_FW_OK) dev->active_part = part;
    return rc;
}
static const uiox_emmc_ops_t sdhost_emmc_ops = {
    .init         = sdhost_init,
    .deinit       = sdhost_deinit,
    .read         = sdhost_read,
    .write        = sdhost_write,
    .flush        = sdhost_flush,
    .switch_speed = sdhost_switch_speed,
    .select_part  = sdhost_select_part,
};
uiox_fw_err_t uiox_fw_emmc_init(uiox_emmc_dev_t *dev,
                                    const uiox_emmc_ops_t *ops)
{ if(!dev||!ops||!ops->init) return UIOX_FW_ERR_INVAL;
  dev->priv=(void*)ops; return ops->init(dev); }
void uiox_fw_emmc_deinit(uiox_emmc_dev_t *dev)
{ if(dev&&dev->priv&&OPS_EMMC(dev)->deinit) OPS_EMMC(dev)->deinit(dev); }
uiox_fw_err_t uiox_fw_emmc_read(uiox_emmc_dev_t *dev,
                                   uiox_emmc_part_t part, uint32_t lba,
                                   uint8_t *buf, uint32_t sectors)
{ if(!dev||!dev->priv||!OPS_EMMC(dev)->read) return UIOX_FW_ERR_INVAL;
  return OPS_EMMC(dev)->read(dev,part,lba,buf,sectors); }
uiox_fw_err_t uiox_fw_emmc_write(uiox_emmc_dev_t *dev,
                                    uiox_emmc_part_t part, uint32_t lba,
                                    const uint8_t *buf, uint32_t sectors)
{ if(!dev||!dev->priv||!OPS_EMMC(dev)->write) return UIOX_FW_ERR_INVAL;
  return OPS_EMMC(dev)->write(dev,part,lba,buf,sectors); }
uiox_fw_err_t uiox_fw_emmc_flush(uiox_emmc_dev_t *dev)
{ if(!dev||!dev->priv||!OPS_EMMC(dev)->flush) return UIOX_FW_ERR_INVAL;
  return OPS_EMMC(dev)->flush(dev); }
uiox_fw_err_t uiox_fw_emmc_init_sdhost(uiox_emmc_dev_t *dev, uintptr_t base)
{
    if(!dev) return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)dev; for(size_t i=0u;i<sizeof(*dev);i++) p[i]=0u;
    dev->base=base; dev->speed=UIOX_EMMC_SPEED_IDENT;
    dev->bus_width=1u;
    return uiox_fw_emmc_init(dev, &sdhost_emmc_ops);
}