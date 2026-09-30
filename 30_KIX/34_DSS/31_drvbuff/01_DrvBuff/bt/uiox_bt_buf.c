/**
 * @file    uiox_bt_buf.c
 * @brief   UIOX Bluetooth buffer pool implementation.
 * @date    2026-06-09
 */

 #include "uiox_bt_buf.h"

 /* Freestanding assert — no libc assert.h available under -nostdinc */
 #ifndef UIOX_ASSERT
 #  define UIOX_ASSERT(cond)  do { if (!(cond)) __builtin_trap(); } while (0)
 #endif
 
 static uiox_bt_pkt_t s_cmd_pool[UIOX_BT_CMD_POOL_SIZE];
 static uiox_bt_pkt_t s_acl_pool[UIOX_BT_ACL_POOL_SIZE];
 static uiox_bt_pkt_t *s_cmd_free = NULL;
 static uiox_bt_pkt_t *s_acl_free = NULL;
 
 void uiox_bt_buf_init(void)
 {
     s_cmd_free = NULL;
     for (int i = 0; i < UIOX_BT_CMD_POOL_SIZE; i++) {
         memset(&s_cmd_pool[i], 0, sizeof(s_cmd_pool[i]));
         s_cmd_pool[i].next = s_cmd_free;
         s_cmd_free = &s_cmd_pool[i];
     }
     s_acl_free = NULL;
     for (int i = 0; i < UIOX_BT_ACL_POOL_SIZE; i++) {
         memset(&s_acl_pool[i], 0, sizeof(s_acl_pool[i]));
         s_acl_pool[i].next = s_acl_free;
         s_acl_free = &s_acl_pool[i];
     }
 }
 
 static uiox_bt_pkt_t *pool_alloc(uiox_bt_pkt_t **list)
 {
     if (!*list) return NULL;
     uiox_bt_pkt_t *p = *list;
     *list = p->next; p->next = NULL; p->in_use = 1; p->len = 0;
     return p;
 }
 
 uiox_bt_pkt_t *uiox_bt_cmd_alloc(void) { return pool_alloc(&s_cmd_free); }
 uiox_bt_pkt_t *uiox_bt_acl_alloc(void) { return pool_alloc(&s_acl_free); }
 
 void uiox_bt_pkt_free(uiox_bt_pkt_t *p)
 {
     if (!p) return;
     UIOX_ASSERT(p->in_use > 0);
     if (--p->in_use == 0) {
         bool is_cmd = (p >= s_cmd_pool && p < s_cmd_pool + UIOX_BT_CMD_POOL_SIZE);
         if (is_cmd) { p->next = s_cmd_free; s_cmd_free = p; }
         else        { p->next = s_acl_free; s_acl_free = p; }
     }
 }
 
 void uiox_bt_evt_ring_init(uiox_bt_evt_ring_t *r)
 {
     if (!r) return;
     r->head = r->tail = r->overflow = 0;
     memset(r->buf, 0, sizeof(r->buf));
 }
 
 bool uiox_bt_evt_push(uiox_bt_evt_ring_t *r,
                        const uint8_t *data, uint16_t len, uint8_t pkt_type)
 {
     if (!r || !data) return false;
     uint32_t next = (r->head + 1u) & UIOX_BT_EVT_RING_MASK;
     if (next == r->tail) { r->overflow++; return false; }
     uiox_bt_pkt_t *slot = &r->buf[r->head];
     uint16_t copy = len < UIOX_BT_PKT_MAX_LEN ? len : UIOX_BT_PKT_MAX_LEN;
     memcpy(slot->data, data, copy);
     slot->len = copy; slot->pkt_type = pkt_type; slot->in_use = 1;
     r->head = next;
     return true;
 }
 
 bool uiox_bt_evt_pop(uiox_bt_evt_ring_t *r, uiox_bt_pkt_t *out)
 {
     if (!r || !out || r->head == r->tail) return false;
     *out = r->buf[r->tail];
     r->tail = (r->tail + 1u) & UIOX_BT_EVT_RING_MASK;
     return true;
 }
 
 bool uiox_bt_evt_empty(const uiox_bt_evt_ring_t *r)
 { return r ? (r->head == r->tail) : true; }
 