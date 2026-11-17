/*
 * 50_UIX/00_libs/00_uixlibs/dev/uiox_dev_path.c
 *
 * "/dev/<prefix><unit>" — the one place the node name is formatted.
 * Rules follow 30_KIX/30_DSS/src/uiox_devparse.c: a bare prefix is
 * unit 0, a unit appends a decimal, and a name naming no class fails.
 */
#include "uiox_dev.h"

uiox_dev_err_t uiox_dev_make_path(const char *prefix, int unit,
                                  char *out, unsigned long cap)
{
    unsigned long i = 0;
    const char *p = "/dev/";

    if (!prefix || !*prefix || !out || cap < 8u) return UIOX_DEV_EBADNAME;

    while (*p && i + 1u < cap) out[i++] = *p++;
    while (*prefix && i + 1u < cap) out[i++] = *prefix++;

    if (unit > 0) {
        char d[12];
        int  n = 0;
        while (unit > 0 && n < 11) { d[n++] = (char)('0' + (unit % 10)); unit /= 10; }
        while (n > 0 && i + 1u < cap) out[i++] = d[--n];
    }

    out[i] = '\0';
    return UIOX_DEV_OK;
}
