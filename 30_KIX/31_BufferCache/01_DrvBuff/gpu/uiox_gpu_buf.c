/**
 * @file    uiox_gpu_buf.c
 * @brief   UIOX GPU buffer pool implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_buf.h"
 #include <string.h>
 #include <assert.h>
 #include <errno.h>
 
 /* Static storage — replace with DMA-coherent heap on real targets */
 static uiox_gpu_buf_t     s_vbo[UIOX_GPU_VBO_POOL_SIZE];
 static uiox_gpu_buf_t     s_ibo[UIOX_GPU_IBO_POOL_SIZE];
 static uiox_gpu_buf_t     s_ubo[UIOX_GPU_UBO_POOL_SIZE];
 static uiox_gpu_buf_t     s_cmd[UIOX_GPU_CMD_POOL_SIZE];
 static uiox_gpu_texture_t s_tex[UIOX_GPU_TEX_POOL_SIZE];
 static uiox_gpu_fbo_t     s_fbo[UIOX_GPU_FBO_POOL_SIZE];
 
 static uint8_t s_vbo_mem[UIOX_GPU_VBO_POOL_SIZE]
                          [UIOX_GPU_VBO_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 static uint8_t s_ibo_mem[UIOX_GPU_IBO_POOL_SIZE]
                          [UIOX_GPU_IBO_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 static uint8_t s_ubo_mem[UIOX_GPU_UBO_POOL_SIZE]
                          [UIOX_GPU_UBO_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 static uint8_t s_cmd_mem[UIOX_GPU_CMD_POOL_SIZE]
                          [UIOX_GPU_CMD_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 
 /* Simple texture memory: 4 MB per texture slot */
 #define UIOX_GPU_TEX_MAX_BYTES (4u * 1024u * 1024u)
 static uint8_t s_tex_mem[UIOX_GPU_TEX_POOL_SIZE]
                          [UIOX_GPU_TEX_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 
 static uiox_gpu_buf_t     *s_vbo_free = NULL, *s_ibo_free = NULL;
 static uiox_gpu_buf_t     *s_ubo_free = NULL, *s_cmd_free = NULL;
 static uiox_gpu_texture_t *s_tex_free = NULL;
 static uiox_gpu_fbo_t     *s_fbo_free = NULL;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 static void init_buf_pool(uiox_gpu_buf_t *pool, int n,
                            uint8_t (*mem)[], uint32_t cap,
                            uiox_gpu_buf_type_t type,
                            uiox_gpu_buf_t **free_list)
 {
     *free_list = NULL;
     for (int i = 0; i < n; i++) {
         uiox_gpu_buf_t *b = &pool[i];
         memset(b, 0, sizeof(*b));
         uintptr_t base = (uintptr_t)mem[i];
         uintptr_t al   = align_up(base, UIOX_GPU_BUF_ALIGN);
         b->cpu_addr = (uint8_t *)al;
         b->paddr    = al;
         b->gpu_va   = al;  /* identity for non-MMU targets */
         b->capacity = cap;
         b->type     = type;
         b->next     = *free_list;
         *free_list  = b;
     }
 }
 
 void uiox_gpu_buf_pool_init(void)
 {
     init_buf_pool(s_vbo, UIOX_GPU_VBO_POOL_SIZE, s_vbo_mem,
                   UIOX_GPU_VBO_MAX_BYTES, UIOX_GPU_BUF_VBO, &s_vbo_free);
     init_buf_pool(s_ibo, UIOX_GPU_IBO_POOL_SIZE, s_ibo_mem,
                   UIOX_GPU_IBO_MAX_BYTES, UIOX_GPU_BUF_IBO, &s_ibo_free);
     init_buf_pool(s_ubo, UIOX_GPU_UBO_POOL_SIZE, s_ubo_mem,
                   UIOX_GPU_UBO_MAX_BYTES, UIOX_GPU_BUF_UBO, &s_ubo_free);
     init_buf_pool(s_cmd, UIOX_GPU_CMD_POOL_SIZE, s_cmd_mem,
                   UIOX_GPU_CMD_MAX_BYTES, UIOX_GPU_BUF_CMD, &s_cmd_free);
 
    /* Texture pool */
    s_tex_free = NULL;
    for (int i = 0; i < UIOX_GPU_TEX_POOL_SIZE; i++) {
        uiox_gpu_texture_t *t = &s_tex[i];
        memset(t, 0, sizeof(*t));
        uintptr_t base = (uintptr_t)s_tex_mem[i];
        uintptr_t al   = align_up(base, UIOX_GPU_BUF_ALIGN);
        t->cpu_addr = (uint8_t *)al;
        t->paddr    = al;
        t->gpu_va   = al;
        t->capacity = UIOX_GPU_TEX_MAX_BYTES;
        t->next     = s_tex_free;
        s_tex_free  = t;
    }

    /* FBO pool */
    s_fbo_free = NULL;
    for (int i = 0; i < UIOX_GPU_FBO_POOL_SIZE; i++) {
        memset(&s_fbo[i], 0, sizeof(s_fbo[i]));
        s_fbo[i].next = s_fbo_free;
        s_fbo_free    = &s_fbo[i];
    }
}

uiox_gpu_buf_t *uiox_gpu_buf_alloc(uiox_gpu_buf_type_t type)
{
    uiox_gpu_buf_t **list = NULL;
    switch (type) {
    case UIOX_GPU_BUF_VBO: list = &s_vbo_free; break;
    case UIOX_GPU_BUF_IBO: list = &s_ibo_free; break;
    case UIOX_GPU_BUF_UBO: list = &s_ubo_free; break;
    case UIOX_GPU_BUF_CMD: list = &s_cmd_free; break;
    default: return NULL;
    }
    if (!*list) return NULL;
    uiox_gpu_buf_t *b = *list;
    *list      = b->next;
    b->next    = NULL;
    b->in_use  = 1;
    b->used    = 0;
    return b;
}

void uiox_gpu_buf_free(uiox_gpu_buf_t *buf)
{
    if (!buf) return;
    assert(buf->in_use > 0);
    if (--buf->in_use == 0) {
        buf->used = 0;
        uiox_gpu_buf_t **list = NULL;
        switch (buf->type) {
        case UIOX_GPU_BUF_VBO: list = &s_vbo_free; break;
        case UIOX_GPU_BUF_IBO: list = &s_ibo_free; break;
        case UIOX_GPU_BUF_UBO: list = &s_ubo_free; break;
        case UIOX_GPU_BUF_CMD: list = &s_cmd_free; break;
        default: return;
        }
        buf->next = *list;
        *list     = buf;
    }
}

void uiox_gpu_buf_ref(uiox_gpu_buf_t *buf)
{ if (buf) buf->in_use++; }

uiox_gpu_texture_t *uiox_gpu_tex_alloc(uint16_t w, uint16_t h,
                                         uint8_t mips,
                                         uiox_gpu_fmt_t fmt)
{
    if (!s_tex_free) return NULL;
    uiox_gpu_texture_t *t = s_tex_free;
    s_tex_free = t->next;
    t->next       = NULL;
    t->in_use     = 1;
    t->width      = w;
    t->height     = h;
    t->mip_levels = mips ? mips : 1u;
    t->fmt        = fmt;
    return t;
}

void uiox_gpu_tex_free(uiox_gpu_texture_t *tex)
{
    if (!tex) return;
    assert(tex->in_use > 0);
    if (--tex->in_use == 0) {
        tex->next  = s_tex_free;
        s_tex_free = tex;
    }
}

uiox_gpu_fbo_t *uiox_gpu_fbo_alloc(uint16_t w, uint16_t h, uint8_t msaa)
{
    if (!s_fbo_free) return NULL;
    uiox_gpu_fbo_t *fbo = s_fbo_free;
    s_fbo_free      = fbo->next;
    fbo->next       = NULL;
    fbo->in_use     = 1;
    fbo->width      = w;
    fbo->height     = h;
    fbo->msaa_samples = msaa ? msaa : 1u;
    fbo->num_colour = 0;
    memset(fbo->colour, 0, sizeof(fbo->colour));
    fbo->depth   = NULL;
    fbo->stencil = NULL;
    return fbo;
}

void uiox_gpu_fbo_free(uiox_gpu_fbo_t *fbo)
{
    if (!fbo) return;
    assert(fbo->in_use > 0);
    if (--fbo->in_use == 0) {
        fbo->next  = s_fbo_free;
        s_fbo_free = fbo;
    }
}

int uiox_gpu_fbo_attach(uiox_gpu_fbo_t *fbo,
                         uiox_gpu_texture_t *tex, bool is_depth)
{
    if (!fbo || !tex) return -EINVAL;
    if (is_depth) {
        fbo->depth = tex;
    } else {
        if (fbo->num_colour >= 4) return -ENOSPC;
        fbo->colour[fbo->num_colour++] = tex;
    }
    return 0;
}
