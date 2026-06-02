/**
 * @file    uiox_gpu_hw.h
 * @brief   UIOX GPU Hardware Abstraction Layer (HAL).
 *
 * Lowest-level interface to GPU hardware. Owns:
 *   - MMIO register access to GPU command processor
 *   - Shader core / execution unit configuration
 *   - DMA engine for command buffer submission
 *   - Tile-based rendering engine (TBR) control
 *   - Memory-mapped VRAM / shared system RAM mapping
 *   - IRQ handling: job done, fault, MMU, thermal
 *   - Power management: core voltage, frequency scaling
 *   - Performance counters
 *
 * Supports:
 *   - Imagination PowerVR (tile-based)
 *   - ARM Mali (tile-based deferred rendering)
 *   - Vivante GC series (embedded GPU)
 *   - Custom softcore GPU (FPGA)
 *
 * @version 1.0.0
 * @date    2026-06-01
 */

 #ifndef UIOX_GPU_HW_H
 #define UIOX_GPU_HW_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Hardware capability flags
  * ====================================================================== */
 
 #define UIOX_GPU_CAP_COMPUTE        (1u << 0)  /**< Compute shaders        */
 #define UIOX_GPU_CAP_GEOMETRY_SH    (1u << 1)  /**< Geometry shaders       */
 #define UIOX_GPU_CAP_TESSELLATION   (1u << 2)  /**< Tessellation           */
 #define UIOX_GPU_CAP_MSAA           (1u << 3)  /**< Multi-sample AA        */
 #define UIOX_GPU_CAP_HDR            (1u << 4)  /**< HDR framebuffer        */
 #define UIOX_GPU_CAP_ASTC           (1u << 5)  /**< ASTC texture compress  */
 #define UIOX_GPU_CAP_ETC2           (1u << 6)  /**< ETC2 texture compress  */
 #define UIOX_GPU_CAP_TBR            (1u << 7)  /**< Tile-based rendering   */
 #define UIOX_GPU_CAP_TBDR           (1u << 8)  /**< Tile-based deferred    */
 #define UIOX_GPU_CAP_DMA            (1u << 9)  /**< DMA command submit     */
 #define UIOX_GPU_CAP_MMU            (1u << 10) /**< GPU MMU / IOMMU        */
 #define UIOX_GPU_CAP_PERF_COUNTERS  (1u << 11) /**< HW perf counters       */
 #define UIOX_GPU_CAP_THERMAL_MGMT   (1u << 12) /**< Thermal throttle       */
 #define UIOX_GPU_CAP_UNIFIED_MEM    (1u << 13) /**< CPU/GPU shared memory  */
 
 /* =========================================================================
  * GPU pixel formats
  * ====================================================================== */
 
 typedef enum {
     UIOX_GPU_FMT_RGBA8888 = 0,
     UIOX_GPU_FMT_RGB888,
     UIOX_GPU_FMT_RGB565,
     UIOX_GPU_FMT_RGBA4444,
     UIOX_GPU_FMT_R8,
     UIOX_GPU_FMT_RG8,
     UIOX_GPU_FMT_RGBA16F,
     UIOX_GPU_FMT_RGBA32F,
     UIOX_GPU_FMT_D16,
     UIOX_GPU_FMT_D24S8,
     UIOX_GPU_FMT_D32F,
 } uiox_gpu_fmt_t;
 
 /* =========================================================================
  * Shader stage flags
  * ====================================================================== */
 
 #define UIOX_GPU_STAGE_VERTEX       (1u << 0)
 #define UIOX_GPU_STAGE_FRAGMENT     (1u << 1)
 #define UIOX_GPU_STAGE_COMPUTE      (1u << 2)
 #define UIOX_GPU_STAGE_GEOMETRY     (1u << 3)
 
 /* =========================================================================
  * GPU topology (primitive types)
  * ====================================================================== */
 
 typedef enum {
     UIOX_GPU_TOPO_TRIANGLES = 0,
     UIOX_GPU_TOPO_TRIANGLE_STRIP,
     UIOX_GPU_TOPO_TRIANGLE_FAN,
     UIOX_GPU_TOPO_LINES,
     UIOX_GPU_TOPO_LINE_STRIP,
     UIOX_GPU_TOPO_POINTS,
 } uiox_gpu_topology_t;
 
 /* =========================================================================
  * DMA descriptor
  * ====================================================================== */
 
 #define UIOX_GPU_DMA_DESC_ALIGN  64
 
 typedef struct __attribute__((packed, aligned(UIOX_GPU_DMA_DESC_ALIGN))) {
     volatile uint32_t  status;      /**< OWN + done/fault flags            */
     uint32_t           ctrl;        /**< Command type + size               */
     uint32_t           buf_lo;      /**< Command buffer physical addr lo   */
     uint32_t           buf_hi;
     uint32_t           size;        /**< Size in bytes                     */
     uint32_t           fence_val;   /**< Fence value to write on complete  */
     uint32_t           reserved[2];
 } uiox_gpu_dma_desc_t;
 
 #define UIOX_GPU_DESC_OWN    (1u << 31)
 #define UIOX_GPU_DESC_DONE   (1u << 1)
 #define UIOX_GPU_DESC_FAULT  (1u << 0)
 
 /* =========================================================================
  * GPU hardware device descriptor
  * ====================================================================== */
 
 typedef struct {
     uintptr_t           base_addr;     /**< MMIO base of GPU controller   */
     uint32_t            irq_job;       /**< Job complete IRQ               */
     uint32_t            irq_mmu;       /**< MMU fault IRQ                  */
     uint32_t            irq_err;       /**< Error / thermal IRQ            */
     uint32_t            caps;          /**< UIOX_GPU_CAP_* bitmask        */
     uint32_t            num_cores;     /**< Shader core count              */
     uint32_t            num_exec_units;/**< Execution units per core       */
     uint32_t            tile_width;    /**< Tile width (pixels, TBR)       */
     uint32_t            tile_height;
     uint64_t            vram_base;     /**< VRAM physical base address     */
     uint64_t            vram_size;     /**< VRAM size (bytes)              */
     uint32_t            max_freq_mhz;
     uint32_t            cur_freq_mhz;
 
     /* DMA rings */
     uiox_gpu_dma_desc_t *cmd_ring;    /**< Command submit ring            */
     uint16_t             cmd_ring_sz;
     uint16_t             cmd_head;
     uint16_t             cmd_tail;
 
     /* Fence / sync */
     volatile uint32_t    fence_seqno; /**< HW writes seqno on job done    */
     uint32_t             submitted_seqno;
 
     /* State */
     bool                 powered;
     bool                 fault;
     uint32_t             fault_addr;
 
     void                *priv;
 } uiox_gpu_hw_t;
 
 /* =========================================================================
  * Hardware operations vtable
  * ====================================================================== */
 
 typedef struct {
     int  (*init)          (uiox_gpu_hw_t *hw);
     void (*deinit)        (uiox_gpu_hw_t *hw);
     int  (*power_on)      (uiox_gpu_hw_t *hw);
     void (*power_off)     (uiox_gpu_hw_t *hw);
     int  (*set_freq)      (uiox_gpu_hw_t *hw, uint32_t freq_mhz);
 
     /** Map a physical address range into GPU MMU. */
     int  (*mmu_map)       (uiox_gpu_hw_t *hw,
                            uint64_t gpu_va, uint64_t phys,
                            uint64_t size, uint32_t flags);
     void (*mmu_unmap)     (uiox_gpu_hw_t *hw,
                            uint64_t gpu_va, uint64_t size);
 
     /** Submit a command buffer (physical address + size) via DMA. */
     int  (*cmd_submit)    (uiox_gpu_hw_t *hw,
                            uintptr_t phys, uint32_t size,
                            uint32_t fence_val);
 
     /** Wait for fence to be signalled (blocking up to timeout_ms). */
     int  (*fence_wait)    (uiox_gpu_hw_t *hw,
                            uint32_t fence_val, uint32_t timeout_ms);
 
     /** Load a compiled shader binary into GPU shader store. */
     int  (*shader_load)   (uiox_gpu_hw_t *hw,
                            const uint8_t *binary, uint32_t size,
                            uint32_t stage_flags,
                            uint32_t *shader_id_out);
 
     /** Unload a shader from GPU shader store. */
     void (*shader_unload) (uiox_gpu_hw_t *hw, uint32_t shader_id);
 
     /** Read a hardware performance counter. */
     uint64_t (*perf_read) (uiox_gpu_hw_t *hw, uint8_t counter_id);
 
     /** ISRs */
     void (*isr_job)       (uiox_gpu_hw_t *hw);
     void (*isr_mmu)       (uiox_gpu_hw_t *hw);
     void (*isr_err)       (uiox_gpu_hw_t *hw);
 } uiox_gpu_hw_ops_t;
 
 /* =========================================================================
  * HAL public API
  * ====================================================================== */
 
 int  uiox_gpu_hw_init      (uiox_gpu_hw_t *hw,
                              const uiox_gpu_hw_ops_t *ops);
 void uiox_gpu_hw_deinit    (uiox_gpu_hw_t *hw);
 int  uiox_gpu_hw_power_on  (uiox_gpu_hw_t *hw);
 void uiox_gpu_hw_power_off (uiox_gpu_hw_t *hw);
 int  uiox_gpu_hw_cmd_submit(uiox_gpu_hw_t *hw,
                              uintptr_t phys, uint32_t size,
                              uint32_t fence_val);
 int  uiox_gpu_hw_fence_wait(uiox_gpu_hw_t *hw,
                              uint32_t fence_val, uint32_t timeout_ms);
 int  uiox_gpu_hw_shader_load(uiox_gpu_hw_t *hw,
                               const uint8_t *binary, uint32_t size,
                               uint32_t stage_flags,
                               uint32_t *shader_id_out);
 
 static inline uint32_t uiox_gpu_caps(const uiox_gpu_hw_t *hw)
 { return hw ? hw->caps : 0u; }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_GPU_HW_H */
 