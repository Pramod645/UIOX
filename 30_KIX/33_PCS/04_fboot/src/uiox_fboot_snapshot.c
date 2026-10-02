/**
 * @file  uiox_fboot_snapshot.c
 * @brief UIOX Fast Boot — suspend-to-disk snapshot save / restore.
 * @date  2026-07-08
 *
 * ── what changed in 1.0.1 ─────────────────────────────────────────────
 *   1. uiox_fb_plat_sha256() now delegates to uiox_ks_sha256().  The
 *      former body was FNV-1a — 32 bits of non-cryptographic hash in a
 *      32-byte buffer — and that digest is the last check between a
 *      corrupt snapshot partition and the indirect jump to
 *      *(resume_fn_t *)ctx->ram_base.  A collision is trivial to
 *      construct, and the input is the entire image.
 *
 *   2. uiox_fb_snap_restore() validated nothing before reading.  It hashed
 *      image_size bytes from snap_part_base + sizeof(hdr) without proving
 *      that extent lies inside the partition, and handed raw_size to the
 *      decompressor without proving it fits RAM.  Both are now checked
 *      first, in subtraction form so a hostile header cannot wrap the test.
 *
 *   3. uiox_fb_snap_capture() guarded snap_part_size against underflow —
 *      avail = snap_part_size - sizeof(hdr) wrapped to a huge size_t if the
 *      partition was smaller than a header.
 *
 * @version 1.0.1
 * @date    2026-10-02
 */
#include "../include/uiox_fboot_snapshot.h"
#include "../include/uiox_fboot_timer.h"
#include "uiox_kix_ksign_crypto.h"   /* uiox_ks_sha256 — the snapshot hash */

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void sn_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void sn_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static int sn_memcmp(const uint8_t *a, const uint8_t *b, size_t n)
{
    uint8_t diff = 0;
    while (n--)
        diff |= (*a++ ^ *b++);
    return (int)diff;
}

/* ── Platform hooks (override in BSP) ───────────────────────────────── */

/**
 * @brief Read @len bytes from flash/disk at @offset into @buf.
 *        Production: replace with your flash / eMMC / NVMe driver.
 */
__attribute__((weak))
uiox_fb_err_t uiox_fb_plat_flash_read(uintptr_t part_base,
                                        uint64_t  offset,
                                        void     *buf,
                                        size_t    len)
{
    /* Stub: direct memory-mapped flash (XIP) */
    sn_memcpy(buf, (const void *)(part_base + (uintptr_t)offset), len);
    return UIOX_FB_OK;
}

__attribute__((weak))
uiox_fb_err_t uiox_fb_plat_flash_write(uintptr_t   part_base,
                                         uint64_t    offset,
                                         const void *buf,
                                         size_t      len)
{
    /* Stub: direct memory-mapped write (for devices without erase) */
    sn_memcpy((void *)(part_base + (uintptr_t)offset), buf, len);
    return UIOX_FB_OK;
}

/**
 * @brief Compute SHA-256 of @len bytes at @data into @digest[32].
 *
 * ── this digest guards an indirect jump ───────────────────────────────
 * uiox_fb_snap_restore() verifies this output against the snapshot
 * header, then calls *(resume_fn_t *)ctx->ram_base.  That is the last
 * check between a corrupt or hostile snapshot partition and control
 * transfer into arbitrary memory, so the hash is load-bearing.
 *
 * The previous body was FNV-1a: 4 bytes of 32-bit non-cryptographic
 * hash in a 32-byte buffer with the other 28 zeroed.  It now delegates
 * to 03_ksign.
 *
 * uiox_ks_sha256() is a STRONG symbol in
 * 03_ksign/src/uiox_kix_ksign_crypto.c, so any image linking this
 * archive must also link libksign.a — without it this becomes an
 * undefined reference at link time, not a compile error here.
 *
 * Declared weak so a BSP with hardware SHA-256 can override it.
 */
__attribute__((weak))
void uiox_fb_plat_sha256(const uint8_t *data, size_t len,
                           uint8_t digest[UIOX_FB_SNAP_HASH_LEN])
{
    uiox_ks_sha256(data, len, digest);
}

/**
 * @brief Minimal LZ4-block-style compressor stub.
 *        Production: link real LZ4 or lz4hc.
 *        Returns compressed size written into @out.
 */
__attribute__((weak))
size_t uiox_fb_plat_compress(const uint8_t *src, size_t src_len,
                               uint8_t *dst, size_t dst_cap)
{
    /* Stub: uncompressed copy */
    if (src_len > dst_cap) return 0u;
    sn_memcpy(dst, src, src_len);
    return src_len;
}

