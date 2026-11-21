/*
 * 50_UIX/00_libs/00_uixlibs/dev/gpu/uiox_dev_gpu.h
 *
 * GPU — the application's view, at EL0.
 *
 * ── REWRITTEN: REG shape → STREAM shape ─────────────────────────────
 * v1.0.0 read four REG commands.  v2.0.0 speaks to the four CSS rows
 * the kernel now carries:
 *
 *   429  gpu_state()                subsystem state
 *   430  gpu_stats(out, len)        fills a uiox_gpu_frame_stats_t
 *   431  gpu_pso(op, id, desc)      op 0 = create, 1 = destroy
 *   432  gpu_frame(cmds, len, ms)   replays a packed command array
 *
 * ── WHAT v1.0.0 READ THAT v2.0.0 CANNOT ─────────────────────────────
 *   GET_INFO  0x01  MMIO base + three IRQ numbers  → no row
 *   GET_WINDOW 0x03 hw->base_addr                  → no row — mmap's job
 *   GET_VRAM  0x04  heap base/size/used/free       → no row
 *
 * Only GET_STATS survives, as row 430.  So this layer now READS LESS
 * than it did.  That is a real loss, not a tidy-up, and it is recorded
 * here rather than smoothed over.
 *
 * ── AND IT STILL CANNOT DRAW ────────────────────────────────────────
 * The same three things v1.0.0's banner named are still missing, and
 * row 432 does not supply them:
 *
 *   a mapped ring    mmap on /dev/gpu, SYS_MMAP 49
 *   a doorbell       a row meaning "kick the queue"
 *   a pollable fence uiox_gpu_subsys_end_frame() already RETURNS the
 *                    fence — only the row to hand it out is missing
 *
 * Row 432 replays a PACKED COMMAND ARRAY from a user buffer.  That is
 * a submission shape, not a ring: no descriptor list, no doorbell, and
 * the frame's fence never reaches the caller.
 *
 * ── NAMED ABSENT, and each for a reason ─────────────────────────────
 *   pass (GPU_CMD_PASS)  the kernel casts arg0 to a uiox_gpu_fbo_t *.
 *                        An fbo held at EL0 is a user address, so PASS
 *                        waits on the same POOL-INDEX decision HDMI
 *                        and MONITOR are parked on.
 *   alloc_vbo/ibo/ubo/tex/fbo   those buffers live in VRAM behind
 *                        /dev/gpu's MMIO window, not in EL0 memory.
 *   load_shader          uiox_gpu_hw_shader_load() is a HAL call.
 *   get_mem_stats        uiox_gpu_mem_stats() is kernel-side state.
 *   get_info/print_info  the three fields above, no longer reachable.
 *
 * v2.0.0  @date 2026-11-21
 */
#ifndef UIOX_DEV_GPU_H
#define UIOX_DEV_GPU_H

#include "../uiox_dev_stream.h"
#include "../uiox_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UIOX_DEV_GPU_DEVPATH  "gpu"

/* ── the mirrored enums ────────────────────────────────────────────────
 * Every one of these is defined in 30_KIX, which EL0 cannot include.
 * The source is named beside each; drift is the risk this header owns. */

/* uiox_gpu_subsys.h */
typedef enum {
    UIOX_GPU_SUBSYS_STOPPED = 0,
    UIOX_GPU_SUBSYS_IDLE,
    UIOX_GPU_SUBSYS_RECORDING,
    UIOX_GPU_SUBSYS_SUBMITTED,
} uiox_gpu_subsys_state_t;

/* uiox_gpu_hw.h:85-91  (30_DSS/10_DI/01_Com/gpu/) */
typedef enum {
    UIOX_GPU_TOPO_TRIANGLES = 0,
    UIOX_GPU_TOPO_TRIANGLE_STRIP,
    UIOX_GPU_TOPO_TRIANGLE_FAN,
    UIOX_GPU_TOPO_LINES,
    UIOX_GPU_TOPO_LINE_STRIP,
    UIOX_GPU_TOPO_POINTS,
} uiox_gpu_topology_t;

/* uiox_gpu_cmd.h:62-65 */
typedef enum {
    UIOX_GPU_BLEND_NONE = 0,
    UIOX_GPU_BLEND_ALPHA,
    UIOX_GPU_BLEND_ADDITIVE,
    UIOX_GPU_BLEND_PREMULT_ALPHA,
} uiox_gpu_blend_t;

/* uiox_gpu_cmd.h:71-73 */
typedef enum {
    UIOX_GPU_CULL_NONE = 0,
    UIOX_GPU_CULL_FRONT,
    UIOX_GPU_CULL_BACK,
} uiox_gpu_cull_t;

