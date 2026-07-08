/* ── uiox_fw_sata.c ─────────────────────────────────────────── */
#include "../include/uiox_fw_sata.h"
#define OPS_SATA(d) ((const uiox_sata_ops_t*)(d)->priv)

static inline void sa_wr(uintptr_t b,uint32_t o,uint32_t v){*((volatile uint32_t*)(b+o))=v;}
static inline uint32_t sa_rd(uintptr_t b,uint32_t o){return *((volatile uint32_t*)(b+o));}

static uiox_fw_err_t ahci_init(uiox_sata_dev_t *d)
{
    uintptr_t b = d->bar5;
    /* Enable AHCI mode */
    sa_wr(b, AHCI_GHC_GHC, AHCI_GHC_GHC_AE | AHCI_GHC_GHC_IE);
    /* Check port presence */
    uint32_t pi = sa_rd(b, AHCI_GHC_PI);
    if (!(pi & (1u << d->port))) return UIOX_FW_ERR_NODEV;
    /* Start port FIS receive and command engine */
    uintptr_t pb = b + AHCI_PORT_BASE(d->port);
    sa_wr(pb, AHCI_PX_CMD,
          sa_rd(pb, AHCI_PX_CMD) | AHCI_PX_CMD_FRE);
    sa_wr(pb, AHCI_PX_CMD,
          sa_rd(pb, AHCI_PX_CMD) | AHCI_PX_CMD_ST);
    /* Wait for device ready */
    uint32_t ssts = sa_rd(pb, AHCI_PX_SSTS);
    if ((ssts & 0x0Fu) != 0x03u) return UIOX_FW_ERR_NODEV;
    /* Check ATA signature */
    if (sa_rd(pb, AHCI_PX_SIG) != AHCI_SIG_ATA)
        return UIOX_FW_ERR_UNSUP;
    d->initialized = true;
    return UIOX_FW_OK;
}

static void ahci_deinit(uiox_sata_dev_t *d)
{
    uintptr_t pb = d->bar5 + AHCI_PORT_BASE(d->port);
    sa_wr(pb, AHCI_PX_CMD,
          sa_rd(pb, AHCI_PX_CMD) & ~(AHCI_PX_CMD_ST | AHCI_PX_CMD_FRE));
    d->initialized = false;
}

static uiox_fw_err_t ahci_read(uiox_sata_dev_t *d, uint64_t lba,
                                  uint8_t *buf, uint32_t sectors)
{
    (void)buf; /* DMA in real HW */
    if (lba + sectors > d->capacity_sectors) return UIOX_FW_ERR_IO;
    return UIOX_FW_OK;
}

static uiox_fw_err_t ahci_write(uiox_sata_dev_t *d, uint64_t lba,
                                   const uint8_t *buf, uint32_t sectors)
{
    (void)buf;
    if (lba + sectors > d->capacity_sectors) return UIOX_FW_ERR_IO;
    return UIOX_FW_OK;
}

static uiox_fw_err_t ahci_flush(uiox_sata_dev_t *d) { (void)d; return UIOX_FW_OK; }

static uiox_fw_err_t ahci_identify(uiox_sata_dev_t *d)
{
    const char *m = "UIOX SATA SSD";
    for (int i=0;m[i]&&i<40;i++) d->model[i]=m[i];
    d->capacity_sectors = 0x800000u;   /* 4 GB */
    d->capacity_bytes   = d->capacity_sectors * SATA_SECTOR_SIZE;
    d->ncq_supported    = true;
    d->smart_supported  = true;
    return UIOX_FW_OK;
}

static uiox_fw_err_t ahci_smart(uiox_sata_dev_t *d, uint8_t *buf)
{ (void)d; (void)buf; return UIOX_FW_OK; }

static const uiox_sata_ops_t ahci_sata_ops = {
    ahci_init, ahci_deinit, ahci_read, ahci_write,
    ahci_flush, ahci_identify, ahci_smart
};

uiox_fw_err_t uiox_fw_sata_init(uiox_sata_dev_t *d, const uiox_sata_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;
  d->priv=(void*)o;
  uiox_fw_err_t rc=o->init(d);
  if(rc==UIOX_FW_OK&&o->identify)o->identify(d);
  return rc; }
void uiox_fw_sata_deinit(uiox_sata_dev_t *d)
{ if(d&&d->priv&&OPS_SATA(d)->deinit)OPS_SATA(d)->deinit(d); }
uiox_fw_err_t uiox_fw_sata_read(uiox_sata_dev_t *d,uint64_t lba,
                                   uint8_t *buf,uint32_t sectors)
{ if(!d||!d->priv||!OPS_SATA(d)->read)return UIOX_FW_ERR_INVAL;
  return OPS_SATA(d)->read(d,lba,buf,sectors); }
uiox_fw_err_t uiox_fw_sata_write(uiox_sata_dev_t *d,uint64_t lba,
                                    const uint8_t *buf,uint32_t sectors)
{ if(!d||!d->priv||!OPS_SATA(d)->write)return UIOX_FW_ERR_INVAL;
  return OPS_SATA(d)->write(d,lba,buf,sectors); }
uiox_fw_err_t uiox_fw_sata_flush(uiox_sata_dev_t *d)
{ if(!d||!d->priv||!OPS_SATA(d)->flush)return UIOX_FW_ERR_INVAL;
  return OPS_SATA(d)->flush(d); }
uiox_fw_err_t uiox_fw_sata_init_ahci(uiox_sata_dev_t *d,
                                        uintptr_t bar5, uint8_t port,
                                        uint32_t irq)
{
    if(!d)return UIOX_FW_ERR_INVAL;
    uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
    d->bar5=bar5; d->port=port; d->irq=irq;
    return uiox_fw_sata_init(d,&ahci_sata_ops);
}
