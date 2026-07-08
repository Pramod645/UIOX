/* ── uiox_fw_nvme.c (continued) ────────────────────────────── */
#include "../include/uiox_fw_nvme.h"
#define OPS_NVME(d) ((const uiox_nvme_ops_t*)(d)->priv)

static inline void nw(uintptr_t b,uint32_t o,uint32_t v)
{ *((volatile uint32_t*)(b+o))=v; }
static inline uint32_t nr(uintptr_t b,uint32_t o)
{ return *((volatile uint32_t*)(b+o)); }

static uiox_fw_err_t nvme_init(uiox_nvme_dev_t *d)
{
    /* Disable controller */
    nw(d->bar0, NVME_REG_CC, 0u);
    for (uint32_t i=0u;i<100000u;i++)
        if (!(nr(d->bar0, NVME_REG_CSTS) & NVME_CSTS_RDY)) break;
    /* Set admin queue size 32/32 */
    nw(d->bar0, NVME_REG_AQA, 0x001F001Fu);
    /* Enable: IOSQES=6, IOCQES=4, AMS=RR, CSS=NVM */
    nw(d->bar0, NVME_REG_CC,
       NVME_CC_EN | (6u<<16u) | (4u<<20u));
    /* Wait RDY */
    for (uint32_t i=0u;i<1000000u;i++) {
        uint32_t csts = nr(d->bar0, NVME_REG_CSTS);
        if (csts & NVME_CSTS_CFS) return UIOX_FW_ERR_IO;
        if (csts & NVME_CSTS_RDY) break;
    }
    d->nsid = 1u;
    d->capacity_lba   = 0x1000000u;   /* 8 GB placeholder */
    d->capacity_bytes = d->capacity_lba * NVME_BLOCK_SIZE;
    d->initialized    = true;
    return UIOX_FW_OK;
}

static void nvme_deinit(uiox_nvme_dev_t *d)
{
    nw(d->bar0, NVME_REG_CC, 0u);
    d->initialized = false;
}

static uiox_fw_err_t nvme_read(uiox_nvme_dev_t *d, uint64_t lba,
                                 uint8_t *buf, uint32_t sectors)
{
    /* Firmware read: in real HW submit SQE and poll CQE */
    (void)buf;
    if (lba + sectors > d->capacity_lba) return UIOX_FW_ERR_IO;
    return UIOX_FW_OK;
}

static uiox_fw_err_t nvme_write(uiox_nvme_dev_t *d, uint64_t lba,
                                  const uint8_t *buf, uint32_t sectors)
{
    (void)buf;
    if (lba + sectors > d->capacity_lba) return UIOX_FW_ERR_IO;
    return UIOX_FW_OK;
}

static uiox_fw_err_t nvme_flush(uiox_nvme_dev_t *d)
{ (void)d; return UIOX_FW_OK; }

static uiox_fw_err_t nvme_trim(uiox_nvme_dev_t *d, uint64_t lba,
                                  uint32_t sectors)
{ (void)d;(void)lba;(void)sectors; return UIOX_FW_OK; }

static uiox_fw_err_t nvme_identify(uiox_nvme_dev_t *d)
{
    const char *m = "UIOX NVMe SSD";
    for (int i=0;m[i]&&i<40;i++) d->model[i]=m[i];
    const char *s = "UIOX0000001";
    for (int i=0;s[i]&&i<20;i++) d->serial[i]=s[i];
    d->volatile_wc    = true;
    d->trim_supported = true;
    return UIOX_FW_OK;
}

static const uiox_nvme_ops_t nvme_bar0_ops = {
    nvme_init, nvme_deinit, nvme_read, nvme_write,
    nvme_flush, nvme_trim, nvme_identify
};

uiox_fw_err_t uiox_fw_nvme_init(uiox_nvme_dev_t *d,
                                   const uiox_nvme_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o; uiox_fw_err_t rc=o->init(d);
  if(rc==UIOX_FW_OK&&o->identify)o->identify(d);
  return rc; }
void uiox_fw_nvme_deinit(uiox_nvme_dev_t *d)
{ if(d&&d->priv&&OPS_NVME(d)->deinit)OPS_NVME(d)->deinit(d); }
uiox_fw_err_t uiox_fw_nvme_read(uiox_nvme_dev_t *d,uint64_t lba,
                                   uint8_t *buf,uint32_t sectors)
{ if(!d||!d->priv||!OPS_NVME(d)->read)return UIOX_FW_ERR_INVAL;
  return OPS_NVME(d)->read(d,lba,buf,sectors); }
uiox_fw_err_t uiox_fw_nvme_write(uiox_nvme_dev_t *d,uint64_t lba,
                                    const uint8_t *buf,uint32_t sectors)
{ if(!d||!d->priv||!OPS_NVME(d)->write)return UIOX_FW_ERR_INVAL;
  return OPS_NVME(d)->write(d,lba,buf,sectors); }
uiox_fw_err_t uiox_fw_nvme_flush(uiox_nvme_dev_t *d)
{ if(!d||!d->priv||!OPS_NVME(d)->flush)return UIOX_FW_ERR_INVAL;
  return OPS_NVME(d)->flush(d); }
uiox_fw_err_t uiox_fw_nvme_init_bar0(uiox_nvme_dev_t *d,
                                        uintptr_t bar0, uint32_t irq)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;
    for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->bar0=bar0; d->irq=irq;
    return uiox_fw_nvme_init(d, &nvme_bar0_ops);
}
