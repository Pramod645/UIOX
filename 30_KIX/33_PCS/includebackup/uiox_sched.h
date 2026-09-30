/*
 * 30_KIX/33_PCS/include/uiox_sched.h
 *
 * Scheduler public API.
 * Implementation: 01_schedular/src/scheduler.c (to be updated).
 *
 * @version 2.0.0  @date 2026-07-23
 */
#ifndef UIOX_SCHED_H
#define UIOX_SCHED_H

#include "../../../../50_UIX/00_libs/00_uixlibs/sys/uix_types.h"
#include "../40_procStruct/include/uiox_task.h"

#ifdef __cplusplus
extern "C" {
#endif

void          uiox_sched_init(void);
void          uiox_sched_add(uiox_task_t *t);
void          uiox_sched_remove(uiox_task_t *t);
void          uiox_sched_yield(void);
void          uiox_task_sleep(uix_uint64_t ticks);
void          uiox_task_wake(uiox_task_t *t);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SCHED_H */
