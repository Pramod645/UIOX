#include "uiox_bios_device.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

int uiox_bios_open(uiox_bios_device_t *dev, const uiox_bios_open_params_t *p)
{
    if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
    memset(dev, 0, sizeof(*dev));
    dev->hw = p->hw;
    int rc = uiox_bios_hw_init(p->hw, p->hw_ops);
    if (rc < 0) return rc;
    rc = uiox_bios_subsys_init(&dev->subsys, p->hw, p->nvram_flash_offset);
    if (rc < 0) return rc;
    if (p->evt_cb)
        uiox_bios_subsys_set_cb(&dev->subsys, p->evt_cb, p->evt_ctx);
    dev->open = true;
    return 0;
}

int  uiox_bios_start(uiox_bios_device_t *dev)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_subsys_start(&dev->subsys); }

void uiox_bios_stop(uiox_bios_device_t *dev)
{ if (!dev||!dev->open) return; uiox_bios_subsys_stop(&dev->subsys); }

void uiox_bios_close(uiox_bios_device_t *dev)
{
    if (!dev||!dev->open) return;
    uiox_bios_stop(dev);
    uiox_bios_hw_deinit(dev->hw);
    dev->open = false;
}

void uiox_bios_tick(uiox_bios_device_t *dev, uint32_t now_ms)
{ if (!dev||!dev->open) return; uiox_bios_subsys_tick(&dev->subsys, now_ms); }

int uiox_bios_flash_read(uiox_bios_device_t *dev,
                          uint32_t offset, void *buf, uint32_t len)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_if_read(&dev->subsys.bif, offset, buf, len); }

int uiox_bios_flash_write(uiox_bios_device_t *dev,
                           uint32_t offset, const void *buf, uint32_t len)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_if_write(&dev->subsys.bif, offset, buf, len); }

int uiox_bios_flash_update(uiox_bios_device_t *dev,
                            uint32_t offset, const void *image, uint32_t size)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_subsys_update(&dev->subsys, offset, image, size); }

int uiox_bios_flash_verify(uiox_bios_device_t *dev,
                            uint32_t offset, const void *image, uint32_t size)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_subsys_verify(&dev->subsys, offset, image, size); }

int  uiox_bios_set_wp(uiox_bios_device_t *dev, bool protect)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_hw_set_wp(dev->hw, protect); }

bool uiox_bios_get_wp(const uiox_bios_device_t *dev)
{ return dev && dev->open ? dev->hw->wp_active : true; }

int uiox_bios_var_get(uiox_bios_device_t *dev,
                       const uiox_efi_guid_t *guid, const char *name,
                       void *data, uint32_t *size, uint32_t *attrs)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_nvram_get_var(&dev->subsys.nvram,
                                   guid, name, data, size, attrs); }

int uiox_bios_var_set(uiox_bios_device_t *dev,
                       const uiox_efi_guid_t *guid, const char *name,
                       const void *data, uint32_t size, uint32_t attrs)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_nvram_set_var(&dev->subsys.nvram,
                                   guid, name, data, size, attrs); }

int uiox_bios_var_del(uiox_bios_device_t *dev,
                       const uiox_efi_guid_t *guid, const char *name)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_nvram_del_var(&dev->subsys.nvram, guid, name); }

uint8_t uiox_bios_cmos_get(uiox_bios_device_t *dev, uint8_t idx)
{ return dev&&dev->open ? uiox_bios_nvram_cmos_get(&dev->subsys.nvram,idx):0u;}

void uiox_bios_cmos_set(uiox_bios_device_t *dev, uint8_t idx, uint8_t val)
{ if (dev&&dev->open) uiox_bios_nvram_cmos_set(&dev->subsys.nvram,idx,val);}

int uiox_bios_tpm_send(uiox_bios_device_t *dev,
                        const uint8_t *cmd, uint16_t cmd_len,
                        uint8_t *resp, uint16_t *resp_len)
{ if (!dev||!dev->open) return -EINVAL;
  return uiox_bios_hw_tpm_send(dev->hw, cmd, cmd_len, resp, resp_len); }

