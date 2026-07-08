/* ── Keyboard ─── */
#include "../include/uiox_fw_keyboard.h"
#define OPS_KBD(d) ((const uiox_kbd_ops_t*)(d)->priv)
#if defined(__x86_64__)||defined(__i386__)
static inline void _outb_kbd(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"dN"(p));}
static inline uint8_t _inb_kbd(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"dN"(p));return v;}
#endif
static uiox_fw_err_t ps2_kbd_init(uiox_kbd_dev_t *d)
{
#if defined(__x86_64__)||defined(__i386__)
    /* Flush output buffer */
    while(_inb_kbd(PS2_CMD_PORT)&PS2_STATUS_OBF) _inb_kbd(PS2_DATA_PORT);
    d->initialized=true;
#endif
    return UIOX_FW_OK;
}
static void ps2_kbd_deinit(uiox_kbd_dev_t *d){(void)d;}
static uiox_fw_err_t ps2_kbd_set_leds(uiox_kbd_dev_t *d,uint8_t leds)
{
#if defined(__x86_64__)||defined(__i386__)
    _outb_kbd(PS2_DATA_PORT,KBD_SET_LEDS);
    _outb_kbd(PS2_DATA_PORT,leds);
    d->leds=leds;
#else
    (void)d;(void)leds;
#endif
    return UIOX_FW_OK;
}
static void ps2_kbd_isr(uiox_kbd_dev_t *d)
{
#if defined(__x86_64__)||defined(__i386__)
    if(!(_inb_kbd(PS2_CMD_PORT)&PS2_STATUS_OBF))return;
    uint8_t sc=_inb_kbd(PS2_DATA_PORT);
    if(d->key_cb)d->key_cb(sc&0x7Fu,!(sc&0x80u),d->key_priv);
#else
    (void)d;
#endif
}
static const uiox_kbd_ops_t ps2_kbd_ops={ps2_kbd_init,ps2_kbd_deinit,ps2_kbd_set_leds,ps2_kbd_isr};
uiox_fw_err_t uiox_fw_kbd_init(uiox_kbd_dev_t *d,const uiox_kbd_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;d->priv=(void*)o;return o->init(d); }
void uiox_fw_kbd_deinit(uiox_kbd_dev_t *d){if(d&&d->priv&&OPS_KBD(d)->deinit)OPS_KBD(d)->deinit(d);}
uiox_fw_err_t uiox_fw_kbd_set_leds(uiox_kbd_dev_t *d,uint8_t leds)
{ if(!d||!d->priv||!OPS_KBD(d)->set_leds)return UIOX_FW_ERR_INVAL;return OPS_KBD(d)->set_leds(d,leds); }
void uiox_fw_kbd_set_key_cb(uiox_kbd_dev_t *d,uiox_kbd_key_cb_t cb,void *p)
{ if(d){d->key_cb=cb;d->key_priv=p;} }
uiox_fw_err_t uiox_fw_kbd_init_ps2(uiox_kbd_dev_t *d,uint32_t irq)
{ if(!d)return UIOX_FW_ERR_INVAL;uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
  d->type=UIOX_KBD_PS2;d->irq=irq;return uiox_fw_kbd_init(d,&ps2_kbd_ops); }
