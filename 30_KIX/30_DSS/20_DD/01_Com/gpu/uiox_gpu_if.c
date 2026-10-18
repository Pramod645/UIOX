/**
 * @file    uiox_gpu_if.c
 * @brief   UIOX GPU interface driver implementation.
 * @date    2026-06-01
 */

 #include "uiox_gpu_if.h"
 #include "uiox_devclass.h"
 #include "uiox_fw_i2c.h"
 #include "uiox_gpu_mem.h"
 
 int uiox_gpu_if_init(uiox_gpu_if_t *gif,
                       uiox_gpu_hw_t *hw,
                       uiox_gpu_mem_t *mem)
 {
     if (!gif || !hw || !mem) return -EINVAL;
     memset(gif, 0, sizeof(*gif));
     gif->hw         = hw;
     gif->mem        = mem;
     gif->next_fence = 1u;
     gif->primed     = true;
     return 0;
 }
 
 uint32_t uiox_gpu_if_submit(uiox_gpu_if_t    *gif,
                               uiox_gpu_buf_t   *cmd_buf,
                               uiox_gpu_priority_t prio,
                               void (*on_complete)(uint32_t, void *),
                               void *ctx)
 {
     if (!gif || !cmd_buf || gif->q_count >= UIOX_GPU_SUBMIT_QUEUE_DEPTH)
         return 0;
 
     uint32_t fence = gif->next_fence++;
     uiox_gpu_submit_t *s = &gif->queue[gif->q_tail % UIOX_GPU_SUBMIT_QUEUE_DEPTH];
     s->cmd_buf     = cmd_buf;
     s->fence_val   = fence;
     s->priority    = prio;
     s->on_complete = on_complete;
     s->ctx         = ctx;
     gif->q_tail++;
     gif->q_count++;
 
     gif->stats.jobs_submitted++;
     gif->stats.bytes_submitted += cmd_buf->used;
 
     /* Immediately flush to hardware */
     uiox_gpu_if_flush(gif);
     return fence;
 }
 
 int uiox_gpu_if_flush(uiox_gpu_if_t *gif)
 {
     if (!gif || !gif->hw) return -EINVAL;
     while (gif->q_count > 0) {
         uiox_gpu_submit_t *s =
             &gif->queue[gif->q_head % UIOX_GPU_SUBMIT_QUEUE_DEPTH];
         int rc = uiox_gpu_hw_cmd_submit(gif->hw,
                                          (uintptr_t)s->cmd_buf->paddr,
                                          s->cmd_buf->used,
                                          s->fence_val);
         if (rc < 0) return rc;
         gif->q_head++;
         gif->q_count--;
     }
     return 0;
 }
 
 int uiox_gpu_if_sync(uiox_gpu_if_t *gif,
                       uint32_t fence_val, uint32_t timeout_ms)
 {
     if (!gif) return -EINVAL;
     gif->stats.fence_waits++;
     int rc = uiox_gpu_hw_fence_wait(gif->hw, fence_val, timeout_ms);
     if (rc == 0) gif->stats.jobs_completed++;
     return rc;
 }
 
 void uiox_gpu_if_stats_get(const uiox_gpu_if_t *gif,
                              uiox_gpu_if_stats_t *out)
 {
     if (!gif || !out) return;
     memcpy(out, &gif->stats, sizeof(*out));
 }
 
 void uiox_gpu_if_stats_reset(uiox_gpu_if_t *gif)
 {
     if (!gif) return;
     memset(&gif->stats, 0, sizeof(gif->stats));
 }
/* ═══════════════════════════ INSERTED — registry layer ══════════════ */

static int gpu_class_init(uiox_dev_dev_t *dev)
{
    uiox_gpu_if_t *gif = (uiox_gpu_if_t *)dev->drv_priv;
    if (!gif || !gif->hw || !gif->mem) return -EINVAL;
    return uiox_gpu_if_init(gif, gif->hw, gif->mem);
}

