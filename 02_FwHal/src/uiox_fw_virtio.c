/*
 * 02_FwHal/src/uiox_fw_virtio.c
 * VirtIO 1.x MMIO block driver. One impl for all four targets.
 */
#include "uiox_fw_virtio.h"
#include "uiox_soc_map.h"

#define VI_MAGIC          0x000u
#define VI_VERSION        0x004u
#define VI_DEVICE_ID      0x008u
#define VI_DRV_FEAT_SEL   0x024u
#define VI_DRV_FEATURES   0x020u
#define VI_QUEUE_SEL      0x030u
#define VI_QUEUE_NUM_MAX  0x034u
#define VI_QUEUE_NUM      0x038u
#define VI_QUEUE_READY    0x044u
#define VI_QUEUE_NOTIFY   0x050u
#define VI_INT_STATUS     0x060u
#define VI_INT_ACK        0x064u
#define VI_STATUS         0x070u
#define VI_Q_DESC_LO      0x080u
#define VI_Q_DESC_HI      0x084u
#define VI_Q_DRV_LO       0x090u
#define VI_Q_DRV_HI       0x094u
#define VI_Q_DEV_LO       0x0A0u
#define VI_Q_DEV_HI       0x0A4u

#define VI_MAGIC_VALUE    0x74726976u
#define VI_VERSION_MODERN 2u
#define VI_ID_BLOCK       2u
#define VI_SLOT_COUNT     8u
#define VI_ST_ACK         1u
#define VI_ST_DRIVER      2u
#define VI_ST_FEATURES_OK 8u
#define VI_ST_DRIVER_OK   4u
#define VI_BLK_T_IN       0u
#define VI_BLK_S_OK       0u
#define VI_VQ_NUM         8u
#define VI_TIMEOUT_LOOPS  5000000u

struct vi_desc { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; }
    __attribute__((packed));
struct vi_blk_hdr { uint32_t type; uint32_t reserved; uint64_t sector; }
    __attribute__((packed));
struct vi_used_elem { uint32_t id; uint32_t len; };
struct vi_used { uint16_t flags; uint16_t idx;
                 struct vi_used_elem ring[VI_VQ_NUM]; }
    __attribute__((packed));

static struct vi_desc    s_desc[VI_VQ_NUM] __attribute__((aligned(16)));
static uint16_t          s_avail[2 + VI_VQ_NUM] __attribute__((aligned(2)));
static struct vi_used    s_used __attribute__((aligned(4)));
static struct vi_blk_hdr s_hdr __attribute__((aligned(16)));
static uint8_t           s_status;
static uint16_t          s_used_seen;
static uintptr_t         s_base;
static uiox_fw_stor_dev_t s_dev;

static uintptr_t vi_scan(void)
{
    for (uint32_t s = 0u; s < VI_SLOT_COUNT; s++) {
        uintptr_t b = SOC_VIRTIO_BASE + (uintptr_t)s * SOC_VIRTIO_STRIDE;
        if (uiox_rd32(b + VI_MAGIC)   != VI_MAGIC_VALUE)    continue;
        if (uiox_rd32(b + VI_VERSION) != VI_VERSION_MODERN) continue;
        if (uiox_rd32(b + VI_DEVICE_ID) == VI_ID_BLOCK)     return b;
    }
    return 0u;
}

