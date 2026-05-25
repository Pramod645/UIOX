/*
 * arm64_exceptions.c — AArch64 exception handler stubs
 */
#include "../include/arm64_exceptions.h"

void arm64_exc_sync   (arm64_exception_t e, const arm64_exc_frame_t *f) { (void)e; (void)f; }
void arm64_exc_irq    (arm64_exception_t e, const arm64_exc_frame_t *f) { (void)e; (void)f; }
void arm64_exc_fiq    (arm64_exception_t e, const arm64_exc_frame_t *f) { (void)e; (void)f; }
void arm64_exc_serror (arm64_exception_t e, const arm64_exc_frame_t *f) { (void)e; (void)f; }

/* ── Default handler table ───────────────────────────────── */
static arm64_exc_handler_t g_handlers[4] = {
    arm64_exc_sync,
    arm64_exc_irq,
    arm64_exc_fiq,
    arm64_exc_serror,
};

/*
 * arm64_exc_dispatch() — route an exception to its handler
 */
void arm64_exc_dispatch(arm64_exception_t exc,
                        const arm64_exc_frame_t *frame)
{
    if ((arm64_uint32_t)exc < 4 && g_handlers[exc])
        g_handlers[exc](exc, frame);
}

/*
 * arm64_exc_register() — install a custom handler
 */
void arm64_exc_register(arm64_exception_t    exc,
                        arm64_exc_handler_t  handler)
{
    if ((arm64_uint32_t)exc < 4)
        g_handlers[exc] = handler;
}
