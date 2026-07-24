/**
 * @file    uiox_wifi_buf.c
 * @brief   UIOX WiFi frame buffer pool implementation.
 * @date    2026-05-28
 */

 #include "uiox_wifi_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_wifi_frame_t s_tx_desc[UIOX_WIFI_TX_POOL_SIZE];
 static uiox_wifi_frame_t s_rx_desc[UIOX_WIFI_RX_POOL_SIZE];
 
 static uint8_t s_tx_mem[UIOX_WIFI_TX_POOL_SIZE]
                         [UIOX_WIFI_HEADROOM + UIOX_WIFI_FRAME_MAX +
                          UIOX_WIFI_BUF_ALIGN];
 static uint8_t s_rx_mem[UIOX_WIFI_RX_POOL_SIZE]
                         [UIOX_WIFI_HEADROOM + UIOX_WIFI_FRAME_MAX +
                          UIOX_WIFI_BUF_ALIGN];
 
 static uiox_wifi_frame_t *s_tx_free = NULL;
 static uiox_wifi_frame_t *s_rx_free = NULL;
 static uint16_t           s_tx_cnt  = 0;
 static uint16_t           s_rx_cnt  = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 static void build_pool(uiox_wifi_frame_t *descs, int n,
    uint8_t (*mem)[], uint16_t *cnt,
    uiox_wifi_frame_t **list)
{
*list = NULL; *cnt = 0;
for (int i = 0; i < n; i++) {
uiox_wifi_frame_t *f = &descs[i];
memset(f, 0, sizeof(*f));
uintptr_t base    = (uintptr_t)mem[i];
uintptr_t aligned = align_up(base, UIOX_WIFI_BUF_ALIGN);
f->buf_start = (uint8_t *)aligned;
f->buf_end   = f->buf_start + UIOX_WIFI_HEADROOM + UIOX_WIFI_FRAME_MAX;
f->data      = f->buf_start + UIOX_WIFI_HEADROOM;
f->paddr     = aligned + UIOX_WIFI_HEADROOM;
f->len       = 0;
f->in_use    = 0;
f->next      = *list;
*list        = f;
(*cnt)++;
}
}

void uiox_wifi_buf_init(void)
{
build_pool(s_tx_desc, UIOX_WIFI_TX_POOL_SIZE,
s_tx_mem, &s_tx_cnt, &s_tx_free);
build_pool(s_rx_desc, UIOX_WIFI_RX_POOL_SIZE,
s_rx_mem, &s_rx_cnt, &s_rx_free);
}

static uiox_wifi_frame_t *pool_alloc(uiox_wifi_frame_t **list,
                  uint16_t *cnt)
{
if (!*list) return NULL;
uiox_wifi_frame_t *f = *list;
*list   = f->next;
(*cnt)--;
f->next   = NULL;
f->in_use = 1;
f->len    = 0;
f->data   = f->buf_start + UIOX_WIFI_HEADROOM;
f->paddr  = (uintptr_t)f->data;
return f;
}

uiox_wifi_frame_t *uiox_wifi_buf_alloc_tx(void)
{
uiox_wifi_frame_t *f = pool_alloc(&s_tx_free, &s_tx_cnt);
if (f) f->type = UIOX_WIFI_FRAME_DATA;
return f;
}

uiox_wifi_frame_t *uiox_wifi_buf_alloc_rx(void)
{
uiox_wifi_frame_t *f = pool_alloc(&s_rx_free, &s_rx_cnt);
if (f) f->type = UIOX_WIFI_FRAME_DATA;
return f;
}

void uiox_wifi_buf_ref(uiox_wifi_frame_t *f)
{ if (f) f->in_use++; }

void uiox_wifi_buf_free(uiox_wifi_frame_t *f)
{
if (!f) return;
assert(f->in_use > 0);
if (--f->in_use == 0) {
bool is_tx = (f >= s_tx_desc &&
  f <  s_tx_desc + UIOX_WIFI_TX_POOL_SIZE);
if (is_tx) { f->next = s_tx_free; s_tx_free = f; s_tx_cnt++; }
else       { f->next = s_rx_free; s_rx_free = f; s_rx_cnt++; }
}
}

void *uiox_wifi_buf_push(uiox_wifi_frame_t *f, uint16_t len)
{
if (!f || f->data - len < f->buf_start) return NULL;
f->data  -= len;
f->len   += len;
f->paddr  = (uintptr_t)f->data;
return f->data;
}

void *uiox_wifi_buf_pull(uiox_wifi_frame_t *f, uint16_t len)
{
if (!f || len > f->len) return NULL;
f->data  += len;
f->len   -= len;
f->paddr  = (uintptr_t)f->data;
return f->data;
}

void *uiox_wifi_buf_put(uiox_wifi_frame_t *f, uint16_t len)
{
if (!f) return NULL;
uint8_t *tail = f->data + f->len;
if (tail + len > f->buf_end) return NULL;
f->len += len;
return tail;
}

uint16_t uiox_wifi_buf_tx_free(void) { return s_tx_cnt; }
uint16_t uiox_wifi_buf_rx_free(void) { return s_rx_cnt; }
 