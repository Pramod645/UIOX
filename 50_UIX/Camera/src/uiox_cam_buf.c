#include "uiox_cam_buf.h"
#include <string.h>
#include <assert.h>

/* Static pool (replace with DMA-coherent allocator on real systems) */
static uiox_cam_frame_t s_desc[UIOX_CAM_POOL_FRAMES];
static uint8_t          s_mem [UIOX_CAM_POOL_FRAMES][UIOX_CAM_FRAME_MAX + UIOX_CAM_FRAME_ALIGN];
static uiox_cam_frame_t *s_free = NULL;
static uint16_t          s_free_count = 0;
static uint16_t          s_w, s_h;
static uint32_t          s_stride;
static uint8_t           s_fmt;

static uintptr_t to_phys(void *v) { return (uintptr_t)(v); } /* stub */

void uiox_cam_buf_init(uint16_t width, uint16_t height,
                       uint32_t stride, uint8_t fmt)
{
    s_free = NULL;
    s_free_count = 0;
    s_w = width; s_h = height; s_stride = stride; s_fmt = fmt;
    for (int i = 0; i < UIOX_CAM_POOL_FRAMES; i++) {
        uiox_cam_frame_t *f = &s_desc[i];
        memset(f, 0, sizeof(*f));
        uintptr_t base = (uintptr_t)s_mem[i];
        uintptr_t aligned = (base + (UIOX_CAM_FRAME_ALIGN - 1)) & ~(uintptr_t)(UIOX_CAM_FRAME_ALIGN - 1);
        f->vaddr  = (uint8_t *)aligned;
        f->paddr  = to_phys((void *)aligned);
        f->length = UIOX_CAM_FRAME_MAX;
        f->width  = s_w;
        f->height = s_h;
        f->stride = s_stride;
        f->fmt    = s_fmt;
        f->next   = s_free;
        s_free    = f;
        s_free_count++;
    }
}

uiox_cam_frame_t *uiox_cam_buf_alloc(void)
{
    if (!s_free) return NULL;
    uiox_cam_frame_t *f = s_free;
    s_free = f->next;
    s_free_count--;
    f->next = NULL;
    f->in_use = 1;
    return f;
}

void uiox_cam_buf_ref(uiox_cam_frame_t *f)
{
    if (f) f->in_use++;
}

void uiox_cam_buf_free(uiox_cam_frame_t *f)
{
    if (!f) return;
    assert(f->in_use > 0);
    if (--f->in_use == 0) {
        f->next = s_free;
        s_free = f;
        s_free_count++;
    }
}

uint16_t uiox_cam_buf_free_count(void) { return s_free_count; }
