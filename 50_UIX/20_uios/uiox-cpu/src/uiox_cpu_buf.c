/**
 * @file    uiox_cpu_buf.c
 * @brief   UIOX CPU per-CPU data and IPI ring buffer implementation.
 * @date    2026-06-02
 */

 #include "uiox_cpu_buf.h"
 #include <string.h>
 #include <assert.h>
 
 uiox_cpu_percpu_t uiox_percpu[UIOX_CPU_MAX_CORES];
 
 static uiox_cpu_work_t  s_work_pool[UIOX_CPU_WORK_POOL_SIZE];
 static uiox_cpu_work_t *s_work_free = NULL;
 
 void uiox_cpu_buf_init(uint8_t num_cores)
 {
     memset(uiox_percpu, 0, sizeof(uiox_percpu));
     for (uint8_t i = 0; i < num_cores && i < UIOX_CPU_MAX_CORES; i++) {
         uiox_percpu[i].core_id = i;
         uiox_percpu[i].state   = UIOX_CPU_STATE_OFFLINE;
         for (uint8_t j = 0; j < UIOX_CPU_MAX_CORES; j++) {
             uiox_percpu[i].ipi_in[j].head = 0;
             uiox_percpu[i].ipi_in[j].tail = 0;
         }
     }
     s_work_free = NULL;
     for (int i = UIOX_CPU_WORK_POOL_SIZE - 1; i >= 0; i--) {
         s_work_pool[i].pending = false;
         s_work_pool[i].next    = s_work_free;
         s_work_free            = &s_work_pool[i];
     }
 }
 
 bool uiox_ipi_push(uiox_ipi_ring_t *ring, const uiox_ipi_msg_t *msg)
 {
     if (!ring || !msg) return false;
     uint32_t next = (ring->head + 1u) & UIOX_CPU_IPI_RING_MASK;
     if (next == ring->tail) { ring->overflow++; return false; }
     ring->ring[ring->head] = *msg;
     ring->head = next;
     return true;
 }
 
 bool uiox_ipi_pop(uiox_ipi_ring_t *ring, uiox_ipi_msg_t *msg)
 {
     if (!ring || !msg || ring->head == ring->tail) return false;
     *msg       = ring->ring[ring->tail];
     ring->tail = (ring->tail + 1u) & UIOX_CPU_IPI_RING_MASK;
     return true;
 }
 
 bool uiox_ipi_empty(const uiox_ipi_ring_t *ring)
 { return ring ? (ring->head == ring->tail) : true; }
 
 uiox_cpu_work_t *uiox_work_alloc(void)
 {
     if (!s_work_free) return NULL;
     uiox_cpu_work_t *w = s_work_free;
     s_work_free = w->next;
     w->next     = NULL;
     w->pending  = true;
     return w;
 }
 
 void uiox_work_free(uiox_cpu_work_t *w)
 {
     if (!w) return;
     w->pending  = false;
     w->fn       = NULL;
     w->ctx      = NULL;
     w->next     = s_work_free;
     s_work_free = w;
 }
 
 void uiox_work_enqueue(uint8_t core_id, uiox_cpu_work_t *w)
 {
     if (!w || core_id >= UIOX_CPU_MAX_CORES) return;
     uiox_cpu_percpu_t *cpu = &uiox_percpu[core_id];
     w->next = NULL;
     if (cpu->work_tail) cpu->work_tail->next = w;
     else               cpu->work_head        = w;
     cpu->work_tail = w;
 }
 
 uiox_cpu_work_t *uiox_work_dequeue(uint8_t core_id)
 {
     if (core_id >= UIOX_CPU_MAX_CORES) return NULL;
     uiox_cpu_percpu_t *cpu = &uiox_percpu[core_id];
     uiox_cpu_work_t *w = cpu->work_head;
     if (!w) return NULL;
     cpu->work_head = w->next;
     if (!cpu->work_head) cpu->work_tail = NULL;
     return w;
 }
 