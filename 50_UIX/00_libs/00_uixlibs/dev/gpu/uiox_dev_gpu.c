/*
 * 50_UIX/00_libs/00_uixlibs/dev/gpu/uiox_dev_gpu.c
 *
 * GPU on the REG shape, observation only.  All four commands are reads,
 * so uiox_dev_reg_set is unused here.
 */
#include "uiox_dev_gpu.h"

uiox_dev_err_t uiox_dev_gpu_open(uiox_dev_gpu_t *g, int unit, int flags)
{
    if (!g) return UIOX_DEV_EBADNAME;
    (void)unit;
    return uiox_dev_reg_open(&g->h, UIOX_DEV_GPU_DEVPATH, flags);
}

uiox_dev_err_t uiox_dev_gpu_close(uiox_dev_gpu_t *g)
{
    if (!g) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&g->h);
}

uiox_dev_err_t uiox_dev_gpu_get_info(uiox_dev_gpu_t *g,
                                     uiox_dev_gpu_info_t *out)
{
    if (!g || !out) return UIOX_DEV_EBADNAME;
    out->base    = 0u;
    out->irq_job = 0u;
    out->irq_mmu = 0u;
    out->irq_err = 0u;
    return uiox_dev_reg_get(&g->h, UIOX_DEV_GPU_IOC_GET_INFO, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_gpu_get_stats(uiox_dev_gpu_t *g,
                                      uiox_dev_gpu_stats_t *out)
{
    if (!g || !out) return UIOX_DEV_EBADNAME;
    out->jobs_submitted  = 0ull;
    out->jobs_completed  = 0ull;
    out->fence_waits     = 0ull;
    out->gpu_faults      = 0ull;
    out->bytes_submitted = 0ull;
    return uiox_dev_reg_get(&g->h, UIOX_DEV_GPU_IOC_GET_STATS, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_gpu_get_vram(uiox_dev_gpu_t *g,
                                     uiox_dev_gpu_vram_t *out)
{
    if (!g || !out) return UIOX_DEV_EBADNAME;
    out->base_phys  = 0ull;
    out->size       = 0ull;
    out->used       = 0ull;
    out->free_bytes = 0ull;
    return uiox_dev_reg_get(&g->h, UIOX_DEV_GPU_IOC_GET_VRAM, out)
           == 0 ? UIOX_DEV_OK : UIOX_DEV_EIO;
}

uiox_dev_err_t uiox_dev_gpu_get_window(uiox_dev_gpu_t *g,
                                       unsigned long *base_addr)
{
    unsigned long v = 0u;
    if (!g || !base_addr) return UIOX_DEV_EBADNAME;
    /* The driver writes a uintptr_t; unsigned long is the same width on
     * all four of this tree's targets (LP64). */
    if (uiox_dev_reg_get(&g->h, UIOX_DEV_GPU_IOC_GET_WINDOW, &v) != 0)
        return UIOX_DEV_EIO;
    *base_addr = v;
    return UIOX_DEV_OK;
}
