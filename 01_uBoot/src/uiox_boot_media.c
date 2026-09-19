/*
 * 01_uBoot/src/boot_media/uiox_boot_media.c
 * Boot-media dispatcher: registration + selection only.
 * No device protocol lives here — each driver (virtio, none, …)
 * defines its own ops table and its own constructor.
 */
#include "uiox_boot.h"
#include "uiox_boot_media.h"

#define UIOX_MEDIA_MAX_DRIVERS 8u

static const uiox_boot_media_ops_t *s_drivers[UIOX_MEDIA_MAX_DRIVERS];
static uint32_t s_driver_count;
static const uiox_boot_media_ops_t *s_active;


void uiox_boot_media_register(const uiox_boot_media_ops_t *ops)
{
    if (!ops || s_driver_count >= UIOX_MEDIA_MAX_DRIVERS) return;
    s_drivers[s_driver_count++] = ops;
}

uiox_boot_media_kind_t uiox_boot_media_select(void)
{
    for (uint32_t i = 0; i < s_driver_count; i++) {
        const uiox_boot_media_ops_t *d = s_drivers[i];
        if (!d->present || !d->present()) continue;
        if (d->init && d->init() != UIOX_BOOT_OK) continue;
        s_active = d;
        uiox_boot_printf("  [media] selected: %s\n", d->name);
        return d->kind;
    }
    s_active = 0;
    uiox_boot_puts("  [media] no boot media found\n");
    return UIOX_MEDIA_NONE;
}

uiox_boot_err_t uiox_boot_media_read_block(uint32_t blk, uint32_t nblocks,
                                           void *buf)
{
    if (!s_active || !s_active->read_block) return UIOX_BOOT_ERR_IO;
    return s_active->read_block(blk, nblocks, buf);
}

uiox_boot_media_kind_t uiox_boot_media_active(void)
{ return s_active ? s_active->kind : UIOX_MEDIA_NONE; }

const char *uiox_boot_media_active_name(void)
{ return s_active ? s_active->name : "(none)"; }
