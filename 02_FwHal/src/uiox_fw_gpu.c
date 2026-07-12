/* Each file follows the identical 4-part pattern:
 *   1. #include header
 *   2. #define OPS_XXX macro
 *   3. static platform driver functions + ops table
 *   4. Public API wrappers + platform init helper
 *
 * All 17 remaining files: */

/* ── GPU ─── */
#include "../include/uiox_fw_gpu.h"
#define OPS_GPU(d) ((const uiox_gpu_ops_t*)(d)->priv)
static uiox_fw_err_t sfb_init(uiox_gpu_dev_t *d,uint32_t w,uint32_t h)
{ d->width=w;d->height=h;d->bpp=32u;d->initialized=true;return UIOX_FW_OK; }
static void sfb_deinit(uiox_gpu_dev_t *d){(void)d;}
static uiox_fw_err_t sfb_flush(uiox_gpu_dev_t *d,const uiox_rect_t *r)
{ (void)d;(void)r;return UIOX_FW_OK; }
static uiox_fw_err_t sfb_set_mode(uiox_gpu_dev_t *d,uint32_t w,uint32_t h)
{ d->width=w;d->height=h;return UIOX_FW_OK; }
static void *sfb_get_fb(uiox_gpu_dev_t *d)
{ return (void*)(uintptr_t)d->fb_phys; }
static const uiox_gpu_ops_t sfb_ops={sfb_init,sfb_deinit,sfb_flush,sfb_set_mode,sfb_get_fb};
uiox_fw_err_t uiox_fw_gpu_init(uiox_gpu_dev_t *d,const uiox_gpu_ops_t *o,uint32_t w,uint32_t h)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;d->priv=(void*)o;return o->init(d,w,h); }
void uiox_fw_gpu_deinit(uiox_gpu_dev_t *d){if(d&&d->priv&&OPS_GPU(d)->deinit)OPS_GPU(d)->deinit(d);}
uiox_fw_err_t uiox_fw_gpu_flush(uiox_gpu_dev_t *d,const uiox_rect_t *r)
{ if(!d||!d->priv||!OPS_GPU(d)->flush)return UIOX_FW_ERR_INVAL;return OPS_GPU(d)->flush(d,r); }
void *uiox_fw_gpu_get_fb(uiox_gpu_dev_t *d)
{ return(d&&d->priv&&OPS_GPU(d)->get_fb)?OPS_GPU(d)->get_fb(d):NULL; }
uiox_fw_err_t uiox_fw_gpu_init_virtio(uiox_gpu_dev_t *d,uintptr_t base)
{ if(!d)return UIOX_FW_ERR_INVAL;uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
  d->base=base;d->type=UIOX_GPU_VIRTIO;return uiox_fw_gpu_init(d,&sfb_ops,1920u,1080u); }
uiox_fw_err_t uiox_fw_gpu_init_simplefb(uiox_gpu_dev_t *d,uintptr_t fb_phys,
                                          uint32_t w,uint32_t h,uint32_t stride)
{ if(!d)return UIOX_FW_ERR_INVAL;uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
  d->fb_phys=fb_phys;d->type=UIOX_GPU_SIMPLE_FB;(void)stride;
  return uiox_fw_gpu_init(d,&sfb_ops,w,h); }
