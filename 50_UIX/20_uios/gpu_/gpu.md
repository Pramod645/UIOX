Application / Device Access API        (uiox_gpu_device)
  → GPU Subsystem: pipeline, shader, render pass         (uiox_gpu_subsys)
    → Command Buffer & Queue: draw calls, dispatch        (uiox_gpu_cmd)
    → Memory Manager: VRAM, heap, buffer objects          (uiox_gpu_mem)
    → Interface driver: submit, fence, sync               (uiox_gpu_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, clocks      (uiox_gpu_hw)
    ↔ Buffer Manager: vertex, index, uniform, framebuffer (uiox_gpu_buf)
================================
uiox-gpu/
├── include/
│   ├── uiox_gpu_hw.h          # Layer 1  — HAL: MMIO, DMA, IRQ, PLL
│   ├── uiox_gpu_buf.h         # Buffer objects: VBO, IBO, UBO, FBO
│   ├── uiox_gpu_mem.h         # Memory manager: VRAM heap, allocator
│   ├── uiox_gpu_if.h          # Layer 2  — Interface: submit, fence, sync
│   ├── uiox_gpu_cmd.h         # Layer 3  — Command buffer: draw, dispatch
│   ├── uiox_gpu_subsys.h      # Layer 4  — Subsystem: pipeline, render pass
│   └── uiox_gpu_device.h      # Layer 5  — Application-facing API
└── src/
    ├── uiox_gpu_hw.c
    ├── uiox_gpu_buf.c
    ├── uiox_gpu_mem.c
    ├── uiox_gpu_if.c
    ├── uiox_gpu_cmd.c
    ├── uiox_gpu_subsys.c
    ├── uiox_gpu_device.c
    └── uiox_gpu_demo.c
================================
uiox-gpu/
├── include/
│   ├── uiox_gpu_hw.h      # Layer 1   — HAL: MMIO regs, shader core config,
│   │                      #              DMA command ring, GPU MMU/IOMMU,
│   │                      #              fence seqno, thermal, perf counters,
│   │                      #              power mgmt, TBR tile config
│   ├── uiox_gpu_buf.h     # Layer 1.5 — Buffer objects: VBO/IBO/UBO/CMD pools,
│   │                      #              texture descriptors, FBO + attachments,
│   │                      #              reference counting
│   ├── uiox_gpu_mem.h     # Layer 1.6 — VRAM memory manager: linear allocator
│   │                      #              with free-list merge, GPU MMU mapping,
│   │                      #              alignment, memory stats
│   ├── uiox_gpu_if.h      # Layer 2   — Interface: submit queue (16 deep),
│   │                      #              priority scheduling (high/normal/low),
│   │                      #              fence wait, completion callbacks,
│   │                      #              IF statistics
│   ├── uiox_gpu_cmd.h     # Layer 3   — Command buffer: binary encoding of
│   │                      #              bind_shader/VBO/IBO/UBO/TEX/FBO,
│   │                      #              viewport/scissor/depth/blend/cull,
│   │                      #              clear, push_constants, draw,
│   │                      #              draw_indexed, dispatch, barrier
│   ├── uiox_gpu_subsys.h  # Layer 4   — Subsystem: PSO table (16 slots),
│   │                      #              render pass begin/end, PSO bind,
│   │                      #              frame begin/end + submit + sync,
│   │                      #              frame statistics
│   └── uiox_gpu_device.h  # Layer 5   — Application API: open/start/stop/
│                          #              close/create_pso/destroy_pso/
│                          #              begin_frame/begin_pass/end_pass/
│                          #              bind_pso/end_frame/cmd accessor/
│                          #              alloc_vbo/ibo/ubo/tex/fbo/
│                          #              load_shader/get_stats/print_info
└── src/
    ├── uiox_gpu_hw.c      # HAL lifecycle: init/deinit/power on-off,
    │                      #   cmd_submit, fence_wait, shader_load
    ├── uiox_gpu_buf.c     # Static pools: VBO/IBO/UBO/CMD/TEX/FBO,
    │                      #   alloc/free/ref, fbo_attach
    ├── uiox_gpu_mem.c     # Linear allocator with alignment + free-list merge,
    │                      #   MMU map/unmap on alloc/free, stats
    ├── uiox_gpu_if.c      # Submit queue push, flush to HW, fence wait,
    │                      #   completion callback dispatch, stats
    ├── uiox_gpu_cmd.c     # Binary command encoder: opcode + payload write,
    │                      #   begin/end/reset, all draw/state/compute cmds
    ├── uiox_gpu_subsys.c  # Subsystem init/deinit, PSO create/destroy,
    │                      #   begin/end frame, begin/end pass, bind PSO,
    │                      #   submit + sync, frame stats
    ├── uiox_gpu_device.c  # All API wrappers, alloc_vbo/ibo/ubo/tex/fbo,
    │                      #   load_shader, print_info/print_stats,
    │                      #   state/fmt name helpers
    └── uiox_gpu_demo.c    # Mali-G76 stub HAL, shader load, PSO create,
                           #   3 render frames (quad + compute), wireframe
                           #   pass, memory stats, teardown
====================
Key Design Decisions
Decision	Rationale
Static buffer pools (VBO/IBO/UBO/CMD/TEX/FBO)	Zero heap fragmentation; all GPU resources pre-allocated at boot — critical for deterministic embedded rendering
Linear VRAM allocator with free-list merge	Simple, predictable allocation; adjacent free blocks coalesced on free — avoids fragmentation without buddy overhead
Binary command buffer encoding	GPU commands serialised as opcode + fixed payload; zero-copy submit via DMA; portable across GPU ISAs by keeping HAL adaptation in one place
PSO (Pipeline State Object) table	Pre-validated pipeline state eliminates per-draw validation; 16-slot table suitable for embedded UIs
Fence-based synchronisation	Monotonically-increasing sequence numbers; fence_wait() blocks until GPU writes seqno — correct without OS semaphores
Priority submit queue (3 levels)	VO=UI critical, Normal=3D, Low=compute/background — prevents audio/UI latency from heavy