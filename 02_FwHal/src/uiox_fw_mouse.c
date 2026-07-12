/* ── Mouse ─── */
#include "../include/uiox_fw_mouse.h"
#define OPS_MOUSE(d) ((const uiox_mouse_ops_t*)(d)->priv)
static uiox_fw_err_t ps2_mouse_init(uiox_mouse_dev_t *d){d->initialized=true;return UIOX_FW_OK;}
static void ps2_mouse_deinit(uiox_mouse_dev_t *d){(void)d;}
static void ps2_mouse_isr(uiox_mouse_dev_t *d)
{
#if defined(__x86_64__)||defined(__i386__)
    static uint8_t _inb_m(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"dN"(p));return v;}
    uint8_t b0=_inb_m(0x60u),b1=_inb_m(0x60u),b2=_inb_m(0x60u);
    uiox_mouse_event_t e={(int32_t)(int8_t)b1,(int32_t)(int8_t)b2,0,b0&0x07u};
    d->x+=e.dx; d->y+=e.dy;
    if(d->ev_cb)d->ev_cb(&e,d->ev_priv);
#else
    (void)d;
#endif
}
static const uiox_mouse_ops_t ps2_mouse_ops={ps2_mouse_init,ps2_mouse_deinit,ps2_mouse_isr};
uiox_fw_err_t uiox_fw_mouse_init(uiox_mouse_dev_t *d,const uiox_mouse_ops_t *o)
{ if(!d||!o||!o->init)return UIOX_FW_ERR_INVAL;d->priv=(void*)o;return o->init(d); }
void uiox_fw_mouse_deinit(uiox_mouse_dev_t *d){if(d&&d->priv&&OPS_MOUSE(d)->deinit)OPS_MOUSE(d)->deinit(d);}
void uiox_fw_mouse_set_cb(uiox_mouse_dev_t *d,uiox_mouse_cb_t cb,void *p)
{ if(d){d->ev_cb=cb;d->ev_priv=p;} }
uiox_fw_err_t uiox_fw_mouse_init_ps2(uiox_mouse_dev_t *d,uint32_t irq)
{ if(!d)return UIOX_FW_ERR_INVAL;uint8_t *p=(uint8_t*)d;for(size_t i=0u;i<sizeof(*d);i++)p[i]=0u;
  d->type=UIOX_MOUSE_PS2;d->irq=irq;return uiox_fw_mouse_init(d,&ps2_mouse_ops); }