void uiox_bios_print_info(const uiox_bios_device_t *dev)
{
    if (!dev) return;
    const uiox_bios_hw_t *hw = dev->hw;
    printf("  BIOS type      : %s\n", uiox_bios_type_name(hw->type));
    printf("  BIOS version   : %s\n", hw->version);
    printf("  BIOS vendor    : %s\n", hw->vendor);
    printf("  Build date     : %08u\n", hw->build_date);
    printf("  Flash size     : %u KB\n", hw->geo.total_bytes / 1024u);
    printf("  Flash JEDEC    : %02X %04X\n",
           hw->geo.jedec_mfr, hw->geo.jedec_dev);
    printf("  Sector size    : %u B\n", hw->geo.sector_bytes);
    printf("  Capabilities   : 0x%08X\n", hw->caps);
    printf("  WP active      : %s\n", hw->wp_active ? "YES" : "NO");
    printf("  Regions        : %u\n", hw->num_regions);
    for (uint8_t i = 0; i < hw->num_regions; i++) {
        const uiox_bios_region_t *r = &hw->regions[i];
        printf("    %-16s  off=0x%06X  size=0x%06X  %s%s%s\n",
               r->name ? r->name : "?", r->offset, r->size,
               r->readable ? "R" : "-",
               r->writable ? "W" : "-",
               r->protected? "P" : "-");
    }
    uiox_bios_svc_print_post(&dev->subsys.svc);
}

void uiox_bios_print_stats(const uiox_bios_device_t *dev)
{
    if (!dev) return;
    printf("  State          : %s\n",
           uiox_bios_state_name(dev->subsys.state));
    printf("  Uptime         : %llu ms\n",
           (unsigned long long)dev->subsys.uptime_ms);
    printf("  Tick count     : %u\n", dev->subsys.tick_count);
    printf("  EFI vars       : %u / %u\n",
           dev->subsys.nvram.var_count, UIOX_BIOS_MAX_VARS);
    printf("  NVRAM dirty    : %s\n",
           dev->subsys.nvram.dirty ? "yes" : "no");
    printf("  CMOS valid     : %s\n",
           uiox_bios_nvram_cmos_valid(&dev->subsys.nvram) ? "yes" : "no");
    uiox_bios_if_stats_t st;
    uiox_bios_if_stats_get(&dev->subsys.bif, &st);
    printf("  Flash read     : %llu bytes\n",
           (unsigned long long)st.bytes_read);
    printf("  Flash written  : %llu bytes\n",
           (unsigned long long)st.bytes_written);
    printf("  Sectors erased : %u\n", st.sectors_erased);
    printf("  WP removes     : %u\n", st.wp_removes);
    printf("  Write errors   : %u\n", st.write_errors);
    printf("  Buf pool free  : %u / %u\n",
           uiox_bios_buf_free_count(), UIOX_BIOS_BUF_POOL_SIZE);
    uiox_bios_svc_print_memmap(&dev->subsys.svc);
}

const char *uiox_bios_state_name(uiox_bios_subsys_state_t s)
{
    switch (s) {
    case UIOX_BIOS_SUBSYS_STOPPED: return "STOPPED";
    case UIOX_BIOS_SUBSYS_POST:    return "POST";
    case UIOX_BIOS_SUBSYS_RUNTIME: return "RUNTIME";
    case UIOX_BIOS_SUBSYS_UPDATE:  return "UPDATE";
    case UIOX_BIOS_SUBSYS_ERROR:   return "ERROR";
    default:                        return "UNKNOWN";
    }
}

const char *uiox_bios_evt_name(uiox_bios_evt_t evt)
{
    switch (evt) {
    case UIOX_BIOS_EVT_POST_DONE:          return "POST_DONE";
    case UIOX_BIOS_EVT_POST_ERROR:         return "POST_ERROR";
    case UIOX_BIOS_EVT_FLASH_WRITE_START:  return "FLASH_WRITE_START";
    case UIOX_BIOS_EVT_FLASH_WRITE_DONE:   return "FLASH_WRITE_DONE";
    case UIOX_BIOS_EVT_FLASH_WRITE_ERROR:  return "FLASH_WRITE_ERROR";
    case UIOX_BIOS_EVT_WP_REMOVED:         return "WP_REMOVED";
    case UIOX_BIOS_EVT_WP_RESTORED:        return "WP_RESTORED";
    case UIOX_BIOS_EVT_VAR_SET:            return "VAR_SET";
    case UIOX_BIOS_EVT_VAR_DEL:            return "VAR_DEL";
    case UIOX_BIOS_EVT_BOOT_SELECT:        return "BOOT_SELECT";
    case UIOX_BIOS_EVT_SECURE_BOOT:        return "SECURE_BOOT";
    default:                                return "UNKNOWN";
    }
}

const char *uiox_bios_type_name(uiox_bios_type_t t)
{
    switch (t) {
    case UIOX_BIOS_TYPE_UEFI:     return "UEFI/EDK2";
    case UIOX_BIOS_TYPE_LEGACY:   return "Legacy BIOS";
    case UIOX_BIOS_TYPE_COREBOOT: return "coreboot";
    case UIOX_BIOS_TYPE_UBOOT:    return "U-Boot";
    default:                       return "UNKNOWN";
    }
}
