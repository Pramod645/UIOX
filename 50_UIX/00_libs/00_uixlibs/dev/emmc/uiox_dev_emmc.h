#ifndef UIOX_DEV_EMMC_H
#define UIOX_DEV_EMMC_H

#include "../uiox_dev_block.h"     /* the BLOCK shape — takes uiox_dev_handle_t */
#include "../sys/uix_types.h"

/* Mirrored from 30_KIX/30_DSS/10_DI/03_NonSensors/emmc/uiox_emmc_ioctl.h.
 * Offsets ADD rather than OR (0xE0 + 0x01); `|` would collapse 0x02 and
 * 0x22 onto one value.  Values MUST stay identical. */
#define UIOX_DEV_EMMC_IOC_GET_IDENT    0xE1u
#define UIOX_DEV_EMMC_IOC_GET_BYTES    0xE2u
#define UIOX_DEV_EMMC_IOC_GET_BLOCKS   0xE3u
#define UIOX_DEV_EMMC_IOC_GET_PART     0xE4u
#define UIOX_DEV_EMMC_IOC_READ_EXT_CSD 0xE5u
#define UIOX_DEV_EMMC_IOC_GET_STATS    0xE6u
#define UIOX_DEV_EMMC_IOC_SET_PART     0xF0u
#define UIOX_DEV_EMMC_IOC_SET_WIDTH    0xF1u
#define UIOX_DEV_EMMC_IOC_SET_SPEED    0xF2u
#define UIOX_DEV_EMMC_IOC_SET_CLOCK    0xF3u

#define UIOX_DEV_EMMC_DEVPATH  "emmc"

/* hw.h:281-286 — every one is an ARRAY BOUND INSIDE a mirrored struct. */
#define UIOX_DEV_EMMC_EXT_CSD_LEN  512u
#define UIOX_DEV_EMMC_CID_LEN       16u
#define UIOX_DEV_EMMC_CSD_LEN       16u
#define UIOX_DEV_EMMC_BLOCK_SIZE   512u
#define UIOX_DEV_EMMC_MAX_PARTS      8u

typedef struct {
    int                id;           /* UIOX_DEV_EMMC_PART_* */
    unsigned long long size_bytes;
    unsigned char      enabled;
} uiox_dev_emmc_part_info_t;

typedef struct {
    unsigned char      cid[UIOX_DEV_EMMC_CID_LEN];
    unsigned char      csd[UIOX_DEV_EMMC_CSD_LEN];
    unsigned char      ext_csd[UIOX_DEV_EMMC_EXT_CSD_LEN];
    char               product_name[7];
    unsigned char      manufacturer_id;
    unsigned char      fwrev;
    unsigned long long capacity_sectors;
    unsigned long long capacity_bytes;
    unsigned int       cache_size_kb;
    unsigned int       erase_group_sectors;
    unsigned char      device_type;
    unsigned char      pre_eol_info;
    unsigned char      life_est_a;
    unsigned char      life_est_b;
    unsigned char      cache_supported;
    unsigned char      trim_supported;
    unsigned char      hpi_supported;
    unsigned char      bkops_supported;
    unsigned char      pon_supported;
    unsigned char      packed_cmd_supported;
    uiox_dev_emmc_part_info_t parts[UIOX_DEV_EMMC_MAX_PARTS];
} uiox_dev_emmc_ident_t;

#define UIOX_DEV_EMMC_PART_USER  0
#define UIOX_DEV_EMMC_PART_BOOT1 1
#define UIOX_DEV_EMMC_PART_BOOT2 2
#define UIOX_DEV_EMMC_PART_RPMB  3
#define UIOX_DEV_EMMC_PART_GP1   4
#define UIOX_DEV_EMMC_PART_GP2   5
#define UIOX_DEV_EMMC_PART_GP3   6
#define UIOX_DEV_EMMC_PART_GP4   7

#define UIOX_DEV_EMMC_SPEED_IDENT   0
#define UIOX_DEV_EMMC_SPEED_DS      1
#define UIOX_DEV_EMMC_SPEED_HS52    2
#define UIOX_DEV_EMMC_SPEED_HS200   3
#define UIOX_DEV_EMMC_SPEED_HS400   4
#define UIOX_DEV_EMMC_SPEED_HS400ES 5

#define UIOX_DEV_EMMC_EOL_NORMAL  0
#define UIOX_DEV_EMMC_EOL_WARNING 1
#define UIOX_DEV_EMMC_EOL_URGENT  2

/* The handle is the shape's own — a bare uiox_dev_handle_t.  There is no
 * uiox_dev_block_t: every block function takes uiox_dev_handle_t *, so
 * wrapping it in a second struct would only have to be unwrapped again
 * at each call. */
typedef struct {
    uiox_dev_handle_t h;
} uiox_dev_emmc_t;

/* open/close delegate to the shape, which owns sys_open/sys_close. */
uiox_dev_err_t uiox_dev_emmc_open (uiox_dev_emmc_t *e, int unit, int flags);
uiox_dev_err_t uiox_dev_emmc_close(uiox_dev_emmc_t *e);

/* block I/O — delegated, not re-implemented */
uix_ssize_t uiox_dev_emmc_read  (uiox_dev_emmc_t *e, void *buf, uix_size_t n);
uix_ssize_t uiox_dev_emmc_write (uiox_dev_emmc_t *e, const void *buf, uix_size_t n);
uix_ssize_t uiox_dev_emmc_pread (uiox_dev_emmc_t *e, void *buf, uix_size_t n,
                                 unsigned long long offset);
uix_ssize_t uiox_dev_emmc_pwrite(uiox_dev_emmc_t *e, const void *buf, uix_size_t n,
                                 unsigned long long offset);

uiox_dev_err_t uiox_dev_emmc_get_ident    (uiox_dev_emmc_t *e,
                                           uiox_dev_emmc_ident_t *out);
uiox_dev_err_t uiox_dev_emmc_get_bytes    (uiox_dev_emmc_t *e,
                                           unsigned long long *out_bytes);
uiox_dev_err_t uiox_dev_emmc_get_blocks   (uiox_dev_emmc_t *e,
                                           unsigned int *out_blocks);
uiox_dev_err_t uiox_dev_emmc_get_part     (uiox_dev_emmc_t *e,
                                           uiox_dev_emmc_part_info_t *out);
uiox_dev_err_t uiox_dev_emmc_read_ext_csd (uiox_dev_emmc_t *e, void *out512);
uiox_dev_err_t uiox_dev_emmc_get_stats    (uiox_dev_emmc_t *e, void *out);

uiox_dev_err_t uiox_dev_emmc_set_part (uiox_dev_emmc_t *e, int part);
uiox_dev_err_t uiox_dev_emmc_set_width(uiox_dev_emmc_t *e, unsigned int lines);
uiox_dev_err_t uiox_dev_emmc_set_speed(uiox_dev_emmc_t *e, int speed);
uiox_dev_err_t uiox_dev_emmc_set_clock(uiox_dev_emmc_t *e, unsigned int hz);

#endif /* UIOX_DEV_EMMC_H */
