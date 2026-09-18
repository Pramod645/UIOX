/*
 * 01_uBoot/src/boot_media/boot_media_virtio.c
 * Direct VirtIO driver over the media interface (used when FwHal is
 * not linked, e.g. a minimal bootloader build). Mirrors the FwHal
 * uiox_fw_virtio.c protocol but with no storage-registry dependency.
 */
#include "uiox_boot.h"
#include "uiox_boot_media.h"
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

struct bvi_desc { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; }
    __attribute__((packed));
struct bvi_hdr  { uint32_t type; uint32_t reserved; uint64_t sector; }
    __attribute__((packed));
struct bvi_used_elem { uint32_t id; uint32_t len; };
struct bvi_used { uint16_t flags; uint16_t idx; struct bvi_used_elem ring[VI_VQ_NUM]; }
    __attribute__((packed));

static struct bvi_desc s_desc[VI_VQ_NUM] __attribute__((aligned(16)));
static uint16_t        s_avail[2 + VI_VQ_NUM] __attribute__((aligned(2)));
static struct bvi_used s_used __attribute__((aligned(4)));
static struct bvi_hdr  s_hdr __attribute__((aligned(16)));
static uint8_t         s_status;
static uint16_t        s_seen;
static uintptr_t       s_base;

static void bmb(void)
{
#if defined(__aarch64__) || defined(__arm__)
    __asm__ volatile("dmb sy" ::: "memory");
#elif defined(__x86_64__)
    __asm__ volatile("mfence" ::: "memory");
#else
    __asm__ volatile("fence rw, rw" ::: "memory");
#endif
}

static uintptr_t bvi_scan(void)
{
    for (uint32_t s = 0u; s < VI_SLOT_COUNT; s++) {
        uintptr_t b = SOC_VIRTIO_BASE + (uintptr_t)s * SOC_VIRTIO_STRIDE;
        if (mmio_read32(b + VI_MAGIC)   != VI_MAGIC_VALUE)    continue;
        if (mmio_read32(b + VI_VERSION) != VI_VERSION_MODERN) continue;
        if (mmio_read32(b + VI_DEVICE_ID) == VI_ID_BLOCK)     return b;
    }
    return 0u;
}

static int bvi_present(void) { return bvi_scan() != 0u; }

static uiox_boot_err_t bvi_init(void)
{
    s_base = bvi_scan();
    return s_base ? UIOX_BOOT_OK : UIOX_BOOT_ERR_NOTFOUND;
}

static uiox_boot_err_t bvi_read(uint32_t blk, uint32_t nblocks, void *buf)
{
    if (!s_base) return UIOX_BOOT_ERR_NOTFOUND;

    uint64_t sector = (uint64_t)blk * UIOX_MEDIA_SECTORS_PER_BLOCK;
    uint32_t nsec   = nblocks * UIOX_MEDIA_SECTORS_PER_BLOCK;

    mmio_write32(s_base + VI_STATUS, 0u);
    mmio_write32(s_base + VI_STATUS, VI_ST_ACK);
    mmio_write32(s_base + VI_STATUS, VI_ST_ACK | VI_ST_DRIVER);
    mmio_write32(s_base + VI_DRV_FEAT_SEL, 0u);
    mmio_write32(s_base + VI_DRV_FEATURES, 0u);
    mmio_write32(s_base + VI_STATUS, VI_ST_ACK | VI_ST_DRIVER | VI_ST_FEATURES_OK);
    if (!(mmio_read32(s_base + VI_STATUS) & VI_ST_FEATURES_OK))
        return UIOX_BOOT_ERR_UNSUP;

    mmio_write32(s_base + VI_QUEUE_SEL, 0u);
    if (mmio_read32(s_base + VI_QUEUE_NUM_MAX) < VI_VQ_NUM) return UIOX_BOOT_ERR_IO;
    mmio_write32(s_base + VI_QUEUE_NUM, VI_VQ_NUM);

    uintptr_t d = (uintptr_t)s_desc, a = (uintptr_t)s_avail, u = (uintptr_t)&s_used;
    mmio_write32(s_base + VI_Q_DESC_LO, (uint32_t)d); mmio_write32(s_base + VI_Q_DESC_HI, (uint32_t)(d>>32));
    mmio_write32(s_base + VI_Q_DRV_LO,  (uint32_t)a); mmio_write32(s_base + VI_Q_DRV_HI,  (uint32_t)(a>>32));
    mmio_write32(s_base + VI_Q_DEV_LO,  (uint32_t)u); mmio_write32(s_base + VI_Q_DEV_HI,  (uint32_t)(u>>32));
    mmio_write32(s_base + VI_QUEUE_READY, 1u);

    s_hdr.type = VI_BLK_T_IN; s_hdr.reserved = 0u; s_hdr.sector = sector;
    s_status = 0xFFu;

    s_desc[0].addr = (uint64_t)(uintptr_t)&s_hdr;    s_desc[0].len = sizeof(s_hdr);
    s_desc[0].flags = 1u; s_desc[0].next = 1u;
    s_desc[1].addr = (uint64_t)(uintptr_t)buf;       s_desc[1].len = nsec * 512u;
    s_desc[1].flags = 1u | 2u; s_desc[1].next = 2u;
    s_desc[2].addr = (uint64_t)(uintptr_t)&s_status; s_desc[2].len = 1u;
    s_desc[2].flags = 2u; s_desc[2].next = 0u;

    s_avail[1] = 0u;
    bmb();
    s_avail[0] = 1u;
    mmio_write32(s_base + VI_QUEUE_NOTIFY, 0u);

    for (uint32_t i = 0u; i < VI_TIMEOUT_LOOPS; i++)
        if (s_used.idx != s_seen) break;
    bmb();
    s_seen = s_used.idx;

    mmio_write32(s_base + VI_INT_ACK, mmio_read32(s_base + VI_INT_STATUS));
    mmio_write32(s_base + VI_STATUS,
                 VI_ST_ACK | VI_ST_DRIVER | VI_ST_FEATURES_OK | VI_ST_DRIVER_OK);

    return (s_status == VI_BLK_S_OK) ? UIOX_BOOT_OK : UIOX_BOOT_ERR_IO;
}

static const uiox_boot_media_ops_t bvi_ops = {
    .name = "virtio", .kind = UIOX_MEDIA_VIRTIO,
    .present = bvi_present, .init = bvi_init, .read_block = bvi_read,
};

const uiox_boot_media_ops_t *uiox_boot_media_virtio(void) { return &bvi_ops; }