static uiox_fw_err_t virtio_stor_read(void *priv, uint64_t lba,
                                      uint8_t *buf, uint32_t count)
{
    (void)priv;
    if (!s_base) return UIOX_FW_ERR_NOINIT;

    s_hdr.type = VI_BLK_T_IN; s_hdr.reserved = 0u; s_hdr.sector = lba;
    s_status = 0xFFu;

    s_desc[0].addr  = (uint64_t)(uintptr_t)&s_hdr;
    s_desc[0].len   = (uint32_t)sizeof(s_hdr);
    s_desc[0].flags = 1u; s_desc[0].next = 1u;
    s_desc[1].addr  = (uint64_t)(uintptr_t)buf;
    s_desc[1].len   = count * 512u;
    s_desc[1].flags = 1u | 2u; s_desc[1].next = 2u;
    s_desc[2].addr  = (uint64_t)(uintptr_t)&s_status;
    s_desc[2].len   = 1u;
    s_desc[2].flags = 2u; s_desc[2].next = 0u;

    s_avail[1] = 0u;
    UIOX_DMB();
    s_avail[0] = 1u;
    uiox_wr32(s_base + VI_QUEUE_NOTIFY, 0u);

    for (uint32_t i = 0u; i < VI_TIMEOUT_LOOPS; i++)
        if (s_used.idx != s_used_seen) break;
    UIOX_DMB();
    s_used_seen = s_used.idx;

    uiox_wr32(s_base + VI_INT_ACK, uiox_rd32(s_base + VI_INT_STATUS));
    return (s_status == VI_BLK_S_OK) ? UIOX_FW_OK : UIOX_FW_ERR_IO;
}

uiox_fw_err_t uiox_fw_virtio_init(void)
{
    s_base = vi_scan();
    if (!s_base) return UIOX_FW_ERR_NODEV;

    uiox_wr32(s_base + VI_STATUS, 0u);
    uiox_wr32(s_base + VI_STATUS, VI_ST_ACK);
    uiox_wr32(s_base + VI_STATUS, VI_ST_ACK | VI_ST_DRIVER);
    uiox_wr32(s_base + VI_DRV_FEAT_SEL, 0u);
    uiox_wr32(s_base + VI_DRV_FEATURES, 0u);
    uiox_wr32(s_base + VI_STATUS, VI_ST_ACK | VI_ST_DRIVER | VI_ST_FEATURES_OK);
    if (!(uiox_rd32(s_base + VI_STATUS) & VI_ST_FEATURES_OK))
        return UIOX_FW_ERR_IO;

    uiox_wr32(s_base + VI_QUEUE_SEL, 0u);
    if (uiox_rd32(s_base + VI_QUEUE_NUM_MAX) < VI_VQ_NUM)
        return UIOX_FW_ERR_NOT_SUPPORTED;
    uiox_wr32(s_base + VI_QUEUE_NUM, VI_VQ_NUM);

    uint64_t d = (uint64_t)(uintptr_t)s_desc;
    uint64_t a = (uint64_t)(uintptr_t)s_avail;
    uint64_t u = (uint64_t)(uintptr_t)&s_used;
    uiox_wr32(s_base + VI_Q_DESC_LO, (uint32_t)d); uiox_wr32(s_base + VI_Q_DESC_HI, (uint32_t)(d>>32));
    uiox_wr32(s_base + VI_Q_DRV_LO,  (uint32_t)a); uiox_wr32(s_base + VI_Q_DRV_HI,  (uint32_t)(a>>32));
    uiox_wr32(s_base + VI_Q_DEV_LO,  (uint32_t)u); uiox_wr32(s_base + VI_Q_DEV_HI,  (uint32_t)(u>>32));
    uiox_wr32(s_base + VI_QUEUE_READY, 1u);

    s_dev.type        = UIOX_FW_STOR_VIRTIO_BLK;
    s_dev.sector_size = UIOX_FW_STOR_SECTOR_SIZE;
    s_dev.num_sectors = 0x200000ull;
    s_dev.read_only   = false;
    s_dev.present     = true;
    s_dev.irq         = 48u;
    s_dev.base        = s_base;
    s_dev.priv        = 0;
    s_dev.read        = virtio_stor_read;
    s_dev.write       = 0;
    s_dev.flush       = 0;
    { const char *n = "virtio-blk"; for (uint32_t i=0u; n[i] && i < UIOX_FW_STOR_NAME_LEN-1u; i++) s_dev.name[i]=n[i]; }

    return uiox_fw_stor_register(&s_dev);
}

uiox_fw_stor_dev_t *uiox_fw_virtio_stor_dev(void) { return &s_dev; }