static void gpu_class_deinit(uiox_dev_dev_t *dev)
{
    (void)dev;   /* no teardown on this interface — the HAL owns it */
}

static int gpu_class_ioctl(uiox_dev_dev_t *dev, uint32_t cmd, void *arg)
{
    uiox_gpu_if_t  *gif = (uiox_gpu_if_t *)dev->drv_priv;
    if (!gif || !gif->hw) return -EINVAL;

    switch (cmd) {
    case UIOX_GPU_IOC_GET_INFO: {
        if (!arg) return -EINVAL;
        uiox_gpu_info_t *o = (uiox_gpu_info_t *)arg;
        o->base    = gif->hw->base_addr;
        o->irq_job = gif->hw->irq_job;
        o->irq_mmu = gif->hw->irq_mmu;
        o->irq_err = gif->hw->irq_err;
        return 0;
    }
    case UIOX_GPU_IOC_GET_STATS:
        if (!arg) return -EINVAL;
        memcpy(arg, &gif->stats, sizeof(gif->stats));
        return 0;
    case UIOX_GPU_IOC_GET_WINDOW:
        if (!arg) return -EINVAL;
        *(uintptr_t *)arg = gif->hw->base_addr;
        return 0;
    case UIOX_GPU_IOC_GET_VRAM: {
        if (!arg || !gif->mem) return -EINVAL;
        uiox_gpu_vram_t *v = (uiox_gpu_vram_t *)arg;
        v->base_phys = gif->mem->heap_base_phys;
        v->size      = gif->mem->heap_size;
        uiox_gpu_mem_stats(gif->mem, &v->used, &v->free_bytes);
        return 0;
    }
    default:
        return -ENOSYS;   /* valid call, no body here */
    }
}

const uiox_gpu_ops_t uiox_gpu_class_ops = {
    .head = { .name = "gpu0", .probe = NULL,
              .init = gpu_class_init, .deinit = gpu_class_deinit,
              .ioctl = gpu_class_ioctl },
};

/* STATIC — §9's uiox_dev_bind() stores the pointer it is given, not a
 * copy, so these must outlive the call. */
static uiox_gpu_mem_t      s_gpu_mem;
static uiox_gpu_hw_t       s_gpu_hw;
static uiox_gpu_if_t       s_gpu_if;
static uiox_dev_dev_t      s_gpu_dev;
static struct uiox_i2c_dev s_gpu_i2c;

int uiox_gpu_probe(void)
{
    if (uiox_gpu_fwbind(&s_gpu_hw, &s_gpu_i2c) < 0)   return -1;

    /* _if_init rejects a NULL mem (uiox_gpu_if.c:12), so the VRAM heap
     * must be a real object — its base and size come from the HAL. */
    if (uiox_gpu_mem_init(&s_gpu_mem, &s_gpu_hw,
                          s_gpu_hw.vram_base, s_gpu_hw.vram_size) < 0)
        return -1;
    if (uiox_gpu_if_init(&s_gpu_if, &s_gpu_hw, &s_gpu_mem) < 0)
        return -1;

    s_gpu_dev.cls  = UIOX_DEVCLASS_GPU;
    s_gpu_dev.unit = 0u;
    s_gpu_dev.bus  = UIOX_BUS_MMIO;      /* a mapped window, as eth/usb */
    s_gpu_dev.base = s_gpu_hw.base_addr; /* the controller's registers  */
    s_gpu_dev.addr = 0u;                 /* no slave address            */
    s_gpu_dev.irq  = s_gpu_hw.irq_err;   /* the fault line              */
    s_gpu_dev.name = "gpu0";
    s_gpu_dev.ops  = &uiox_gpu_class_ops;
    s_gpu_dev.drv_priv = &s_gpu_if;

    return uiox_dev_bind(&s_gpu_dev);
}
/* ═══════════════════════════ END INSERT ═════════════════════════ */
