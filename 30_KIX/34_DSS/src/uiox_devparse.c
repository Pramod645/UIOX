/*
 *  30_KIX/34_DSS/src/uiox_devparse.c
 *
 *  Node name -> (class, unit).
 *
 *  Why this exists: Bach's mknod writes a major and a minor number into
 *  the inode for a block or character special file.  UIOX has no such
 *  fields and does not want them — uiox_devclass_t already packs the
 *  family and the class into one 16-bit value, so one class field and one
 *  unit field replace the pair, with nothing for anyone to allocate and
 *  keep unique.
 *
 *  What has to answer "which class?" then is the NAME.  A device node is
 *  created by a name, the name is what a user types, and /dev/chg0 saying
 *  CHG is the same information the major number used to carry — only
 *  legible.  mknod's own (major, minor) arguments cannot do it: two 8-bit
 *  values cannot hold a class (CHG alone is 0x3003) and a unit.
 *
 *  The parse is: a class prefix, then a decimal instance number.
 *
 *      /dev/chg0     -> UIOX_DEVCLASS_CHG, unit 0
 *      /dev/kbd2     -> UIOX_DEVCLASS_KBD, unit 2
 *      /dev/emmc     -> UIOX_DEVCLASS_EMMC, unit 0   (no suffix)
 *      /dev/ttyS0    -> SCFS_ENODEV  (no such class)
 *
 *  A name that names no class is REFUSED, not defaulted.  A node that
 *  resolves but reaches no driver is worse than no node: it opens, it
 *  reads nothing, and the failure surfaces far from the cause.
 *
 *  v1.0.0  @date 2026-10-08
 */
#include "uiox_devclass.h"

#include <stddef.h>   /* size_t for the table walk */

typedef struct {
    const char     *prefix;   /* the /dev name stem, no index            */
    uiox_devclass_t cls;
} uiox_devname_t;

/* One row per attachable class.  Ordered longest-prefix-first where one
 * prefix could be a stem of another ("usbms" and "usb"): the walk below
 * takes the first match, so order matters. */
static const uiox_devname_t s_devname[] = {
    /* family 1 — BLOCK */
    { "emmc",   UIOX_DEVCLASS_EMMC     },
    { "sdmmc",  UIOX_DEVCLASS_SDMMC    },
    { "nvme",   UIOX_DEVCLASS_NVME     },
    { "usbms",  UIOX_DEVCLASS_USBMS    },

    /* family 2 — STREAM */
    { "eth",    UIOX_DEVCLASS_ETH      },
    { "wlan",   UIOX_DEVCLASS_WIFI     },
    { "bt",     UIOX_DEVCLASS_BT       },
    { "usbhid", UIOX_DEVCLASS_USBHID   },
    { "mic",    UIOX_DEVCLASS_MIC      },
    { "spk",    UIOX_DEVCLASS_SPEAKER  },
    { "cam",    UIOX_DEVCLASS_CAMERA   },

    /* family 3 — EVENT */
    { "kbd",    UIOX_DEVCLASS_KBD      },
    { "mouse",  UIOX_DEVCLASS_MOUSE    },
    { "tp",     UIOX_DEVCLASS_TOUCHPWD },

    /* family 4 — REG */
    { "als",    UIOX_DEVCLASS_ALS      },
    { "therm",  UIOX_DEVCLASS_THERMAL  },
    { "bms",    UIOX_DEVCLASS_BMS      },
    { "chg",    UIOX_DEVCLASS_CHG      },
    { "pmic",   UIOX_DEVCLASS_PMIC     },
    { "rtc",    UIOX_DEVCLASS_RTC      },
    { "fan",    UIOX_DEVCLASS_FAN      },

    /* family 5 — CMD */
    { "gpu",    UIOX_DEVCLASS_GPU      },
    { "hdmi",   UIOX_DEVCLASS_HDMI     },
    { "mon",    UIOX_DEVCLASS_MONITOR  },

    /* family 6 — BUS */
    { "tb4",    UIOX_DEVCLASS_TB4      },
    { "usbhc",  UIOX_DEVCLASS_USBHC    },

    /* console — not attachable, but a node may name it */
    { "uart",   UIOX_DEVCLASS_UART     },
};

#define UIOX_DEVNAME_COUNT  (sizeof(s_devname) / sizeof(s_devname[0]))

/* Compare a name component against a prefix.  Returns the length consumed,
 * or 0 for no match.  A prefix matches only at the start. */
static uint32_t name_prefix_len(const char *name, uint32_t nlen,
                                const char *prefix)
{
    uint32_t i = 0u;

    while (prefix[i] != '\0') {
        if (i >= nlen || name[i] != prefix[i]) return 0u;
        i++;
    }
    return i;   /* >0 on a full match */
}

int uiox_dev_parse(const char *name, uint32_t nlen,
                   uiox_devclass_t *out_cls, uint16_t *out_unit)
{
    if (!name || !out_cls || !out_unit || nlen == 0u) return -EINVAL;

    for (size_t r = 0u; r < UIOX_DEVNAME_COUNT; r++) {
        uint32_t used = name_prefix_len(name, nlen, s_devname[r].prefix);
        if (used == 0u) continue;

        /* What follows the prefix must be the optional index — digits
         * only, then end of component.  "eth0" parses; "ether" does not,
         * because 'e' is neither a digit nor the end. */
        uint32_t unit = 0u;
        uint32_t j    = used;

        while (j < nlen && name[j] >= '0' && name[j] <= '9') {
            unit = (unit * 10u) + (uint32_t)(name[j] - '0');
            if (unit > 0x0FFFu) return -ERANGE;   /* UIOX_DEVINDEX_MASK */
            j++;
        }

        if (j != nlen) continue;   /* trailing junk — try the next row */

        *out_cls  = s_devname[r].cls;
        *out_unit = (uint16_t)unit;
        return 0;
    }

    return -ENOENT;   /* the name names no class */
}
