/*
 * 50_UIX/00_libs/00_uixlibs/dev/gpu/uiox_dev_gpu.h
 *
 * GPU — the application's view, at EL0.  OBSERVATION ONLY.
 *
 * ── FAMILY 5 (CMD) ──────────────────────────────────────────────────
 * Four commands, and every one of them is a read:
 *
 *   GET_INFO    0x01  uiox_gpu_info_t      MMIO window + three IRQ numbers
 *   GET_STATS   0x02  uiox_gpu_if_stats_t  job/fence/fault counters
 *   GET_WINDOW  0x03  uintptr_t            hw->base_addr
 *   GET_VRAM    0x04  uiox_gpu_vram_t      heap base, size, used, free
 *
 * ── THERE IS NO SUBMIT COMMAND ──────────────────────────────────────
 * The driver has uiox_gpu_if_submit(gif, cmd_buf, prio, on_complete,
 * ctx) and uiox_gpu_if_sync(gif, fence_val, timeout_ms) — but
 * on_complete is a function pointer and fence_val is produced by a
 * submission nothing at EL0 can make.  Neither crosses a trap.
 *
 * So this layer reads and cannot draw.  Drawing needs three things the
 * vocabulary does not have: a mapped ring (mmap, SYS_MMAP 49), a
 * doorbell command, and a fence the app can poll.  Adding them is
 * kernel-side work, not a wrapper.
 *
 * v1.0.0  @date 2026-11-18
 */
#ifndef UIOX_DEV_GPU_H
#define UIOX_DEV_GPU_H

#include "../uiox_dev_reg.h"

/* Mirrored from 30_KIX/30_DSS/20_DD/01_Com/gpu/uiox_gpu_if.h:85-88. */
#define UIOX_DEV_GPU_IOC_GET_INFO   0x01u
#define UIOX_DEV_GPU_IOC_GET_STATS  0x02u
#define UIOX_DEV_GPU_IOC_GET_WINDOW 0x03u
#define UIOX_DEV_GPU_IOC_GET_VRAM   0x04u

#define UIOX_DEV_GPU_DEVPATH  "gpu"

/* The kernel's uiox_gpu_info_t (uiox_gpu_if.h:90-93).  `base` is a
 * POINTER-width field — uintptr_t in the kernel — so it must be a
 * pointer-sized type here too, not a fixed 32-bit word. */
typedef struct {
    unsigned long  base;                   /* the MMIO window */
    unsigned int   irq_job;
    unsigned int   irq_mmu;
    unsigned int   irq_err;
} uiox_dev_gpu_info_t;

/* The kernel's uiox_gpu_vram_t (uiox_gpu_if.h:95-97): four uint64_t,
 * written whole by the driver's GET_VRAM body. */
typedef struct {
    unsigned long long base_phys;
    unsigned long long size;
    unsigned long long used;
    unsigned long long free_bytes;
} uiox_dev_gpu_vram_t;

/* The kernel's uiox_gpu_if_stats_t (uiox_gpu_if.h:43-49): five
 * uint64_t.  GET_STATS memcpy's the descriptor's own struct. */
typedef struct {
    unsigned long long jobs_submitted;
    unsigned long long jobs_completed;
    unsigned long long fence_waits;
    unsigned long long gpu_faults;
    unsigned long long bytes_submitted;
} uiox_dev_gpu_stats_t;

typedef struct {
    uiox_dev_handle_t h;
} uiox_dev_gpu_t;

uiox_dev_err_t uiox_dev_gpu_open (uiox_dev_gpu_t *g, int unit, int flags);
uiox_dev_err_t uiox_dev_gpu_close(uiox_dev_gpu_t *g);

uiox_dev_err_t uiox_dev_gpu_get_info (uiox_dev_gpu_t *g,
                                      uiox_dev_gpu_info_t *out);
uiox_dev_err_t uiox_dev_gpu_get_stats(uiox_dev_gpu_t *g,
                                      uiox_dev_gpu_stats_t *out);
uiox_dev_err_t uiox_dev_gpu_get_vram (uiox_dev_gpu_t *g,
                                      uiox_dev_gpu_vram_t *out);

/* GET_WINDOW returns hw->base_addr — the MMIO window, as a raw
 * address.  It is the only command that hands userspace a pointer into
 * the device, and it is the seed of any eventual submit path. */
uiox_dev_err_t uiox_dev_gpu_get_window(uiox_dev_gpu_t *g,
                                       unsigned long *base_addr);

#endif
