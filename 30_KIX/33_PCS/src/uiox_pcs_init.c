/*
 * 30_KIX/33_PCS/src/uiox_pcs_init.c
 * Author: Pramod Kumar
 * Description: Process-control subsystem bring-up — the single entry point
 *              the kernel calls to initialise 33_PCS.  Defines
 *              uiox_proc_init(), which uiox_kernel_main.c declares:
 *
 *                  extern void uiox_proc_init(void);
 *
 * NOTE ON THE NAME: pcs.md refers to "src/uiox_proc_init.c" in three
 * places (473, 578, 3428).  No file of that name exists — this is it,
 * under uiox_pcs_init.c.
 *
 * ── boot context ─────────────────────────────────────────────────────
 * Called from kernel_common_init() in uiox_kernel_main.c, after
 * uiox_fs_init() and before uiox_shell_start().  By that point:
 *     stack_setup()   and bss_zero() have run
 *     g_boot_args     holds the uiox_boot_args_t the bootloader built
 *     g_dtb_pa        holds the DTB physical address
 * The boot loader delivered both through x0 (dtb_pa) and x1 (args_pa),
 * and uiox_kernel_get_boot_args() is the accessor for the second.
 *
 * ── order, and why ───────────────────────────────────────────────────
 *   1. memory        physical allocator, before any allocation
 *   2. region table  40_psa's one initialiser
 *   3. scheduler     run queues, needs the process model
 *   4. timer         the tick the scheduler consumes
 *   5. security      ASLR needs a process; MAC needs the table
 *   6. patch engine  last — it rewrites text nothing above may be in
 *
 * Reversing any pair produces a call on uninitialised state rather than
 * an error, which is why the order is written out here and not left to
 * the reader.
 *
 * @version 1.4.0  @date 2026-10-05
 */

 #include "uiox_klibc.h"
 #include "uiox_boot_handoff.h"      /* uiox_boot_args_t, uiox_mem_map_t  */
 #include "uiox_kix_psa_region.h"    /* uiox_kix_psa_region_init()        */
 
 /* ── the layers this file drives ──────────────────────────────────────
  * Declared here rather than pulled in, so a layer that is not compiled
  * produces a missing symbol at link time instead of a missing include at
  * compile time — the failure that names the absent file.
  *
  * uiox_mm_init's parameter types are uiox_uint64_t, matching both this
  * declaration and uiox_phys_alloc.c:109.  The old uix_ prefix had no
  * typedef anywhere in 30_KIX and has been renamed. */
 extern void uiox_mm_init(uiox_uint64_t dram_base, uiox_uint64_t dram_size);
 
 /* These four already exist as weak stubs in uiox_kernel_main.c; the real
  * definitions in their own layers win at link time. */
 extern void uiox_sched_init(void);
 extern void uiox_timer_init(void);
 extern void uiox_sec_init(void);
 extern void uiox_kp_engine_init(void);
 
 /* From uiox_kernel_main.c — returns g_boot_args. */
 extern const uiox_boot_args_t *uiox_kernel_get_boot_args(void);
 
 static bool s_pcs_ready = false;
 
 /* ═════════════════════════════════════════════════════════════════════
  * The RAM window
  *
  * uiox_mm_init() takes ONE contiguous span and caps at UIOX_MAX_PAGES
  * (16384 pages = 64 MB, per MemMngm.md:37).  The boot hand-off supplies a
  * uiox_mem_map_t of up to UIOX_MEM_MAX_REGIONS (32) typed regions, and a
  * real machine's usable RAM is usually several of them.
  *
  * The largest usable region is the one chosen: DTB region order is not
  * guaranteed, and the first usable entry is often a small low-memory
  * window the firmware left in place.
  *
  * A machine whose RAM is fragmented across several large regions needs
  * uiox_mm_init() extended to walk them — that is a change to the page
  * table's sizing, not to this call site.
  * ═════════════════════════════════════════════════════════════════════ */
 static bool pcs_ram_window(uint64_t *base_out, uint64_t *size_out)
 {
     const uiox_boot_args_t *ba = uiox_kernel_get_boot_args();
 
     if (!ba || ba->mem_map.count == 0u) return false;
 
     const uiox_mem_region_t *best = (const uiox_mem_region_t *)0;
 
     for (uint32_t i = 0u; i < ba->mem_map.count && i < UIOX_MEM_MAX_REGIONS; i++) {
         const uiox_mem_region_t *r = &ba->mem_map.regions[i];
 
         if (r->type != UIOX_MEM_USABLE || r->size == 0u) continue;
         if (!best || r->size > best->size) best = r;
     }
 
     if (!best) return false;
 
     *base_out = best->base;
     *size_out = best->size;
     return true;
 }
 
 /* ═════════════════════════════════════════════════════════════════════
  * uiox_proc_init — called once from kernel_common_init()
  *
  * Returns void, matching the kernel's declaration, so a failure here
  * cannot be reported upward and must be handled by not proceeding.
  * ═════════════════════════════════════════════════════════════════════ */
 void uiox_proc_init(void)
 {
     uint64_t ram_base = 0u;
     uint64_t ram_size = 0u;
 
     if (s_pcs_ready) return;
 
     /* ── 1. memory ──────────────────────────────────────────────────
      * First: the physical allocator backs every structure the layers
      * above allocate, and uiox_phys_alloc.c:7/:107 both name this
      * function as its caller.
      *
      * If the boot args carry no usable region, STOP.  Handing the
      * allocator a fabricated window is worse than not starting it: it
      * would hand out pages the bootloader never mapped, and the fault
      * surfaces later as corruption at an unrelated address. */
     if (!pcs_ram_window(&ram_base, &ram_size)) return;
 
     uiox_mm_init((uiox_uint64_t)ram_base, (uiox_uint64_t)ram_size);
 
     /* ── 2. region table ────────────────────────────────────────────
      * 40_psa's ONE initialiser (uiox_kix_psa_region.h:205).  The
      * process table needs none — uiox_kix_psa_process.h:216 allocates
      * slots lazily and sets state CREATED on each. */
     uiox_kix_psa_region_init();
 
     /* ── 3. scheduler ───────────────────────────────────────────────
      * Run queues hold uiox_kix_psa_proc_t *, so they follow the region
      * table and the process model's static state. */
     uiox_sched_init();
 
     /* ── 4. timer ───────────────────────────────────────────────────
      * The tick drives the scheduler, so it comes after the run queues
      * are ready to receive it. */
     uiox_timer_init();
 
     /* ── 5. security ────────────────────────────────────────────────
      * ASLR needs a loaded process to randomise; MAC needs the process
      * table to label.  Both exist by now. */
     uiox_sec_init();
 
     /* ── 6. patch engine ────────────────────────────────────────────
      * Last: it patches kernel text, so nothing above may be executing
      * code that is about to be rewritten. */
     uiox_kp_engine_init();
 
     s_pcs_ready = true;
 }
 