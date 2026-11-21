/*
 * 50_UIX/21_apps/app_gpu.c
 *
 * GPU application — EL0.  State, stats, PSO, one recorded frame.
 *
 * v2.0.0  @date 2026-11-21
 */

 #include "gpu/uiox_dev_gpu.h"
 #include "uiox_dev.h"
 #include "uix_stdio.h"
 #include "uix_string.h"
 
 int app_gpu_main(void)
 {
     uiox_dev_gpu_t         g;
     uiox_gpu_pso_desc_t    pso;
     uiox_gpu_frame_stats_t stats;
     uiox_gpu_cmd_entry_t   cmds[4];
     uiox_dev_err_t         rc;
 
     uix_printf("gpu: opening /dev/gpu0\n");
 
     rc = uiox_dev_gpu_open(&g, 0, UIOX_O_RDWR);
     if (rc != UIOX_DEV_OK) {
         uix_printf("gpu: open failed (%d) — no /dev/gpu0.\n", (int)rc);
         uix_printf("      Is the node created, and did uiox_gpu_subsys_init() run?\n");
         return 1;
     }
 
     uix_printf("gpu: state %s\n", uiox_dev_gpu_state_name(uiox_dev_gpu_state(&g)));
 
     uix_memset(&pso, 0, sizeof(pso));
     pso.topology    = UIOX_GPU_TOPO_TRIANGLES;
     pso.blend       = UIOX_GPU_BLEND_NONE;
     pso.cull        = UIOX_GPU_CULL_BACK;
     pso.depth_test  = UIX_TRUE;
     pso.depth_write = UIX_TRUE;
     pso.colour_fmt  = UIOX_GPU_FMT_RGBA8888;
     pso.depth_fmt   = UIOX_GPU_FMT_D24S8;
 
     if (uiox_dev_gpu_pso_create(&g, &pso) == UIOX_DEV_OK)
         uix_printf("gpu: PSO created\n");
 
     /* one frame: bind the PSO, then terminate.  No PASS — its arg0 is
      * an fbo pointer the kernel would have to resolve, and that pool
      * index does not exist yet. */
     uix_memset(cmds, 0, sizeof(cmds));
     cmds[0].op   = UIOX_GPU_CMD_PSO;
     cmds[0].arg0 = 0u;
     cmds[1].op   = UIOX_GPU_CMD_END;
 
     if (uiox_dev_gpu_submit(&g, cmds, 2u, 1000u) == UIOX_DEV_OK)
         uix_printf("gpu: frame submitted\n");
 
     uix_memset(&stats, 0, sizeof(stats));
     if (uiox_dev_gpu_stats(&g, &stats) == UIOX_DEV_OK)
         uix_printf("gpu: frame %u — %u draw calls, %u triangles\n",
                    (unsigned)stats.frame_id,
                    (unsigned)stats.draw_calls,
                    (unsigned)stats.triangles);
 
     uix_printf("\ngpu: no draw path yet — a mapped ring, a doorbell\n");
     uix_printf("     and a pollable fence are kernel-side work.\n");
 
     uiox_dev_gpu_close(&g);
     return 0;
 }
 
 int main(void)
 {
     return app_gpu_main();
 }
 