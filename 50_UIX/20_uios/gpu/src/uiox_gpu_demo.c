/**
 * @file    uiox_gpu_demo.c
 * @brief   UIOX GPU stack end-to-end demonstration.
 *
 * Demonstrates:
 *   HAL init → power on → VRAM alloc → shader load → PSO create →
 *   FBO alloc → frame begin → render pass → bind state → draw calls →
 *   compute dispatch → frame end (submit + sync) → stats → teardown.
 *
 * @date    2026-06-01
 */

 #include "uiox_gpu_device.h"
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <math.h>
 #include <errno.h>
 
 /* =========================================================================
  * Stub shader binaries (normally compiled SPIR-V / GPU ISA)
  * ====================================================================== */
 
 static const uint8_t s_vert_shader[] = {
     /* Magic header identifying vertex shader binary (stub) */
     0x53,0x50,0x56,0x52, /* "SPVR" */
     0x01,0x00,0x00,0x00, /* version 1 */
     0x00,0x04,0x00,0x00, /* stage: vertex */
     /* ... (real SPIR-V / GPU ISA bytecode would follow) */
     0xDE,0xAD,0xBE,0xEF
 };
 
 static const uint8_t s_frag_shader[] = {
     0x53,0x50,0x46,0x52, /* "SPFR" */
     0x01,0x00,0x00,0x00,
     0x02,0x00,0x00,0x00, /* stage: fragment */
     0xCA,0xFE,0xBA,0xBE
 };
 
 static const uint8_t s_comp_shader[] = {
     0x53,0x50,0x43,0x4F, /* "SPCO" */
     0x01,0x00,0x00,0x00,
     0x04,0x00,0x00,0x00, /* stage: compute */
     0xBE,0xEF,0xCA,0xFE
 };
 
 /* =========================================================================
  * Sample geometry: unit quad (2 triangles, 4 vertices)
  * Vertex layout: X Y Z U V (5 floats = 20 bytes per vertex)
  * ====================================================================== */
 
 static const float s_quad_verts[] = {
     /* X      Y      Z     U     V  */
     -1.0f, -1.0f,  0.0f, 0.0f, 0.0f,
      1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
      1.0f,  1.0f,  0.0f, 1.0f, 1.0f,
     -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
 };
 
 static const uint16_t s_quad_indices[] = { 0, 1, 2,  2, 3, 0 };
 
 /* Transform uniform (mat4 = 16 floats) */
 static float s_transform[16] = {
     1,0,0,0,
     0,1,0,0,
     0,0,1,0,
     0,0,0,1,
 };
 
 /* Simple RGBA8 texture (2×2 checkerboard) */
 static const uint8_t s_checker_tex[] = {
     0xFF,0x00,0x00,0xFF,  0x00,0xFF,0x00,0xFF,
     0x00,0x00,0xFF,0xFF,  0xFF,0xFF,0x00,0xFF,
 };
 
 /* =========================================================================
  * Stub HAL ops
  * ====================================================================== */
 
 static int stub_init(uiox_gpu_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] init  Mali-G76  cores=%u  eu/core=%u\n",
            hw->num_cores, hw->num_exec_units);
     return 0;
 }
 
 static void stub_deinit  (uiox_gpu_hw_t *hw) { (void)hw; }
 static int  stub_power_on(uiox_gpu_hw_t *hw)
 {
     (void)hw;
     printf("  [hal] power ON  @ %u MHz\n", hw->cur_freq_mhz);
     hw->powered = true;
     return 0;
 }
 static void stub_power_off(uiox_gpu_hw_t *hw)
 { (void)hw; hw->powered = false; printf("  [hal] power OFF\n"); }
 
 static int stub_set_freq(uiox_gpu_hw_t *hw, uint32_t mhz)
 {
     (void)hw; (void)mhz;
     hw->cur_freq_mhz = mhz;
     printf("  [hal] freq → %u MHz\n", mhz);
     return 0;
 }
 
 static int stub_mmu_map(uiox_gpu_hw_t *hw, uint64_t gva,
                          uint64_t phys, uint64_t size, uint32_t flags)
 {
     (void)hw; (void)flags;
     printf("  [hal] MMU map  gva=0x%llX  phys=0x%llX  size=%llu KB\n",
            (unsigned long long)gva, (unsigned long long)phys,
            (unsigned long long)(size/1024));
     return 0;
 }
 
 static void stub_mmu_unmap(uiox_gpu_hw_t *hw, uint64_t gva, uint64_t size)
 {
     (void)hw; (void)gva; (void)size;
 }
 
 static uint32_t s_fence_seqno = 0;
 static int stub_cmd_submit(uiox_gpu_hw_t *hw, uintptr_t phys,
                             uint32_t size, uint32_t fence_val)
 {
     (void)hw; (void)phys;
     printf("  [hal] cmd submit  size=%u bytes  fence=%u\n",
            size, fence_val);
     s_fence_seqno = fence_val;
     hw->fence_seqno = fence_val;  /* instant completion in simulation */
     return 0;
 }
 
 static int stub_fence_wait(uiox_gpu_hw_t *hw, uint32_t fence_val,
                             uint32_t timeout_ms)
 {
     (void)timeout_ms;
     printf("  [hal] fence wait  fence=%u  (current=%u)\n",
            fence_val, hw->fence_seqno);
     return (hw->fence_seqno >= fence_val) ? 0 : -ETIMEDOUT;
 }
 
 static uint32_t s_shader_id = 1u;
 static int stub_shader_load(uiox_gpu_hw_t *hw, const uint8_t *binary,
                              uint32_t size, uint32_t stage_flags,
                              uint32_t *id_out)
 {
     (void)hw; (void)binary;
     printf("  [hal] shader load  size=%u  stages=0x%X  → id=%u\n",
            size, stage_flags, s_shader_id);
     if (id_out) *id_out = s_shader_id++;
     return 0;
 }
 
 static void stub_shader_unload(uiox_gpu_hw_t *hw, uint32_t id)
 { (void)hw; printf("  [hal] shader unload  id=%u\n", id); }
 
 static uint64_t stub_perf_read(uiox_gpu_hw_t *hw, uint8_t id)
 { (void)hw; (void)id; return 0; }
 
 static void stub_isr_job(uiox_gpu_hw_t *hw) { (void)hw; }
 static void stub_isr_mmu(uiox_gpu_hw_t *hw) { (void)hw; }
 static void stub_isr_err(uiox_gpu_hw_t *hw) { (void)hw; }
 
 static const uiox_gpu_hw_ops_t stub_ops = {
     .init          = stub_init,
     .deinit        = stub_deinit,
     .power_on      = stub_power_on,
     .power_off     = stub_power_off,
     .set_freq      = stub_set_freq,
     .mmu_map       = stub_mmu_map,
     .mmu_unmap     = stub_mmu_unmap,
     .cmd_submit    = stub_cmd_submit,
     .fence_wait    = stub_fence_wait,
     .shader_load   = stub_shader_load,
     .shader_unload = stub_shader_unload,
     .perf_read     = stub_perf_read,
     .isr_job       = stub_isr_job,
     .isr_mmu       = stub_isr_mmu,
     .isr_err       = stub_isr_err,
 };
 
 /* =========================================================================
  * Hardware device instance
  * ====================================================================== */
 
 static uiox_gpu_hw_t s_hw = {
     .base_addr       = 0xFC000000uL,
     .irq_job         = 64,
     .irq_mmu         = 65,
     .irq_err         = 66,
     .caps            = UIOX_GPU_CAP_COMPUTE     |
                        UIOX_GPU_CAP_MSAA        |
                        UIOX_GPU_CAP_ASTC        |
                        UIOX_GPU_CAP_ETC2        |
                        UIOX_GPU_CAP_TBR         |
                        UIOX_GPU_CAP_TBDR        |
                        UIOX_GPU_CAP_DMA         |
                        UIOX_GPU_CAP_MMU         |
                        UIOX_GPU_CAP_UNIFIED_MEM |
                        UIOX_GPU_CAP_PERF_COUNTERS,
     .num_cores       = 4,
     .num_exec_units  = 16,
     .tile_width      = 16,
     .tile_height     = 16,
     .vram_base       = 0x80000000ULL,
     .vram_size       = 128ULL * 1024ULL * 1024ULL, /* 128 MB */
     .max_freq_mhz    = 800,
     .cur_freq_mhz    = 600,
 };
 
 /* =========================================================================
  * main
  * ====================================================================== */
 
 int main(void)
 {
     printf("=== UIOX GPU Stack Demo ===\n\n");
 
     /* ------------------------------------------------------------------ */
     /* 1. Open device                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("--- Open ---\n");
     uiox_gpu_device_t dev;
     uiox_gpu_open_params_t p = {
         .hw            = &s_hw,
         .hw_ops        = &stub_ops,
         .vram_phys     = s_hw.vram_base,
         .vram_size     = s_hw.vram_size,
         .target_freq_mhz = 800u,
     };
 
     int rc = uiox_gpu_open(&dev, &p);
     if (rc < 0) { printf("[error] open: %d\n", rc); return 1; }
 
     printf("\n--- GPU info ---\n");
     uiox_gpu_print_info(&dev);
 
     /* ------------------------------------------------------------------ */
     /* 2. Start (power on)                                                 */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Start ---\n");
     rc = uiox_gpu_start(&dev);
     printf("  GPU: ACTIVE  rc=%d\n", rc);
 
     /* ------------------------------------------------------------------ */
     /* 3. Load shaders                                                     */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Shader loading ---\n");
     uint32_t vert_id = 0, frag_id = 0, comp_id = 0;
     uiox_gpu_load_shader(&dev, s_vert_shader, sizeof(s_vert_shader),
                           UIOX_GPU_STAGE_VERTEX, &vert_id);
     uiox_gpu_load_shader(&dev, s_frag_shader, sizeof(s_frag_shader),
                           UIOX_GPU_STAGE_FRAGMENT, &frag_id);
     uiox_gpu_load_shader(&dev, s_comp_shader, sizeof(s_comp_shader),
                           UIOX_GPU_STAGE_COMPUTE, &comp_id);
     printf("  Vertex   shader id=%u\n", vert_id);
     printf("  Fragment shader id=%u\n", frag_id);
     printf("  Compute  shader id=%u\n", comp_id);
 
     /* ------------------------------------------------------------------ */
     /* 4. Create PSOs                                                      */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Pipeline State Objects ---\n");
 
     /* Graphics PSO */
     uiox_gpu_pso_t gfx_pso = {
         .vert_shader = vert_id,
         .frag_shader = frag_id,
         .comp_shader = 0,
         .topology    = UIOX_GPU_TOPO_TRIANGLES,
         .blend       = UIOX_GPU_BLEND_ALPHA,
         .cull        = UIOX_GPU_CULL_BACK,
         .depth_test  = true,
         .depth_write = true,
         .depth_func  = 0x03u,  /* LESS */
         .colour_fmt  = UIOX_GPU_FMT_RGBA8888,
         .depth_fmt   = UIOX_GPU_FMT_D24S8,
     };
     int gfx_pso_id = uiox_gpu_create_pso(&dev, &gfx_pso);
     printf("  Graphics PSO id=%d\n", gfx_pso_id);
 
     /* Compute PSO */
     uiox_gpu_pso_t cmp_pso = {
         .vert_shader = 0,
         .frag_shader = 0,
         .comp_shader = comp_id,
     };
     int cmp_pso_id = uiox_gpu_create_pso(&dev, &cmp_pso);
     printf("  Compute  PSO id=%d\n", cmp_pso_id);
 
     /* ------------------------------------------------------------------ */
     /* 5. Allocate GPU resources                                           */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Resource allocation ---\n");
 
     uiox_gpu_buf_t *vbo = uiox_gpu_alloc_vbo(&dev,
                                                s_quad_verts,
                                                sizeof(s_quad_verts));
     printf("  VBO  size=%zu bytes  gpu_va=0x%llX\n",
            sizeof(s_quad_verts),
            vbo ? (unsigned long long)vbo->gpu_va : 0ULL);
 
     uiox_gpu_buf_t *ibo = uiox_gpu_alloc_ibo(&dev,
                                                s_quad_indices,
                                                sizeof(s_quad_indices));
     printf("  IBO  size=%zu bytes  indices=%zu\n",
            sizeof(s_quad_indices),
            sizeof(s_quad_indices)/sizeof(uint16_t));
 
     uiox_gpu_buf_t *ubo = uiox_gpu_alloc_ubo(&dev,
                                                s_transform,
                                                sizeof(s_transform));
     printf("  UBO  size=%zu bytes  (mat4 transform)\n",
            sizeof(s_transform));
 
     uiox_gpu_texture_t *tex = uiox_gpu_alloc_tex(&dev, 2u, 2u,
                                                    UIOX_GPU_FMT_RGBA8888,
                                                    s_checker_tex);
     printf("  TEX  2×2  %s\n", uiox_gpu_fmt_name(UIOX_GPU_FMT_RGBA8888));
 
     uiox_gpu_fbo_t *fbo = uiox_gpu_alloc_fbo(&dev,
                                                1920u, 1080u,
                                                UIOX_GPU_FMT_RGBA8888,
                                                true);
     printf("  FBO  1920×1080  colour=%s  depth=%s\n",
            uiox_gpu_fmt_name(UIOX_GPU_FMT_RGBA8888),
            uiox_gpu_fmt_name(UIOX_GPU_FMT_D24S8));
 
     /* ------------------------------------------------------------------ */
     /* 6. Render 3 frames                                                  */
     /* ------------------------------------------------------------------ */
 
     printf("\n--- Render loop (3 frames) ---\n");
 
     for (int frame = 0; frame < 3; frame++) {
         printf("\n  [frame %d]\n", frame + 1);
 
         /* Update transform (simple rotation stub) */
         s_transform[0] = cosf((float)frame * 0.5f);
         s_transform[1] = -sinf((float)frame * 0.5f);
         s_transform[4] = sinf((float)frame * 0.5f);
         s_transform[5] = cosf((float)frame * 0.5f);
         if (ubo) memcpy(ubo->cpu_addr, s_transform, sizeof(s_transform));
 
         /* Begin frame */
         rc = uiox_gpu_begin_frame(&dev);
         printf("    begin_frame  rc=%d\n", rc);
 
         /* Begin render pass */
         uiox_gpu_renderpass_t rp = {
             .fbo             = fbo,
             .clear_r         = 0.1f, .clear_g = 0.1f,
             .clear_b         = 0.2f, .clear_a = 1.0f,
             .clear_depth     = 1.0f,
             .clear_colour    = true,
             .clear_depth_flag= true,
         };
         rc = uiox_gpu_begin_pass(&dev, &rp);
         printf("    begin_pass   rc=%d\n", rc);
 
         /* Get command buffer and record commands */
         uiox_gpu_cmd_t *cmd = uiox_gpu_cmd(&dev);
 
         /* Bind graphics PSO */
         uiox_gpu_bind_pso(&dev, (uint8_t)gfx_pso_id);
 
         /* Set viewport + scissor */
         uiox_gpu_cmd_set_viewport(cmd, 0.f, 0.f, 1920.f, 1080.f, 0.f, 1.f);
         uiox_gpu_cmd_set_scissor (cmd, 0, 0, 1920, 1080);
 
         /* Bind buffers + texture */
         if (vbo) uiox_gpu_cmd_bind_vbo(cmd, vbo, 5u * sizeof(float), 0);
         if (ibo) uiox_gpu_cmd_bind_ibo(cmd, ibo, true);
         if (ubo) uiox_gpu_cmd_bind_ubo(cmd, ubo, 0u, 0u);
         if (tex) uiox_gpu_cmd_bind_texture(cmd, tex, 0u);
 
         /* Push constants: colour tint */
         float tint[4] = { 1.0f, 0.8f + 0.1f*(float)frame, 0.5f, 1.0f };
         uiox_gpu_cmd_push_constants(cmd, tint, sizeof(tint),
                                      UIOX_GPU_STAGE_FRAGMENT);
 
         /* Draw indexed quad */
         uiox_gpu_cmd_draw_indexed(cmd, UIOX_GPU_TOPO_TRIANGLES,
                                    6u, 0u, 0, 1u);
        printf("    draw_indexed  6 indices  (1 quad)\n");

        /* End render pass */
        uiox_gpu_end_pass(&dev);

        /* Compute dispatch (post-processing stub) */
        uiox_gpu_cmd_bind_shader(cmd, cmp_pso_id);
        uiox_gpu_cmd_dispatch(cmd, 60u, 34u, 1u);  /* 1920/32 × 1080/32 */
        printf("    dispatch  60×34×1 work groups\n");

        /* End frame — submit + wait */
        rc = uiox_gpu_end_frame(&dev, 5000u);
        printf("    end_frame  rc=%d  state=%s\n",
               rc, uiox_gpu_state_name(dev.subsys.state));

        /* Per-frame stats */
        uiox_gpu_frame_stats_t fs;
        uiox_gpu_get_frame_stats(&dev, &fs);
        printf("    frame_id=%u  draw_calls=%u  triangles=%u\n",
               fs.frame_id, fs.draw_calls, fs.triangles);
    }

    /* ------------------------------------------------------------------ */
    /* 7. Second PSO — wireframe pass                                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- Wireframe pass ---\n");
    uiox_gpu_pso_t wire_pso = {
        .vert_shader = vert_id,
        .frag_shader = frag_id,
        .topology    = UIOX_GPU_TOPO_LINES,
        .blend       = UIOX_GPU_BLEND_NONE,
        .cull        = UIOX_GPU_CULL_NONE,
        .depth_test  = false,
        .depth_write = false,
        .colour_fmt  = UIOX_GPU_FMT_RGBA8888,
        .depth_fmt   = UIOX_GPU_FMT_D16,
    };
    int wire_pso_id = uiox_gpu_create_pso(&dev, &wire_pso);
    printf("  Wireframe PSO id=%d\n", wire_pso_id);

    rc = uiox_gpu_begin_frame(&dev);
    uiox_gpu_renderpass_t wire_rp = {
        .fbo             = fbo,
        .clear_colour    = false,
        .clear_depth_flag= false,
    };
    uiox_gpu_begin_pass(&dev, &wire_rp);
    uiox_gpu_cmd_t *cmd = uiox_gpu_cmd(&dev);
    uiox_gpu_bind_pso(&dev, (uint8_t)wire_pso_id);
    if (vbo) uiox_gpu_cmd_bind_vbo(cmd, vbo, 5u * sizeof(float), 0);
    if (ibo) uiox_gpu_cmd_bind_ibo(cmd, ibo, true);
    uiox_gpu_cmd_draw_indexed(cmd, UIOX_GPU_TOPO_LINES, 6u, 0u, 0, 1u);
    uiox_gpu_end_pass(&dev);
    rc = uiox_gpu_end_frame(&dev, 5000u);
    printf("  Wireframe frame  rc=%d\n", rc);

    /* ------------------------------------------------------------------ */
    /* 8. Memory statistics                                                */
    /* ------------------------------------------------------------------ */

    printf("\n--- Memory statistics ---\n");
    uint64_t vused = 0, vfree = 0;
    uiox_gpu_get_mem_stats(&dev, &vused, &vfree);
    printf("  VRAM used  : %llu KB / %llu MB\n",
           (unsigned long long)(vused / 1024),
           (unsigned long long)(vused / (1024*1024)));
    printf("  VRAM free  : %llu MB\n",
           (unsigned long long)(vfree / (1024*1024)));

    /* ------------------------------------------------------------------ */
    /* 9. Full statistics                                                   */
    /* ------------------------------------------------------------------ */

    printf("\n--- Full statistics ---\n");
    uiox_gpu_print_stats(&dev);

    /* ------------------------------------------------------------------ */
    /* 10. PSO cleanup                                                      */
    /* ------------------------------------------------------------------ */

    printf("\n--- PSO cleanup ---\n");
    uiox_gpu_destroy_pso(&dev, (uint8_t)gfx_pso_id);
    uiox_gpu_destroy_pso(&dev, (uint8_t)cmp_pso_id);
    uiox_gpu_destroy_pso(&dev, (uint8_t)wire_pso_id);
    printf("  PSOs destroyed: %d, %d, %d\n",
           gfx_pso_id, cmp_pso_id, wire_pso_id);

    /* ------------------------------------------------------------------ */
    /* 11. Free GPU resources                                              */
    /* ------------------------------------------------------------------ */

    printf("\n--- Free GPU resources ---\n");
    if (vbo) uiox_gpu_buf_free(vbo);
    if (ibo) uiox_gpu_buf_free(ibo);
    if (ubo) uiox_gpu_buf_free(ubo);
    if (tex) uiox_gpu_tex_free(tex);
    if (fbo) {
        /* Free attached textures */
        for (uint8_t i = 0; i < fbo->num_colour; i++)
            if (fbo->colour[i]) uiox_gpu_tex_free(fbo->colour[i]);
        if (fbo->depth) uiox_gpu_tex_free(fbo->depth);
        uiox_gpu_fbo_free(fbo);
    }
    printf("  VBO, IBO, UBO, TEX, FBO freed\n");

    /* ------------------------------------------------------------------ */
    /* 12. Stop and close                                                   */
    /* ------------------------------------------------------------------ */

    printf("\n--- Stop and close ---\n");
    uiox_gpu_stop(&dev);
    printf("  GPU: POWERED OFF\n");
    uiox_gpu_close(&dev);
    printf("  Device: CLOSED\n");

    printf("\n=== UIOX GPU Demo complete ===\n");
    return 0;
}
