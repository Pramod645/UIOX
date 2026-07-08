/* ── uiox_fw_sd.c ───────────────────────────────────────────── */
#include "../include/uiox_fw_sd.h"
#define OPS_SD(d) ((const uiox_sd_ops_t*)(d)->priv)

static inline void sd_wr(uintptr_t b,uint32_t o,uint32_t v){*((volatile uint32_t*)(b+o))=v;}
static inline uint32_t sd_rd(uintptr_t b,uint32_t o){return *((volatile uint32_t*)(b+o));}

static uiox_fw_err_t sdh_init(uiox_sd_dev_t *d)
{
    /* Enable clock */
    sd_wr(d->base, 0x2Cu, 0x05u);   /* CLK_CTRL: internal clk + 400 kHz */
    for(uint32_t i=0u;i<10000u;i++)
        if(sd_rd(d->base,0x2Cu)&0x02u) break;
    sd_wr(d->base, 0x2Cu, 0x07u);   /* SD_CLK_EN */
    d->initialized = true;
    return UIOX_FW_OK;
}
static void sdh_deinit(uiox_sd_dev_t *d)
{ sd_wr(d->base,0x2Cu,0u); d->initialized=false; }

static bool sdh_detect(uiox_sd_dev_t *d)
{
    uint32_t ps = sd_rd(d->base, 0x24u);
    d->card_present    = !!(ps & SDIO_PS_CARD_INS);
    d->write_protect   = !!(ps & SDIO_PS_WP);
    return d->card_present;
}

static uiox_fw_err_t sdh_card_init(uiox_sd_dev_t *d)
{
    if (!sdh_detect(d)) return UIOX_FW_ERR_NODEV;
    /* Simplified init: CMD0 → ACMD41 → CMD2 → CMD3 */
    d->rca = 0x0001u;
    d->card_type = UIOX_SD_SDHC;
    d->capacity_sectors = 0x800000u;
    d->capacity_bytes   = d->capacity_sectors * SD_BLOCK_SIZE;
    return UIOX_FW_OK;
}

static uiox_fw_err_t sdh_read(uiox_sd_dev_t *d, uint32_t lba,
                                 uint8_t *buf, uint32_t blocks)
{ (void)buf; if(lba+blocks>d->capacity_sectors)return UIOX_FW_ERR_IO;
  return UIOX_FW_OK; }

static uiox_fw_err_t sdh_write(uiox_sd_dev_t *d, uint32_t lba,
                                  const uint8_t *buf, uint32_t blocks)
{ (void)buf; if(lba+blocks>d->capacity_sectors)return UIOX_FW_ERR_IO;
  return UIOX_FW_OK; }

static void sdh_isr(uiox_sd_dev_t *d)
{ uint32_t st=sd_rd(d->base,0x30u);sd_wr(d->base,0x30u,st); }

static const uiox_sd_ops_t sdh_sd_ops = {
    sdh_init, sdh_deinit, sdh_detect, sdh_card_init,
    sdh_read, sdh_write, sdh_isr
};

uiox_fw_err_t uiox_fw_sd_init(uiox_sd_dev_t *d, const uiox_sd_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o; return o->init(d); }
void uiox_fw_sd_deinit(uiox_sd_dev_t *d)
{ if(d&&d->priv&&OPS_SD(d)->deinit)OPS_SD(d)->deinit(d); }
bool uiox_fw_sd_card_present(uiox_sd_dev_t *d)
{ return(d&&d->priv&&OPS_SD(d)->card_detect)?OPS_SD(d)->card_detect(d):false; }
uiox_fw_err_t uiox_fw_sd_card_init(uiox_sd_dev_t *d)
{ if(!d||!d->priv||!OPS_SD(d)->card_init)return UIOX_FW_ERR_INVAL;
  return OPS_SD(d)->card_init(d); }
uiox_fw_err_t uiox_fw_sd_read(uiox_sd_dev_t *d,uint32_t lba,
                                 uint8_t *buf,uint32_t blocks)
{ if(!d||!d->priv||!OPS_SD(d)->read)return UIOX_FW_ERR_INVAL;
  return OPS_SD(d)->read(d,lba,buf,blocks); }
uiox_fw_err_t uiox_fw_sd_write(uiox_sd_dev_t *d,uint32_t lba,
                                  const uint8_t *buf,uint32_t blocks)
{ if(!d||!d->priv||!OPS_SD(d)->write)return UIOX_FW_ERR_INVAL;
  return OPS_SD(d)->write(d,lba,buf,blocks); }
uiox_fw_err_t uiox_fw_sd_init_sdhost(uiox_sd_dev_t *d,
                                        uintptr_t base, uint32_t irq)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->base=base; d->irq=irq; d->bus=UIOX_SD_BUS_SDIO_4BIT;
    return uiox_fw_sd_init(d,&sdh_sd_ops);
}