__attribute__((weak))
size_t uiox_fb_plat_decompress(const uint8_t *src, size_t src_len,
                                 uint8_t *dst, size_t dst_cap)
{
    if (src_len > dst_cap) return 0u;
    sn_memcpy(dst, src, src_len);
    return src_len;
}

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_fb_err_t uiox_fb_snap_init(uiox_fb_snap_ctx_t *ctx,
                                  uintptr_t snap_part_base,
                                  size_t    snap_part_size,
                                  uintptr_t ram_base,
                                  size_t    ram_size)
{
    if (!ctx || snap_part_size == 0u || ram_size == 0u)
        return UIOX_FB_ERR_INVAL;

    sn_memset(ctx, 0, sizeof(*ctx));
    ctx->snap_part_base = snap_part_base;
    ctx->snap_part_size = snap_part_size;
    ctx->ram_base       = ram_base;
    ctx->ram_size       = ram_size;
    ctx->initialized    = true;
    return UIOX_FB_OK;
}

/* =========================================================================
 * Probe — is there a valid, current snapshot?
 * ====================================================================== */
uiox_fb_err_t uiox_fb_snap_probe(uiox_fb_snap_ctx_t *ctx,
                                   uint32_t current_kernel_version)
{
    if (!ctx || !ctx->initialized) return UIOX_FB_ERR_INVAL;

    uiox_fb_snap_hdr_t hdr;
    uiox_fb_err_t rc = uiox_fb_plat_flash_read(ctx->snap_part_base,
                                                 0u, &hdr, sizeof(hdr));
    if (rc != UIOX_FB_OK)               return rc;
    if (hdr.magic   != UIOX_FB_SNAP_MAGIC)   return UIOX_FB_ERR_BADMAGIC;
    if (hdr.version != UIOX_FB_SNAP_VERSION) return UIOX_FB_ERR_BADVERSION;
    if (!(hdr.flags & UIOX_FB_SNAP_FLAG_VALID))   return UIOX_FB_ERR_BADMAGIC;
    if (hdr.kernel_version != current_kernel_version)
        return UIOX_FB_ERR_BADVERSION;

    sn_memcpy(&ctx->hdr, &hdr, sizeof(hdr));
    uiox_fw_printf("[fboot-snap] Valid snapshot found: "
                   "kernel_ver=%u  size=%llu B\n",
                   hdr.kernel_version,
                   (unsigned long long)hdr.image_size);
    return UIOX_FB_OK;
}

/* =========================================================================
 * Restore — decompress snapshot into RAM, then resume
 * ====================================================================== */
uiox_fb_err_t uiox_fb_snap_restore(uiox_fb_snap_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_FB_ERR_INVAL;

    const uiox_fb_snap_hdr_t *h = &ctx->hdr;

    /* ── Validate the header against BOTH regions before touching either.
     *
     * image_size drives the hash read below and the decompress call; the
     * pointer is snap_part_base + sizeof(*h), so a value past the end of
     * the partition reads whatever follows it in physical memory, and the
     * hash comparison then "passes" over those bytes.
     *
     * raw_size drives how much a real LZ4 decompressor writes.  The stub
     * checks src_len against dst_cap, but a production decompressor
     * expands according to the stored raw size — so this has to be
     * validated against ram_size BEFORE the call, not by comparing the
     * returned length afterwards.
     *
     * Both are subtractions rather than add-compare, so a hostile header
     * setting a field to its maximum cannot wrap the test. */
    if (h->image_size > ctx->snap_part_size - sizeof(*h))
        return UIOX_FB_ERR_IO;
    if (h->raw_size > ctx->ram_size)
        return UIOX_FB_ERR_IO;

    uiox_fw_printf("[fboot-snap] Restoring %llu B → RAM 0x%lx...\n",
                   (unsigned long long)h->image_size,
                   (unsigned long)ctx->ram_base);

    /* 1. Read compressed image from partition */
    uint8_t *comp_buf = (uint8_t *)(ctx->snap_part_base + sizeof(*h));

    /* 2. Verify hash before touching RAM */
    uint8_t actual_hash[UIOX_FB_SNAP_HASH_LEN];
    uiox_fb_plat_sha256(comp_buf, (size_t)h->image_size, actual_hash);
    if (sn_memcmp(actual_hash, h->hash, UIOX_FB_SNAP_HASH_LEN) != 0) {
        uiox_fw_printf("[fboot-snap] Hash mismatch — snapshot corrupt.\n");
        return UIOX_FB_ERR_IO;
    }

    /* 3. Decompress → RAM */
    size_t restored = uiox_fb_plat_decompress(
        comp_buf, (size_t)h->image_size,
        (uint8_t *)ctx->ram_base, ctx->ram_size);

    if (restored == 0u || restored != (size_t)h->raw_size) {
        uiox_fw_printf("[fboot-snap] Decompress failed "
                       "(got %zu, expected %llu).\n",
                       restored, (unsigned long long)h->raw_size);
        return UIOX_FB_ERR_IO;
    }

    uiox_fw_printf("[fboot-snap] Restore OK (%zu B). Resuming...\n",
                   restored);

    /*
     * 4. Resume — in a real implementation this performs an architecture-
     *    specific long-jump back to the saved CPU state (e.g. ARM64
     *    ldp / eret from the saved context frame in the snapshot).
     *    Here we represent it with an indirect call to the resume vector
     *    stored at a known offset in the restored RAM image.
     *
     *    This does NOT return on success.
     */
    typedef void (*resume_fn_t)(void);
    resume_fn_t resume = *(resume_fn_t *)(ctx->ram_base);
    resume();

    /* Unreachable */
    return UIOX_FB_ERR_IO;
}