/* uiox_gpu_cmd.h — the eleven spellings taken from
 * uiox_gpu_fmt_name()'s own switch, so they cannot disagree with it. */
typedef enum {
    UIOX_GPU_FMT_RGBA8888 = 0,
    UIOX_GPU_FMT_RGB888,
    UIOX_GPU_FMT_RGB565,
    UIOX_GPU_FMT_RGBA4444,
    UIOX_GPU_FMT_R8,
    UIOX_GPU_FMT_RG8,
    UIOX_GPU_FMT_RGBA16F,
    UIOX_GPU_FMT_RGBA32F,
    UIOX_GPU_FMT_D16,
    UIOX_GPU_FMT_D24S8,
    UIOX_GPU_FMT_D32F,
} uiox_gpu_fmt_t;

/* ── the PSO descriptor the kernel copies in (row 431) ────────────────
 * Mirror of uiox_gpu_pso_t (uiox_gpu_subsys.h) minus `valid` and `id`,
 * which the kernel owns and never takes from a caller. */
typedef struct {
    uix_uint32_t         vert_shader;
    uix_uint32_t         frag_shader;
    uix_uint32_t         comp_shader;   /**< 0 = graphics pipeline */
    uiox_gpu_topology_t  topology;
    uiox_gpu_blend_t     blend;
    uiox_gpu_cull_t      cull;
    uix_bool_t           depth_test;
    uix_bool_t           depth_write;
    uix_uint8_t          depth_func;
    uiox_gpu_fmt_t       colour_fmt;
    uiox_gpu_fmt_t       depth_fmt;
} uiox_gpu_pso_desc_t;

/* ── frame statistics the kernel fills (row 430) ─────────────────────
 * uiox_gpu_subsys.h — six fields, same order, same widths. */
typedef struct {
    uix_uint32_t  draw_calls;
    uix_uint32_t  triangles;
    uix_uint32_t  vertices;
    uix_uint32_t  dispatch_calls;
    uix_uint64_t  gpu_time_ns;
    uix_uint32_t  frame_id;
} uiox_gpu_frame_stats_t;

/* ── the frame recorder ───────────────────────────────────────────────
 * The kernel replays a flat array on row 432, so the app records. */
#define UIOX_GPU_CMD_END   0u
#define UIOX_GPU_CMD_PASS  1u
#define UIOX_GPU_CMD_PSO   2u
#define UIOX_GPU_CMD_MAX   64u

/* One entry is 24 bytes, mirrored from uiox_kix_css_gpu.c.  The kernel
 * reads op | pad | arg0 | arg1 in that order, fixed width, so the
 * array length is always a multiple of 24. */
typedef struct {
    uix_uint32_t  op;
    uix_uint32_t  pad;
    uix_uint64_t  arg0;
    uix_uint64_t  arg1;
} uiox_gpu_cmd_entry_t;


/* ── what an EL0 caller chooses at open ───────────────────────────── */
typedef struct {
    uix_uint32_t  wait_ms;      /**< default sync timeout for submit */
} uiox_gpu_open_params_t;

typedef struct {
    uiox_dev_handle_t        h;
    uiox_gpu_subsys_state_t  last_state;
    uix_bool_t               open;
} uiox_dev_gpu_t;

/* ── lifecycle ─────────────────────────────────────────────────────── */
uiox_dev_err_t uiox_dev_gpu_open (uiox_dev_gpu_t *g, int unit, int flags);
uiox_dev_err_t uiox_dev_gpu_close(uiox_dev_gpu_t *g);

/* ── the verbs ─────────────────────────────────────────────────────── */
uiox_gpu_subsys_state_t uiox_dev_gpu_state(const uiox_dev_gpu_t *g);

uiox_dev_err_t uiox_dev_gpu_stats(uiox_dev_gpu_t *g,
                                  uiox_gpu_frame_stats_t *out);

uiox_dev_err_t uiox_dev_gpu_pso_create (uiox_dev_gpu_t *g,
                                        const uiox_gpu_pso_desc_t *desc);
uiox_dev_err_t uiox_dev_gpu_pso_destroy(uiox_dev_gpu_t *g, uix_uint8_t pso_id);

/* Submit one frame's worth of recorded commands. */
uiox_dev_err_t uiox_dev_gpu_submit(uiox_dev_gpu_t *g,
                                   const uiox_gpu_cmd_entry_t *cmds,
                                   uix_uint32_t count,
                                   uix_uint32_t wait_ms);

const char *uiox_dev_gpu_state_name(uiox_gpu_subsys_state_t s);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_DEV_GPU_H */
