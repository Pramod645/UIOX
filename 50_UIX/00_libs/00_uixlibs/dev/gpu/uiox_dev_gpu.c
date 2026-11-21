/*
 * 50_UIX/00_libs/00_uixlibs/dev/gpu/uiox_dev_gpu.c
 *
 * GPU on the STREAM shape — state, stats, PSO, frame.
 *
 * ── the wire forms ──────────────────────────────────────────────────
 *   429  state   read  → one state value
 *   430  stats   write → kernel fills a uiox_gpu_frame_stats_t
 *   431  pso     write → 8-byte op + id, then the descriptor
 *   432  frame   write → the packed command array, whole
 *
 * Like HDMI and MONITOR, every call is a write or a read on the same
 * node — the router's row does the deciding.
 */
#include "uiox_dev_gpu.h"
#include "uix_string.h"          /* uix_memset */

/* ── PSO opcodes, mirrored from the same file ──────────────────────── */
#define UIOX_GPU_PSO_OP_CREATE   0u
#define UIOX_GPU_PSO_OP_DESTROY  1u

/* ── lifecycle ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_gpu_open(uiox_dev_gpu_t *g, int unit, int flags)
{
    if (!g) return UIOX_DEV_EBADNAME;

    uix_memset(g, 0, sizeof(*g));

    if (uiox_dev_stream_open_name(&g->h, "gpu0", flags) != UIOX_DEV_OK)
        return UIOX_DEV_ENODEV;

    (void)unit;
    g->last_state = UIOX_GPU_SUBSYS_STOPPED;
    g->open       = UIX_TRUE;
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_gpu_close(uiox_dev_gpu_t *g)
{
    if (!g || !g->open) return UIOX_DEV_ENODEV;
    g->open = UIX_FALSE;
    return uiox_dev_stream_close(&g->h);
}

/* ── 429 · state ───────────────────────────────────────────────────── */

uiox_gpu_subsys_state_t uiox_dev_gpu_state(const uiox_dev_gpu_t *g)
{
    if (!g || !g->open) return UIOX_GPU_SUBSYS_STOPPED;
    return g->last_state;
}

/* ── 430 · stats ───────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_gpu_stats(uiox_dev_gpu_t *g,
                                  uiox_gpu_frame_stats_t *out)
{
    if (!g || !g->open || !out) return UIOX_DEV_EBADNAME;

    if (uiox_dev_stream_read(&g->h, out, sizeof(*out)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

/* ── 431 · pso ─────────────────────────────────────────────────────── */

uiox_dev_err_t uiox_dev_gpu_pso_create(uiox_dev_gpu_t *g,
                                       const uiox_gpu_pso_desc_t *desc)
{
    uix_uint64_t hdr;
    uix_ssize_t  rc;

    if (!g || !g->open || !desc) return UIOX_DEV_EBADNAME;

    /* the header: op | id<<32 */
    hdr = (uix_uint64_t)UIOX_GPU_PSO_OP_CREATE;

    rc = uiox_dev_stream_write(&g->h, &hdr, sizeof(hdr));
    if (rc < 0) return UIOX_DEV_EIO;

    rc = uiox_dev_stream_write(&g->h, desc, sizeof(*desc));
    if (rc < 0) return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_gpu_pso_destroy(uiox_dev_gpu_t *g, uix_uint8_t pso_id)
{
    uix_uint64_t hdr = (uix_uint64_t)UIOX_GPU_PSO_OP_DESTROY
                     | ((uix_uint64_t)pso_id << 32);

    if (!g || !g->open) return UIOX_DEV_ENODEV;

    if (uiox_dev_stream_write(&g->h, &hdr, sizeof(hdr)) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

/* ── 432 · frame ───────────────────────────────────────────────────── */
uiox_dev_err_t uiox_dev_gpu_submit(uiox_dev_gpu_t *g,
                                   const uiox_gpu_cmd_entry_t *cmds,
                                   uix_uint32_t count,
                                   uix_uint32_t wait_ms)
{
    uix_uint64_t hdr;
    uix_size_t   bytes;

    if (!g || !g->open || !cmds)  return UIOX_DEV_EBADNAME;
    if (!count || count > UIOX_GPU_CMD_MAX) return UIOX_DEV_EIO;

    /* the kernel requires a terminator as the last entry */
    if (cmds[count - 1].op != UIOX_GPU_CMD_END)
        return UIOX_DEV_EIO;

    bytes = (uix_size_t)count * sizeof(uiox_gpu_cmd_entry_t);

    /* the header carries the timeout; the array follows whole */
    hdr = (uix_uint64_t)wait_ms;

    if (uiox_dev_stream_write(&g->h, &hdr, sizeof(hdr)) < 0)
        return UIOX_DEV_EIO;

    if (uiox_dev_stream_write(&g->h, cmds, bytes) < 0)
        return UIOX_DEV_EIO;

    return UIOX_DEV_OK;
}

/* ── name helper, unchanged from the kernel-side file ──────────────── */

const char *uiox_dev_gpu_state_name(uiox_gpu_subsys_state_t s)
{
    switch (s) {
    case UIOX_GPU_SUBSYS_STOPPED:   return "STOPPED";
    case UIOX_GPU_SUBSYS_IDLE:      return "IDLE";
    case UIOX_GPU_SUBSYS_RECORDING: return "RECORDING";
    case UIOX_GPU_SUBSYS_SUBMITTED: return "SUBMITTED";
    default:                         return "UNKNOWN";
    }
}
