/**
 * @file    uiox_spk_buf.c
 * @brief   UIOX Speaker buffer pool and ring buffer implementation.
 * @date    2026-06-01
 */

 #include "uiox_spk_buf.h"
 #include <string.h>
 #include <assert.h>
 
 /* Static PCM frame pool */
 static uiox_spk_pcm_frame_t s_pcm_desc[UIOX_SPK_PCM_POOL_SIZE];
 static uint8_t               s_pcm_mem [UIOX_SPK_PCM_POOL_SIZE]
                                         [UIOX_SPK_PCM_MAX_BYTES +
                                          UIOX_SPK_BUF_ALIGN];
 
 static uiox_spk_pcm_frame_t s_mix_desc[UIOX_SPK_MIX_POOL_SIZE];
 static uint8_t               s_mix_mem [UIOX_SPK_MIX_POOL_SIZE]
                                         [UIOX_SPK_PCM_MAX_BYTES +
                                          UIOX_SPK_BUF_ALIGN];
 
 static uiox_spk_pcm_frame_t *s_pcm_free = NULL;
 static uiox_spk_pcm_frame_t *s_mix_free = NULL;
 static uint8_t               s_pcm_cnt  = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 static void build_pool(uiox_spk_pcm_frame_t *descs, int n,
                         uint8_t (*mem)[], uint32_t cap,
                         uiox_spk_pcm_frame_t **list, uint8_t *cnt)
 {
     *list = NULL;
     if (cnt) *cnt = 0;
     for (int i = 0; i < n; i++) {
         uiox_spk_pcm_frame_t *f = &descs[i];
         memset(f, 0, sizeof(*f));
         uintptr_t base = (uintptr_t)mem[i];
         uintptr_t al   = align_up(base, UIOX_SPK_BUF_ALIGN);
         f->data     = (uint8_t *)al;
         f->paddr    = al;
         f->capacity = cap;
         f->state    = UIOX_SPK_FRAME_FREE;
         f->next     = *list;
         *list       = f;
         if (cnt) (*cnt)++;
     }
 }
 
 void uiox_spk_buf_init(void)
 {
     build_pool(s_pcm_desc, UIOX_SPK_PCM_POOL_SIZE, s_pcm_mem,
                UIOX_SPK_PCM_MAX_BYTES, &s_pcm_free, &s_pcm_cnt);
     build_pool(s_mix_desc, UIOX_SPK_MIX_POOL_SIZE, s_mix_mem,
                UIOX_SPK_PCM_MAX_BYTES, &s_mix_free, NULL);
 }
 
 static uiox_spk_pcm_frame_t *pool_alloc(uiox_spk_pcm_frame_t **list,
    uint8_t *cnt)
{
if (!*list) return NULL;
uiox_spk_pcm_frame_t *f = *list;
*list      = f->next;
if (cnt && *cnt > 0) (*cnt)--;
f->next        = NULL;
f->in_use      = 1;
f->valid_bytes = 0;
f->state       = UIOX_SPK_FRAME_FILLING;
return f;
}

uiox_spk_pcm_frame_t *uiox_spk_buf_alloc_pcm(void)
{ return pool_alloc(&s_pcm_free, &s_pcm_cnt); }

uiox_spk_pcm_frame_t *uiox_spk_buf_alloc_mix(void)
{ return pool_alloc(&s_mix_free, NULL); }

void uiox_spk_buf_free(uiox_spk_pcm_frame_t *f)
{
if (!f) return;
assert(f->in_use > 0);
if (--f->in_use == 0) {
f->state = UIOX_SPK_FRAME_FREE;
/* Determine pool by pointer range */
bool is_pcm = (f >= s_pcm_desc &&
f < s_pcm_desc + UIOX_SPK_PCM_POOL_SIZE);
if (is_pcm) {
f->next   = s_pcm_free;
s_pcm_free= f;
s_pcm_cnt++;
} else {
f->next   = s_mix_free;
s_mix_free= f;
}
}
}

uint8_t uiox_spk_buf_pcm_free(void) { return s_pcm_cnt; }

/* =========================================================================
* SPSC ring buffer (stereo int16 samples)
* ====================================================================== */

void uiox_spk_ring_init(uiox_spk_ring_t *r)
{
if (!r) return;
r->head = r->tail = r->overflow = r->underrun = 0;
memset(r->buf, 0, sizeof(r->buf));
}

uint32_t uiox_spk_ring_write(uiox_spk_ring_t *r,
const int16_t *samples, uint32_t n_stereo)
{
if (!r || !samples) return 0;
uint32_t written = 0;
while (written < n_stereo) {
uint32_t next = (r->head + 1u) & UIOX_SPK_RING_MASK;
if (next == r->tail) { r->overflow++; break; }
r->buf[r->head * 2]     = samples[written * 2];
r->buf[r->head * 2 + 1] = samples[written * 2 + 1];
r->head = next;
written++;
}
return written;
}

uint32_t uiox_spk_ring_read(uiox_spk_ring_t *r,
int16_t *samples, uint32_t n_stereo)
{
if (!r || !samples) return 0;
uint32_t read = 0;
while (read < n_stereo) {
if (r->tail == r->head) { r->underrun++; break; }
samples[read * 2]     = r->buf[r->tail * 2];
samples[read * 2 + 1] = r->buf[r->tail * 2 + 1];
r->tail = (r->tail + 1u) & UIOX_SPK_RING_MASK;
read++;
}
return read;
}

uint32_t uiox_spk_ring_avail(const uiox_spk_ring_t *r)
{
if (!r) return 0;
return (r->head - r->tail) & UIOX_SPK_RING_MASK;
}

uint32_t uiox_spk_ring_space(const uiox_spk_ring_t *r)
{
if (!r) return 0;
return (UIOX_SPK_RING_SAMPLES - 1u -
((r->head - r->tail) & UIOX_SPK_RING_MASK));
}

void uiox_spk_ring_flush(uiox_spk_ring_t *r)
{ if (r) r->tail = r->head; }
