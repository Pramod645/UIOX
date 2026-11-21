/*
 * 34_CSS/01_Com/gpu/uiox_kix_css_gpu.c
 *
 * GPU's four entry points.  Each unpacks a register slot and calls the
 * subsystem API in uiox_gpu_subsys.h.  No GPU logic here.
 *
 * ── unlike hdmi, THIS family needs a gate ───────────────────────────
 * hdmi's four take values, so uiox_kix_css_hdmi.c declares no gate.
 * GPU has two rows that carry a user ADDRESS:
 *
 *   gpu_stats  a uiox_gpu_frame_stats_t the kernel fills for the caller
 *   gpu_frame  a packed command buffer the app recorded at EL0
 *
 * uiox_kix_sccss.h says the gate follows the FAMILY, not the arm, and
 * uiox_kix_scpcs.h declares check_user_ptr / copy_to_user /
 * copy_from_user.  Those two rows use them.
 *
 * ── the instance is unwatched ───────────────────────────────────────
 * s_gpu_sys is zeroed until something calls uiox_gpu_subsys_init(), so
 * the guard turns that into ENODEV rather than faulting — the same
 * honest state HDMI and MONITOR are in.
 *
 * @version 1.0.0  @date 2026-11-21
 */

 #include "uiox_kix_css_gpu.h"
 #include "uiox_kix_scpcs.h"       /* check_user_ptr, copy_to_user, copy_from_user */
 
 #define GPU_ENODEV   ((int64_t)-19)
 #define GPU_EINVAL   ((int64_t)-22)
 #define GPU_ENOMEM   ((int64_t)-12)
 
 /* ── the PSO opcode ────────────────────────────────────────────────── */
 #define GPU_PSO_CREATE   0ull
 #define GPU_PSO_DESTROY  1ull
 
 /* ── the frame wire form ───────────────────────────────────────────────
  * One frame is four kernel calls that must land on ONE recorder:
  *
  *     begin_frame → begin_pass → bind_pso → end_frame
  *
  * The trap carries no state between calls, so the app records the whole
  * frame at EL0 into a flat array of {op, arg0, arg1} and gpu_frame
  * submits the lot.  The kernel replays it against s_gpu_sys.
  *
  *   0x00  end     — terminator, last entry only
  *   0x01  pass    — arg0 = fbo, arg1 = clear flags
  *   0x02  pso     — arg0 = pso id
  *
  * Fixed 24 bytes per entry, so the length is always a multiple of it. */
 #define GPU_CMD_END    0u
 #define GPU_CMD_PASS   1u
 #define GPU_CMD_PSO    2u
 #define GPU_CMD_SIZE   24u
 #define GPU_CMD_MAX    64u
 
 typedef struct {
     uiox_uint32_t op;
     uiox_uint32_t pad;
     uiox_uint64_t arg0;
     uiox_uint64_t arg1;
 } uiox_kix_gpu_cmd_t;
 
 /* ── the singleton ───────────────────────────────────────────────────
  * Filled by the probe's uiox_gpu_subsys_init() call — which is what
  * uix_sys.h:935-938 currently has commented out. */
 uiox_gpu_subsys_t *uiox_gpu_subsys_current(void)
 {
     extern uiox_gpu_subsys_t s_gpu_sys;
     return &s_gpu_sys;
 }
 
 /* ── 429 · state ───────────────────────────────────────────────────── */
 int64_t uiox_kix_css_gpu_state(void)
 {
     uiox_gpu_subsys_t *sys = uiox_gpu_subsys_current();
 
     if (!sys) return GPU_ENODEV;
 
     return (int64_t)sys->state;   /* UIOX_GPU_SUBSYS_* */
 }
 
 /* ── 430 · stats ───────────────────────────────────────────────────── */
 int64_t uiox_kix_css_gpu_stats(uiox_uint64_t out_addr, uiox_uint64_t out_len)
 {
     uiox_gpu_subsys_t *sys = uiox_gpu_subsys_current();
     uiox_gpu_frame_stats_t snap;
 
     if (!sys)                  return GPU_ENODEV;
     if (out_len < sizeof(snap)) return GPU_EINVAL;
 
     if (check_user_ptr((void *)out_addr, sizeof(snap)) < 0)
         return GPU_EINVAL;
 
     uiox_gpu_subsys_frame_stats(sys, &snap);
 
     if (copy_to_user((void *)out_addr, &snap, sizeof(snap)) < 0)
         return GPU_EINVAL;
 
     return 0;
 }
 
 /* ── 431 · pso ─────────────────────────────────────────────────────── */
 int64_t uiox_kix_css_gpu_pso(uiox_uint64_t op,
                              uiox_uint64_t pso_id,
                              uiox_uint64_t desc_addr)
 {
     uiox_gpu_subsys_t *sys = uiox_gpu_subsys_current();
     uiox_gpu_pso_t     desc;
 
     if (!sys) return GPU_ENODEV;
 
     if (op == GPU_PSO_CREATE) {
         if (check_user_ptr((void *)desc_addr, sizeof(desc)) < 0)
             return GPU_EINVAL;
 
         if (copy_from_user(&desc, (void *)desc_addr, sizeof(desc)) < 0)
             return GPU_EINVAL;
 
         /* the kernel owns id and valid — never the caller */
         desc.valid = false;
         desc.id    = 0;
 
         return (int64_t)uiox_gpu_subsys_create_pso(sys, &desc);
     }
 
     if (op == GPU_PSO_DESTROY) {
         if (pso_id >= sys->pso_count) return GPU_EINVAL;
         uiox_gpu_subsys_destroy_pso(sys, (uiox_uint8_t)pso_id);
         return 0;
     }
 
     return GPU_EINVAL;
 }
 
 /* ── 432 · frame ───────────────────────────────────────────────────── */
 int64_t uiox_kix_css_gpu_frame(uiox_uint64_t cmdbuf_addr,
                                uiox_uint64_t cmdbuf_len,
                                uiox_uint64_t wait_ms)
 {
     uiox_gpu_subsys_t *sys = uiox_gpu_subsys_current();
     uiox_gpu_renderpass_t pass;
     uiox_kix_gpu_cmd_t    cmds[GPU_CMD_MAX];
     uiox_uint32_t         n, i;
     int64_t               rc;
 
     if (!sys)                     return GPU_ENODEV;
     if (!cmdbuf_len || !cmdbuf_addr) return GPU_EINVAL;
     if (cmdbuf_len > sizeof(cmds))   return GPU_EINVAL;
     if (cmdbuf_len % GPU_CMD_SIZE)   return GPU_EINVAL;
     if (wait_ms > (uiox_uint64_t)0xFFFFFFFFull) return GPU_EINVAL;
 
     if (check_user_ptr((void *)cmdbuf_addr, cmdbuf_len) < 0)
         return GPU_EINVAL;
 
     if (copy_from_user(cmds, (void *)cmdbuf_addr, cmdbuf_len) < 0)
         return GPU_EINVAL;
 
     n = (uiox_uint32_t)(cmdbuf_len / GPU_CMD_SIZE);
     if (cmds[n - 1].op != GPU_CMD_END) return GPU_EINVAL;
 
     rc = uiox_gpu_subsys_begin_frame(sys);
     if (rc < 0) return rc;
 
     for (i = 0; i < n; i++) {
         switch (cmds[i].op) {
         case GPU_CMD_END:
             goto submit;
 
         case GPU_CMD_PASS:
             /* v1.0.0 carries the clear flags in arg1 and no colour —
              * a pass with a colour needs the full descriptor, which is
              * a later row. */
             pass.fbo              = (uiox_gpu_fbo_t *)cmds[i].arg0;
             pass.clear_colour     = false;
             pass.clear_depth_flag = (cmds[i].arg1 & 1ull) != 0;
             pass.clear_r = pass.clear_g = pass.clear_b = pass.clear_a = 0.0f;
             pass.clear_depth = 1.0f;
 
             rc = uiox_gpu_subsys_begin_pass(sys, &pass);
             if (rc < 0) return rc;
             break;
 
         case GPU_CMD_PSO:
             rc = uiox_gpu_subsys_bind_pso(sys, (uiox_uint8_t)cmds[i].arg0);
             if (rc < 0) return rc;
             break;
 
         default:
             return GPU_EINVAL;
         }
     }
 
 submit:
     uiox_gpu_subsys_end_pass(sys);
     return (int64_t)uiox_gpu_subsys_end_frame(sys, (uiox_uint32_t)wait_ms);
 }
 