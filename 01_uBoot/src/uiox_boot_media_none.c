/*
 * 01_uBoot/src/boot_media/boot_media_none.c
 * Template media driver — copy to add a controller. Always absent.
 */
#include "uiox_boot.h"
#include "uiox_boot_media.h"

static int none_present(void) { return 0; }
static uiox_boot_err_t none_init(void) { return UIOX_BOOT_ERR_UNSUP; }
static uiox_boot_err_t none_read(uint32_t b, uint32_t n, void *p)
{ (void)b; (void)n; (void)p; return UIOX_BOOT_ERR_UNSUP; }

static const uiox_boot_media_ops_t none_ops = {
    .name = "none", .kind = UIOX_MEDIA_NONE,
    .present = none_present, .init = none_init, .read_block = none_read,
};

const uiox_boot_media_ops_t *uiox_boot_media_none(void) { return &none_ops; }
