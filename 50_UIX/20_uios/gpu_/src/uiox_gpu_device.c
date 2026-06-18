/**
 * @file    uiox_gpu_device.c
 * @brief   UIOX GPU device API implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_device.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_gpu_open(uiox_gpu_device_t *dev, const uiox_gpu_open_params_t *p)
 {
     if (!dev || !p || !p->hw || !p->hw_ops) return -EINVAL;
     memset(dev, 0, sizeof(*dev));
     dev->hw = p->hw;
 
     int rc = uiox_gpu_hw_init(p->hw, p->hw_ops);
     if (rc < 0) return rc;
 
     rc = uiox_gpu_subsys_init(&dev->subsys, p->hw,
                                p->vram_phys, p->vram_size);
     if (rc < 0) return rc;
 
     dev->open = true;
     return 0;
 }
 
 int uiox_gpu_start(uiox_gpu_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_gpu_hw_power_on(dev->hw);
 }
 
 void uiox_gpu_stop(uiox_gpu_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_gpu_hw_power_off(dev->hw);
 }
 
 void uiox_gpu_close(uiox_gpu_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_gpu_stop(dev);
     uiox_gpu_subsys_deinit(&dev->subsys);
     uiox_gpu_hw_deinit(dev->hw);
     dev->open = false;
 }
 
 int uiox_gpu_create_pso(uiox_gpu_device_t *dev, const uiox_gpu_pso_t *desc)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_gpu_subsys_create_pso(&dev->subsys, desc);
 }
 
 void uiox_gpu_destroy_pso(uiox_gpu_device_t *dev, uint8_t pso_id)
 {
     if (!dev || !dev->open) return;
     uiox_gpu_subsys_destroy_pso(&dev->subsys, pso_id);
 }
 
 int uiox_gpu_begin_frame(uiox_gpu_device_t *dev)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_gpu_subsys_begin_frame(&dev->subsys);
 }
 
 int uiox_gpu_begin_pass(uiox_gpu_device_t *dev,
                          const uiox_gpu_renderpass_t *pass)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_gpu_subsys_begin_pass(&dev->subsys, pass);
 }
 
 void uiox_gpu_end_pass(uiox_gpu_device_t *dev)
 {
     if (!dev || !dev->open) return;
     uiox_gpu_subsys_end_pass(&dev->subsys);
 }
 
 int uiox_gpu_bind_pso(uiox_gpu_device_t *dev, uint8_t pso_id)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_gpu_subsys_bind_pso(&dev->subsys, pso_id);
 }
 
 int uiox_gpu_end_frame(uiox_gpu_device_t *dev, uint32_t wait_timeout_ms)
 {
     if (!dev || !dev->open) return -EINVAL;
     return uiox_gpu_subsys_end_frame(&dev->subsys, wait_timeout_ms);
 }
 
 uiox_gpu_cmd_t *uiox_gpu_cmd(uiox_gpu_device_t *dev)
 {
     return (dev && dev->open) ? &dev->subsys.cmd : NULL;
 }
 
 uiox_gpu_buf_t *uiox_gpu_alloc_vbo(uiox_gpu_device_t *dev,
                                      const void *data, uint32_t bytes)
 {
     if (!dev || !dev->open) return NULL;
     uiox_gpu_buf_t *b = uiox_gpu_buf_alloc(UIOX_GPU_BUF_VBO);
     if (!b || bytes > b->capacity) return NULL;
     if (data) memcpy(b->cpu_addr, data, bytes);
     b->used = bytes;
     return b;
 }
 
 uiox_gpu_buf_t *uiox_gpu_alloc_ibo(uiox_gpu_device_t *dev,
                                      const void *data, uint32_t bytes)
 {
     if (!dev || !dev->open) return NULL;
     uiox_gpu_buf_t *b = uiox_gpu_buf_alloc(UIOX_GPU_BUF_IBO);
     if (!b || bytes > b->capacity) return NULL;
     if (data) memcpy(b->cpu_addr, data, bytes);
     b->used = bytes;
     return b;
 }
 
 uiox_gpu_buf_t *uiox_gpu_alloc_ubo(uiox_gpu_device_t *dev,
    const void *data, uint32_t bytes)
{
if (!dev || !dev->open) return NULL;
uiox_gpu_buf_t *b = uiox_gpu_buf_alloc(UIOX_GPU_BUF_UBO);
if (!b || bytes > b->capacity) return NULL;
if (data) memcpy(b->cpu_addr, data, bytes);
b->used = bytes;
return b;
}

uiox_gpu_texture_t *uiox_gpu_alloc_tex(uiox_gpu_device_t *dev,
        uint16_t w, uint16_t h,
        uiox_gpu_fmt_t fmt,
        const void *pixels)
{
if (!dev || !dev->open) return NULL;
uiox_gpu_texture_t *t = uiox_gpu_tex_alloc(w, h, 1u, fmt);
if (!t) return NULL;
if (pixels) {
uint32_t bpp = (fmt == UIOX_GPU_FMT_RGB565 ||
fmt == UIOX_GPU_FMT_RGBA4444) ? 2u :
(fmt == UIOX_GPU_FMT_RGB888)   ? 3u :
(fmt == UIOX_GPU_FMT_RGBA16F)  ? 8u :
(fmt == UIOX_GPU_FMT_RGBA32F)  ? 16u : 4u;
uint32_t bytes = (uint32_t)w * h * bpp;
if (bytes <= t->capacity) memcpy(t->cpu_addr, pixels, bytes);
}
return t;
}

uiox_gpu_fbo_t *uiox_gpu_alloc_fbo(uiox_gpu_device_t *dev,
    uint16_t w, uint16_t h,
    uiox_gpu_fmt_t colour_fmt,
    bool with_depth)
{
if (!dev || !dev->open) return NULL;
uiox_gpu_fbo_t *fbo = uiox_gpu_fbo_alloc(w, h, 1u);
if (!fbo) return NULL;
uiox_gpu_texture_t *col = uiox_gpu_tex_alloc(w, h, 1u, colour_fmt);
if (!col) { uiox_gpu_fbo_free(fbo); return NULL; }
uiox_gpu_fbo_attach(fbo, col, false);
if (with_depth) {
uiox_gpu_texture_t *dep =
uiox_gpu_tex_alloc(w, h, 1u, UIOX_GPU_FMT_D24S8);
if (dep) uiox_gpu_fbo_attach(fbo, dep, true);
}
return fbo;
}

int uiox_gpu_load_shader(uiox_gpu_device_t *dev,
const uint8_t *binary, uint32_t size,
uint32_t stage_flags,
uint32_t *shader_id_out)
{
if (!dev || !dev->open) return -EINVAL;
return uiox_gpu_hw_shader_load(dev->hw, binary, size,
   stage_flags, shader_id_out);
}

void uiox_gpu_get_frame_stats(uiox_gpu_device_t     *dev,
uiox_gpu_frame_stats_t *out)
{
if (!dev || !dev->open || !out) return;
uiox_gpu_subsys_frame_stats(&dev->subsys, out);
}

void uiox_gpu_get_mem_stats(uiox_gpu_device_t *dev,
uint64_t *used, uint64_t *free_bytes)
{
if (!dev || !dev->open) return;
uiox_gpu_mem_stats(&dev->subsys.mem, used, free_bytes);
}

void uiox_gpu_print_info(const uiox_gpu_device_t *dev)
{
if (!dev) return;
const uiox_gpu_hw_t *hw = dev->hw;
printf("  GPU cores      : %u\n",   hw->num_cores);
printf("  Exec units     : %u\n",   hw->num_exec_units);
printf("  Tile size      : %ux%u\n",hw->tile_width, hw->tile_height);
printf("  VRAM base      : 0x%016llX\n",
(unsigned long long)hw->vram_base);
printf("  VRAM size      : %llu MB\n",
(unsigned long long)(hw->vram_size / (1024*1024)));
printf("  Max frequency  : %u MHz\n", hw->max_freq_mhz);
printf("  Capabilities   : 0x%08X\n", hw->caps);
printf("  TBR            : %s\n",
(hw->caps & UIOX_GPU_CAP_TBR)  ? "yes" : "no");
printf("  Compute        : %s\n",
(hw->caps & UIOX_GPU_CAP_COMPUTE) ? "yes" : "no");
printf("  ASTC           : %s\n",
(hw->caps & UIOX_GPU_CAP_ASTC) ? "yes" : "no");
}

void uiox_gpu_print_stats(uiox_gpu_device_t *dev)
{
if (!dev) return;
uiox_gpu_frame_stats_t fs;
uiox_gpu_subsys_frame_stats(&dev->subsys, &fs);
printf("  Frame ID       : %u\n",   fs.frame_id);
printf("  Draw calls     : %u\n",   fs.draw_calls);
printf("  Triangles      : %u\n",   fs.triangles);
printf("  Vertices       : %u\n",   fs.vertices);
printf("  Dispatches     : %u\n",   fs.dispatch_calls);

uint64_t vused = 0, vfree = 0;
uiox_gpu_get_mem_stats(dev, &vused, &vfree);
printf("  VRAM used      : %llu KB\n", (unsigned long long)(vused / 1024));
printf("  VRAM free      : %llu KB\n", (unsigned long long)(vfree / 1024));

uiox_gpu_if_stats_t is;
uiox_gpu_if_stats_get(&dev->subsys.gif, &is);
printf("  Jobs submitted : %llu\n",
(unsigned long long)is.jobs_submitted);
printf("  Jobs completed : %llu\n",
(unsigned long long)is.jobs_completed);
printf("  Bytes submitted: %llu\n",
(unsigned long long)is.bytes_submitted);
printf("  State          : %s\n",
uiox_gpu_state_name(dev->subsys.state));
}

const char *uiox_gpu_state_name(uiox_gpu_subsys_state_t s)
{
switch (s) {
case UIOX_GPU_SUBSYS_STOPPED:   return "STOPPED";
case UIOX_GPU_SUBSYS_IDLE:      return "IDLE";
case UIOX_GPU_SUBSYS_RECORDING: return "RECORDING";
case UIOX_GPU_SUBSYS_SUBMITTED: return "SUBMITTED";
default:                         return "UNKNOWN";
}
}

const char *uiox_gpu_fmt_name(uiox_gpu_fmt_t fmt)
{
switch (fmt) {
case UIOX_GPU_FMT_RGBA8888: return "RGBA8888";
case UIOX_GPU_FMT_RGB888:   return "RGB888";
case UIOX_GPU_FMT_RGB565:   return "RGB565";
case UIOX_GPU_FMT_RGBA4444: return "RGBA4444";
case UIOX_GPU_FMT_R8:       return "R8";
case UIOX_GPU_FMT_RG8:      return "RG8";
case UIOX_GPU_FMT_RGBA16F:  return "RGBA16F";
case UIOX_GPU_FMT_RGBA32F:  return "RGBA32F";
case UIOX_GPU_FMT_D16:      return "D16";
case UIOX_GPU_FMT_D24S8:    return "D24S8";
case UIOX_GPU_FMT_D32F:     return "D32F";
default:                     return "UNKNOWN";
}
}