/* =========================================================================
 * Capture — save RAM → snapshot partition (called on clean shutdown)
 * ====================================================================== */
uiox_fb_err_t uiox_fb_snap_capture(uiox_fb_snap_ctx_t *ctx,
                                     uint32_t kernel_version)
{
    if (!ctx || !ctx->initialized) return UIOX_FB_ERR_INVAL;

    /* A partition no larger than its own header leaves no room for a
     * payload, and avail below would wrap to a huge size_t. */
    if (ctx->snap_part_size <= sizeof(uiox_fb_snap_hdr_t))
        return UIOX_FB_ERR_INVAL;

    uiox_fw_printf("[fboot-snap] Capturing %zu B of RAM...\n",
                   ctx->ram_size);

    /* Destination: right after the header in the partition */
    uint8_t *dst = (uint8_t *)(ctx->snap_part_base + sizeof(uiox_fb_snap_hdr_t));
    size_t   avail = ctx->snap_part_size - sizeof(uiox_fb_snap_hdr_t);

    size_t comp_size = uiox_fb_plat_compress(
        (const uint8_t *)ctx->ram_base, ctx->ram_size,
        dst, avail);

    if (comp_size == 0u) {
        uiox_fw_printf("[fboot-snap] Compress failed.\n");
        return UIOX_FB_ERR_IO;
    }

    /* Build header */
    uiox_fb_snap_hdr_t hdr;
    sn_memset(&hdr, 0, sizeof(hdr));
    hdr.magic          = UIOX_FB_SNAP_MAGIC;
    hdr.version        = UIOX_FB_SNAP_VERSION;
    hdr.image_size     = (uint64_t)comp_size;
    hdr.raw_size       = (uint64_t)ctx->ram_size;
    hdr.kernel_version = kernel_version;
    hdr.flags          = UIOX_FB_SNAP_FLAG_VALID | UIOX_FB_SNAP_FLAG_COMPRESSED;
    uiox_fb_plat_sha256(dst, comp_size, hdr.hash);

    /* Write header */
    uiox_fb_err_t rc = uiox_fb_plat_flash_write(ctx->snap_part_base,
                                                   0u, &hdr, sizeof(hdr));
    if (rc != UIOX_FB_OK) return rc;

    sn_memcpy(&ctx->hdr, &hdr, sizeof(hdr));
    uiox_fw_printf("[fboot-snap] Snapshot captured: "
                   "compressed %zu → %zu B (ratio %.1f%%)\n",
                   ctx->ram_size, comp_size,
                   100.0f * (float)comp_size / (float)ctx->ram_size);
    return UIOX_FB_OK;
}

/* =========================================================================
 * Invalidate
 * ====================================================================== */
uiox_fb_err_t uiox_fb_snap_invalidate(uiox_fb_snap_ctx_t *ctx)
{
    if (!ctx || !ctx->initialized) return UIOX_FB_ERR_INVAL;

    uint32_t zero_magic = 0u;
    uiox_fb_err_t rc = uiox_fb_plat_flash_write(ctx->snap_part_base,
                                                   0u, &zero_magic,
                                                   sizeof(zero_magic));
    if (rc == UIOX_FB_OK) {
        sn_memset(&ctx->hdr, 0, sizeof(ctx->hdr));
        uiox_fw_printf("[fboot-snap] Snapshot invalidated.\n");
    }
    return rc;
}

/* =========================================================================
 * Print
 * ====================================================================== */
void uiox_fb_snap_print(const uiox_fb_snap_ctx_t *ctx)
{
    if (!ctx) return;
    const uiox_fb_snap_hdr_t *h = &ctx->hdr;
    uiox_fw_printf("[fboot-snap] Snapshot header:\n");
    uiox_fw_printf("  valid       : %s\n",
                   (h->flags & UIOX_FB_SNAP_FLAG_VALID) ? "YES" : "NO");
    uiox_fw_printf("  kernel_ver  : %u\n",  h->kernel_version);
    uiox_fw_printf("  raw_size    : %llu B\n",
                   (unsigned long long)h->raw_size);
    uiox_fw_printf("  comp_size   : %llu B\n",
                   (unsigned long long)h->image_size);
    uiox_fw_printf("  compressed  : %s\n",
                   (h->flags & UIOX_FB_SNAP_FLAG_COMPRESSED) ? "YES" : "NO");
}
