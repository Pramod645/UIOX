/**
 * @file    uiox_cpu_buf.h
 * @brief   UIOX CPU per-CPU data storage and IPI message pool.
 *
 * Provides:
 *   - Per-CPU data structure (cache-line aligned, no false sharing)
 *   - IPI message ring buffer (lock-free SPSC per core pair)
 *   - Work-queue entry pool for deferred processing
 *
 * @date    2026-06-02
 */

 #ifndef UIOX_CPU_BUF_H
 #define UIOX_CPU_BUF_H
 
 #include "uiox_cpu_hw.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_CPU_CACHE_LINE_BYTES   64u
 #define UIOX_CPU_IPI_RING_SIZE      32u   /**< Must be power of 2          */
 #define UIOX_CPU_IPI_RING_MASK      (UIOX_CPU_IPI_RING_SIZE - 1u)
 #define UIOX_CPU_WORK_POOL_SIZE     64u
 
 /* =========================================================================
  * IPI message
  * ====================================================================== */
 
 typedef enum {
     UIOX_IPI_NOP        = 0,
     UIOX_IPI_RESCHEDULE,     /**< Trigger scheduler on remote core         */
     UIOX_IPI_TLB_FLUSH,     /**< Flush TLB on remote core                 */
     UIOX_IPI_CACHE_FLUSH,   /**< Cache flush on remote core               */
     UIOX_IPI_STOP,          /**< Stop remote core (halting IPI)           */
     UIOX_IPI_FUNC_CALL,     /**< Execute function on remote core          */
     UIOX_IPI_WAKEUP,        /**< Wake sleeping core                       */
 } uiox_ipi_type_t;
 
 typedef struct {
     uiox_ipi_type_t type;
     uint64_t        arg0;
     uint64_t        arg1;
     void          (*fn)(void *ctx);
     void           *fn_ctx;
 } uiox_ipi_msg_t;
 
 /* =========================================================================
  * Per-core IPI ring buffer (one per source core)
  * ====================================================================== */
 
 typedef struct {
     uiox_ipi_msg_t   ring[UIOX_CPU_IPI_RING_SIZE];
     volatile uint32_t head;
     volatile uint32_t tail;
     uint32_t          overflow;
 } uiox_ipi_ring_t;
 
 /* =========================================================================
  * Work item (deferred work queue entry)
  * ====================================================================== */
 
 typedef struct uiox_cpu_work {
     void (*fn)(void *ctx);
     void  *ctx;
     bool   pending;
     struct uiox_cpu_work *next;
 } uiox_cpu_work_t;
 
 /* =========================================================================
  * Per-CPU data block (cache-line padded to prevent false sharing)
  * ====================================================================== */
 
 typedef struct __attribute__((aligned(UIOX_CPU_CACHE_LINE_BYTES))) {
     uint8_t           core_id;
     uint8_t           cluster_id;
     uiox_cpu_core_state_t state;
     uint64_t          jiffies;         /**< Local tick counter             */
     uint64_t          idle_time_ns;    /**< Total idle time accumulated    */
     uint64_t          run_time_ns;     /**< Total run time accumulated     */
     uint32_t          irq_depth;       /**< Interrupt nesting depth        */
     uint32_t          preempt_count;   /**< Preemption disable counter     */
 
     /* IPI rings: one per possible sender core */
     uiox_ipi_ring_t   ipi_in[UIOX_CPU_MAX_CORES];
 
     /* Deferred work queue */
     uiox_cpu_work_t  *work_head;
     uiox_cpu_work_t  *work_tail;
 
     uint8_t           _pad[UIOX_CPU_CACHE_LINE_BYTES -
                             ((sizeof(uint8_t)*3 +
                               sizeof(uint64_t)*4 +
                               sizeof(uint32_t)*2) %
                              UIOX_CPU_CACHE_LINE_BYTES)];
 } uiox_cpu_percpu_t;
 
 /* =========================================================================
  * Global per-CPU array
  * ====================================================================== */
 
 extern uiox_cpu_percpu_t uiox_percpu[UIOX_CPU_MAX_CORES];
 
 /* =========================================================================
  * Buffer / pool API
  * ====================================================================== */
 
 void uiox_cpu_buf_init      (uint8_t num_cores);
 
 /* IPI ring operations */
 bool uiox_ipi_push          (uiox_ipi_ring_t *ring,
                               const uiox_ipi_msg_t *msg);
 bool uiox_ipi_pop           (uiox_ipi_ring_t *ring, uiox_ipi_msg_t *msg);
 bool uiox_ipi_empty         (const uiox_ipi_ring_t *ring);
 
 /* Work queue */
 uiox_cpu_work_t *uiox_work_alloc(void);
 void             uiox_work_free (uiox_cpu_work_t *w);
 void             uiox_work_enqueue(uint8_t core_id,
                                    uiox_cpu_work_t *w);
 uiox_cpu_work_t *uiox_work_dequeue(uint8_t core_id);
 
 /* Per-CPU accessor (compile-time arch dispatch) */
 static inline uiox_cpu_percpu_t *uiox_this_cpu(void)
 {
     extern uiox_cpu_percpu_t uiox_percpu[UIOX_CPU_MAX_CORES];
     uint8_t id = 0;
 #if defined(UIOX_ARCH_ARM64)
     __asm__("mrs %0, mpidr_el1" : "=r"(id)); id &= 0xFFu;
 #elif defined(UIOX_ARCH_X86_64)
     id = uiox_cpu_apic_id();
 #elif defined(UIOX_ARCH_RV64)
     __asm__("csrr %0, mhartid" : "=r"(id));
 #endif
     return &uiox_percpu[id < UIOX_CPU_MAX_CORES ? id : 0];
 }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CPU_BUF_H */
 