/**
 * @file    uiox_usb_buf.c
 * @brief   UIOX USB URB buffer pool implementation.
 * @date    2026-05-28
 */

 #include "uiox_usb_buf.h"
 #include <string.h>
 #include <assert.h>
 
 static uiox_usb_urb_t s_urb_pool[UIOX_USB_URB_POOL_SIZE];
 static uint8_t        s_data_pool[UIOX_USB_URB_POOL_SIZE]
                                   [UIOX_USB_URB_DATA_MAX + UIOX_USB_URB_ALIGN];
 
 static uiox_usb_urb_t *s_free  = NULL;
 static uint16_t        s_free_cnt = 0;
 
 static uintptr_t align_up(uintptr_t p, uintptr_t a)
 { return (p + a - 1u) & ~(a - 1u); }
 
 void uiox_usb_buf_init(void)
 {
     s_free = NULL; s_free_cnt = 0;
     for (int i = 0; i < UIOX_USB_URB_POOL_SIZE; i++) {
         uiox_usb_urb_t *u = &s_urb_pool[i];
         memset(u, 0, sizeof(*u));
         uintptr_t base = (uintptr_t)s_data_pool[i];
         uintptr_t al   = align_up(base, UIOX_USB_URB_ALIGN);
         u->buf     = (uint8_t *)al;
         u->paddr   = al;
         u->buf_len = UIOX_USB_URB_DATA_MAX;
         u->in_use  = 0;
         u->next    = s_free;
         s_free     = u;
         s_free_cnt++;
     }
 }
 
 uiox_usb_urb_t *uiox_usb_buf_alloc(void)
 {
     if (!s_free) return NULL;
     uiox_usb_urb_t *u = s_free;
     s_free     = u->next;
     s_free_cnt--;
     u->next       = NULL;
     u->in_use     = 1;
     u->status     = UIOX_URB_IDLE;
     u->actual_len = 0;
     u->complete   = NULL;
     u->ctx        = NULL;
     u->timeout_ms = 5000u;
     memset(&u->setup, 0, sizeof(u->setup));
     return u;
 }
 
 void uiox_usb_buf_ref(uiox_usb_urb_t *urb)
 { if (urb) urb->in_use++; }
 
 void uiox_usb_buf_free(uiox_usb_urb_t *urb)
 {
     if (!urb) return;
     assert(urb->in_use > 0);
     if (--urb->in_use == 0) {
         urb->status = UIOX_URB_IDLE;
         urb->next   = s_free;
         s_free      = urb;
         s_free_cnt++;
     }
 }
 
 uint16_t uiox_usb_buf_free_count(void) { return s_free_cnt; }
 