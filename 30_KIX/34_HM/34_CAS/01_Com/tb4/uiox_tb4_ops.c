/*
 * 30_KIX/30_DSS/34_CAS/01_Com/tb4/uiox_tb4_ops.c
 *
 * The ops head family 6 binds — the FIRST ioctl implementation in the
 * tree.  uiox_devclass.h:233 declares the slot on the common head so
 * 32_FS reads it at one offset whatever the family is; no class had
 * populated it, so every special file answered ENOTTY.  This fills it.
 *
 * @version 1.0.0  @date 2026-10-12
 */
#include "uiox_tb4_subsys.h"
#include "uiox_devclass.h"      /* uiox_dev_ops_t, uiox_dev_dev_t, bind */
#include "uiox_klibc.h"         /* EINVAL, ENOENT, ENODEV */

typedef enum {
    UIOX_TB4_IOC_STATE_GET = 0,   /* out: uiox_tb4_ioc_state_t         */
    UIOX_TB4_IOC_TOPO_GET  = 1,   /* out: uiox_tb4_ioc_topo_t          */
    UIOX_TB4_IOC_DEV_GET   = 2,   /* in: index; out: uiox_tb4_router_t */
    UIOX_TB4_IOC_APPROVE   = 3,   /* in: uint32_t route_lo             */
    UIOX_TB4_IOC_SEC_GET   = 4,   /* out: mode + auto_approve          */
    UIOX_TB4_IOC_TICK_GET  = 5,   /* out: ticks + uptime_ms            */
} uiox_tb4_ioc_t;

typedef struct { uiox_tb4_state_t state; }                uiox_tb4_ioc_state_t;
typedef struct { uint8_t count; }                         uiox_tb4_ioc_topo_t;
typedef struct { uint8_t index; uiox_tb4_router_t out; }  uiox_tb4_ioc_dev_t;
typedef struct { uiox_tb4_sec_t mode; bool auto_approve; } uiox_tb4_ioc_sec_t;
typedef struct { uint32_t ticks; uint64_t uptime_ms; }    uiox_tb4_ioc_tick_t;

static int tb4_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_tb4_subsys_t *sys;

    if (!dev || !dev->drv_priv) return -EINVAL;
    sys = (uiox_tb4_subsys_t *)dev->drv_priv;

    switch (cmd) {

    case UIOX_TB4_IOC_STATE_GET: {
        uiox_tb4_ioc_state_t *o = (uiox_tb4_ioc_state_t *)arg;
        if (!o) return -EINVAL;
        o->state = uiox_tb4_subsys_state(sys);
        return 0;
    }
    case UIOX_TB4_IOC_TOPO_GET: {
        uiox_tb4_ioc_topo_t *o = (uiox_tb4_ioc_topo_t *)arg;
        if (!o) return -EINVAL;
        return uiox_tb4_subsys_topo_get(sys, &o->count);
    }
    case UIOX_TB4_IOC_DEV_GET: {
        uiox_tb4_ioc_dev_t *o = (uiox_tb4_ioc_dev_t *)arg;
        if (!o) return -EINVAL;
        return uiox_tb4_subsys_dev_get(sys, o->index, &o->out);
    }
    case UIOX_TB4_IOC_APPROVE: {
        uint32_t *route_lo = (uint32_t *)arg;
        uiox_tb4_router_t *r;
        if (!route_lo) return -EINVAL;
        r = uiox_tb4_topo_find(&sys->topo, 0u, *route_lo);
        if (!r) return -ENOENT;
        return uiox_tb4_subsys_approve(sys, r);
    }
    case UIOX_TB4_IOC_SEC_GET: {
        uiox_tb4_ioc_sec_t *o = (uiox_tb4_ioc_sec_t *)arg;
        if (!o) return -EINVAL;
        return uiox_tb4_subsys_sec_get(sys, &o->mode, &o->auto_approve);
    }
    case UIOX_TB4_IOC_TICK_GET: {
        uiox_tb4_ioc_tick_t *o = (uiox_tb4_ioc_tick_t *)arg;
        if (!o) return -EINVAL;
        uiox_tb4_subsys_stats_get(sys, &o->ticks, &o->uptime_ms);
        return 0;
    }
    default:
        return -EINVAL;
    }
}

static int tb4_probe(const uiox_dev_dev_t *dev)
{ return (dev && dev->drv_priv) ? 0 : -ENODEV; }

static int tb4_init(uiox_dev_dev_t *dev)
{
    if (!dev || !dev->drv_priv) return -EINVAL;
    return uiox_tb4_subsys_start((uiox_tb4_subsys_t *)dev->drv_priv);
}

static void tb4_deinit(uiox_dev_dev_t *dev)
{
    if (!dev || !dev->drv_priv) return;
    uiox_tb4_subsys_stop((uiox_tb4_subsys_t *)dev->drv_priv);
}

const uiox_dev_ops_t uiox_tb4_dev_ops = {
    .name   = "tb4",
    .probe  = tb4_probe,
    .init   = tb4_init,
    .deinit = tb4_deinit,
    .ioctl  = tb4_ioctl,
};

/* bind() stores the POINTER, not a copy — s_dev must be static. */
int uiox_tb4_dev_bind(uiox_tb4_subsys_t *sys)
{
    static uiox_dev_dev_t s_dev;

    if (!sys) return -EINVAL;

    s_dev.cls      = UIOX_DEVCLASS_TB4;   /* (UIOX_DEVFAM_BUS, 0) */
    s_dev.unit     = 0u;
    s_dev.name     = "tb4";
    s_dev.ops      = &uiox_tb4_dev_ops;
    s_dev.drv_priv = (void *)sys;

    return uiox_dev_bind(&s_dev);
}

/* ── probe — instantiate, register, then start ───────────────────────
 * The subsystem must be STATIC: uiox_dev_bind() stores the pointer, not
 * a copy (uiox_devclass.h SECTION 9), so the descriptor has to outlive
 * this call for the life of the kernel.
 *
 * Order: init fills the subsystem, bind publishes it, start brings it up.
 * Binding before init would publish a descriptor whose drv_priv has not
 * been set up yet; starting before bind is harmless but pointless, since
 * nothing can find the class until the registry has it.
 * ──────────────────────────────────────────────────────────────────── */
static uiox_tb4_subsys_t s_tb4_sys;

int uiox_tb4_probe(uiox_tb4_hw_t *hw, uiox_tb4_sec_t security)
{
    int rc;

    if (!hw) return -EINVAL;

    rc = uiox_tb4_subsys_init(&s_tb4_sys, hw, security);
    if (rc < 0) return rc;

    rc = uiox_tb4_dev_bind(&s_tb4_sys);
    if (rc < 0) return rc;

    return uiox_tb4_subsys_start(&s_tb4_sys);
}
