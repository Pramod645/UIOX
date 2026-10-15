/**
 * @file    uiox_gpu_buf.c
 * @brief   UIOX GPU buffer pool implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_buf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 /*
  * Freestanding-safe pointer <-> integer helpers via memcpy.
  * sizeof(uintptr_t) == sizeof(void *) on every target — these are always
  * correct and emit no -Werror=pointer-to-int-cast on arm32 or arm64.
  *
  * PTR_TO_UINTPTR : any pointer → uintptr_t  (for alignment arithmetic)
  * UINTPTR_TO_PTR : uintptr_t  → uint8_t *   (after alignment)
  *
  * paddr / gpu_va are uintptr_t in the header (confirmed from source),
  * so simple assignment after align_up is fine — no PTR_TO_PADDR needed.
  */
 #define PTR_TO_UINTPTR(dst, src)                        \
     do { const void *_q = (const void *)(src);          \
          memcpy(&(dst), &_q, sizeof(dst)); } while (0)
 
 #define UINTPTR_TO_PTR(dst, src)                        \
     do { uintptr_t _u = (src);                          \
          memcpy(&(dst), &_u, sizeof(dst)); } while (0)
 
 /* Static storage — replace with DMA-coherent heap on real targets */
 static uiox_gpu_buf_t     s_vbo[UIOX_GPU_VBO_POOL_SIZE];
 static uiox_gpu_buf_t     s_ibo[UIOX_GPU_IBO_POOL_SIZE];
 static uiox_gpu_buf_t     s_ubo[UIOX_GPU_UBO_POOL_SIZE];
 static uiox_gpu_buf_t     s_cmd[UIOX_GPU_CMD_POOL_SIZE];
 static uiox_gpu_texture_t s_tex[UIOX_GPU_TEX_POOL_SIZE];
 static uiox_gpu_fbo_t     s_fbo[UIOX_GPU_FBO_POOL_SIZE];
 
 static uint8_t s_vbo_mem[UIOX_GPU_VBO_POOL_SIZE][UIOX_GPU_VBO_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 static uint8_t s_ibo_mem[UIOX_GPU_IBO_POOL_SIZE][UIOX_GPU_IBO_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 static uint8_t s_ubo_mem[UIOX_GPU_UBO_POOL_SIZE][UIOX_GPU_UBO_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 static uint8_t s_cmd_mem[UIOX_GPU_CMD_POOL_SIZE][UIOX_GPU_CMD_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 
 #define UIOX_GPU_TEX_MAX_BYTES (4u * 1024u * 1024u)
 static uint8_t s_tex_mem[UIOX_GPU_TEX_POOL_SIZE][UIOX_GPU_TEX_MAX_BYTES + UIOX_GPU_BUF_ALIGN];
 
 static uiox_gpu_buf_t     *s_vbo_free = NULL, *s_ibo_free = NULL;
 static uiox_gpu_buf_t     *s_ubo_free = NULL, *s_cmd_free = NULL;
 static uiox_gpu_texture_t *s_tex_free = NULL;
 static uiox_gpu_fbo_t     *s_fbo_free = NULL;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 /* Shared helper — apply to every pool init block */
 static void init_buf_entry(uint8_t *mem_i, uintptr_t capacity,
                             uiox_gpu_buf_type_t type,
                             uiox_gpu_buf_t *b)
 {
     memset(b, 0, sizeof(*b));
     uintptr_t base;
     PTR_TO_UINTPTR(base, mem_i);
     uintptr_t al = align_up(base, UIOX_GPU_BUF_ALIGN);
     uint8_t *cpu;
     UINTPTR_TO_PTR(cpu, al);
     b->cpu_addr = cpu;
     b->paddr    = al;   /* uintptr_t = uintptr_t — no cast warning */
     b->gpu_va   = al;   /* uintptr_t = uintptr_t — no cast warning */
     b->capacity = capacity;
     b->type     = type;
 }
 
 static void init_vbo_pool(void)
 {
     s_vbo_free = NULL;
     for (int i = 0; i < UIOX_GPU_VBO_POOL_SIZE; i++) {
         init_buf_entry(s_vbo_mem[i], UIOX_GPU_VBO_MAX_BYTES,
                        UIOX_GPU_BUF_VBO, &s_vbo[i]);
         s_vbo[i].next = s_vbo_free; s_vbo_free = &s_vbo[i];
     }
 }
 
 static void init_ibo_pool(void)
 {
     s_ibo_free = NULL;
     for (int i = 0; i < UIOX_GPU_IBO_POOL_SIZE; i++) {
         init_buf_entry(s_ibo_mem[i], UIOX_GPU_IBO_MAX_BYTES,
                        UIOX_GPU_BUF_IBO, &s_ibo[i]);
         s_ibo[i].next = s_ibo_free; s_ibo_free = &s_ibo[i];
     }
 }
 
 static void init_ubo_pool(void)
 {
     s_ubo_free = NULL;
     for (int i = 0; i < UIOX_GPU_UBO_POOL_SIZE; i++) {
         init_buf_entry(s_ubo_mem[i], UIOX_GPU_UBO_MAX_BYTES,
                        UIOX_GPU_BUF_UBO, &s_ubo[i]);
         s_ubo[i].next = s_ubo_free; s_ubo_free = &s_ubo[i];
     }
 }
 
 static void init_cmd_pool(void)
 {
     s_cmd_free = NULL;
     for (int i = 0; i < UIOX_GPU_CMD_POOL_SIZE; i++) {
         init_buf_entry(s_cmd_mem[i], UIOX_GPU_CMD_MAX_BYTES,
                        UIOX_GPU_BUF_CMD, &s_cmd[i]);
         s_cmd[i].next = s_cmd_free; s_cmd_free = &s_cmd[i];
     }
 }
 
 void uiox_gpu_buf_pool_init(void)
 {
     init_vbo_pool();
     init_ibo_pool();
     init_ubo_pool();
     init_cmd_pool();
 
     /* Texture pool */
     s_tex_free = NULL;
     for (int i = 0; i < UIOX_GPU_TEX_POOL_SIZE; i++) {
         uiox_gpu_texture_t *t = &s_tex[i];
         memset(t, 0, sizeof(*t));
         uintptr_t base;
         PTR_TO_UINTPTR(base, s_tex_mem[i]);
         uintptr_t al = align_up(base, UIOX_GPU_BUF_ALIGN);
         uint8_t *cpu;
         UINTPTR_TO_PTR(cpu, al);
         t->cpu_addr = cpu;
         t->paddr    = al;
         t->gpu_va   = al;
         t->capacity = UIOX_GPU_TEX_MAX_BYTES;
         t->next = s_tex_free; s_tex_free = t;
     }
 
     /* FBO pool */
     s_fbo_free = NULL;
     for (int i = 0; i < UIOX_GPU_FBO_POOL_SIZE; i++) {
         memset(&s_fbo[i], 0, sizeof(s_fbo[i]));
         s_fbo[i].next = s_fbo_free; s_fbo_free = &s_fbo[i];
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
     *list = b->next; b->next = NULL; b->in_use = 1; b->used = 0;
     return b;
 }
 
 void uiox_gpu_buf_free(uiox_gpu_buf_t *buf)
 {
     if (!buf) return;
     UIOX_ASSERT(buf->in_use > 0);
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
         buf->next = *list; *list = buf;
     }
 }
 
 void uiox_gpu_buf_ref(uiox_gpu_buf_t *buf)
 { if (buf) buf->in_use++; }
 
 uiox_gpu_texture_t *uiox_gpu_tex_alloc(uint16_t w, uint16_t h,
                                          uint8_t mips, uiox_gpu_fmt_t fmt)
 {
     if (!s_tex_free) return NULL;
     uiox_gpu_texture_t *t = s_tex_free;
     s_tex_free = t->next; t->next = NULL; t->in_use = 1;
     t->width = w; t->height = h;
     t->mip_levels = mips ? mips : 1u; t->fmt = fmt;
     return t;
 }
 
 void uiox_gpu_tex_free(uiox_gpu_texture_t *tex)
 {
     if (!tex) return;
     UIOX_ASSERT(tex->in_use > 0);
     if (--tex->in_use == 0) { tex->next = s_tex_free; s_tex_free = tex; }
 }
 
 uiox_gpu_fbo_t *uiox_gpu_fbo_alloc(uint16_t w, uint16_t h, uint8_t msaa)
 {
     if (!s_fbo_free) return NULL;
     uiox_gpu_fbo_t *fbo = s_fbo_free;
     s_fbo_free = fbo->next; fbo->next = NULL; fbo->in_use = 1;
     fbo->width = w; fbo->height = h;
     fbo->msaa_samples = msaa ? msaa : 1u;
     fbo->num_colour = 0;
     memset(fbo->colour, 0, sizeof(fbo->colour));
     fbo->depth = NULL; fbo->stencil = NULL;
     return fbo;
 }
 
 void uiox_gpu_fbo_free(uiox_gpu_fbo_t *fbo)
 {
     if (!fbo) return;
     UIOX_ASSERT(fbo->in_use > 0);
     if (--fbo->in_use == 0) { fbo->next = s_fbo_free; s_fbo_free = fbo; }
 }
 
 int uiox_gpu_fbo_attach(uiox_gpu_fbo_t *fbo,
                          uiox_gpu_texture_t *tex, bool is_depth)
 {
     if (!fbo || !tex) return -EINVAL;
     if (is_depth) { fbo->depth = tex; }
     else {
         if (fbo->num_colour >= 4) return -ENOSPC;
         fbo->colour[fbo->num_colour++] = tex;
     }
     return 0;
 }
 