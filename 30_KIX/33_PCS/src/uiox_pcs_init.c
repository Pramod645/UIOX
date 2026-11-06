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
 #include "uiox_kix_psa_proc_algo.h"  /* uiox_kix_psa_u_area_t, uarea_for() */

 #include "uiox_base_types.h"        /* uiox_uint64_t, uiox_uintptr_t */

 #include "uiox_kix_psa_process.h"    /* proc_alloc, set_state, sched_enqueue */
 #include "uiox_kix_psa_context.h"    /* uiox_kix_psa_sys_context_t          */

 #include "uiox_kix_paging.h"

 #include "arch_runtime.h"
 #include "irq.h"              /* uiox_clock_tick_hook */
 #include "uiox_kix_clock.h"   /* clock_tick           */
 /* ── the layers this file drives ──────────────────────────────────────*/
  /* Declared here rather than pulled in, so a layer that is not compiled
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
 /* ── kinit_main — PID 1's first instruction ───────────────────────────
 * Runs as a kernel thread on the context's sc_kstack.  Its ONLY job is
 * to get to the point where a user process can exist: mount a root so a
 * path resolves, then exec the OS.  Everything a real init decides —
 * which apps, what priority, when to reap — belongs to UIOS, which runs
 * as a user process after the exec below.
 *
 * A kernel thread that returns is a bug.  Both exits park. */
#include "uiox_kix_scpcs.h"     /* uiox_kix_scpcs_execve — same tree */
extern int vfs_mount_root(uint8_t dev);
extern int uiox_fs_populate_root(void);   /* 32_FS/src/uiox_fs_populate.c */


void kinit_main(void)
{
    int64_t rc;
    char *argv[] = { (char *)"/bin/uios", (char *)0 };
    char *envp[] = { (char *)0 };
    early_puts("[init] mount root...\r\n");
    if (vfs_mount_root(0u) != 0) {          /* ROOT_DEV 0 — where UNFS is mounted */
        early_puts("[init] mount failed\r\n");
        for (;;) { }
    }

    early_puts("[init] populate /bin...\r\n");
    if (uiox_fs_populate_root() != 0)
        early_puts("[init] populate failed\r\n");

    /* Kernel thread: no svc, no user stack.  Call the kernel function,
     * not 50_UIX's sys_execve — that one traps and would come back
     * through arch_sys_call_dispatch we are already beneath. */
    early_puts("[init] exec /bin/uios...\r\n");
    rc = uiox_kix_scpcs_execve((uiox_uintptr_t)"/bin/uios",
                                (uiox_uintptr_t)argv,
                                (uiox_uintptr_t)envp);
    early_puts("[init] exec rc="); early_puthex((uint64_t)rc); early_puts("\r\n");
    for (;;) { }
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
     early_puts("[kernel] ram window OK\r\n");   /* or the failure case */

 
     uiox_mm_init((uiox_uint64_t)ram_base, (uiox_uint64_t)ram_size);

     /*
     * Why added below call: nothing in the tree calls uiox_kix_page_fault_init() today — only 
     * its comment and definition exist. Without this, fpl_count stays 0 and the first 
     * allocation prints [fault] no free frames. It must follow uiox_mm_init, which it does.
     */
     uiox_kix_page_fault_init();      /* ← NEW: builds the frame ring added during OS update  */
 
     /* ── 2. region table ────────────────────────────────────────────
      * 40_psa's ONE initialiser (uiox_kix_psa_region.h:205).  The
      * process table needs none — uiox_kix_psa_process.h:216 allocates
      * slots lazily and sets state CREATED on each. */
     uiox_kix_psa_region_init();
 
    /* ── 3. scheduler ───────────────────────────────────────────────
     * Run queues hold uiox_kix_psa_proc_t *, so they follow the region
     * table and the process model's static state. */
    uiox_sched_init();

    /* ── 3a. PID 1 — the init process ───────────────────────────────
     * Created here, not later: the run queue is ready and the timer
     * that drives switching is one step away, so a process enqueued now
     * is the first thing the tick will pick up.
     *
     * It starts as a KERNEL thread running init_main().  That is the
     * only honest order — execve() needs a mounted root to resolve a
     * path, and mounting is not a user operation.  init_main mounts,
     * opens the console, and only then execs.
     *
     * A failure is fatal to the OS but not to the kernel: proc_init
     * returns void, so the best this can do is leave the queues empty
     * and let kernel_common_init proceed to the shell as it does today. */
    /* ── 3a. PID 1 — the init process ───────────────────────────────
     * proc_alloc puts the slot in CREATED; the context carries WHERE it
     * runs (rc_pc) and on what stack (sc_kstack).  A proc slot without
     * a context is a process the scheduler can enqueue but cannot
     * switch to. */
    /* ── 3a. PID 1 — created here, becomes the OS after exec ────────
     * kinit_main is deliberately tiny: mount, then hand off.  Every
     * policy decision — which apps, what priority — belongs to UIOS,
     * which runs as a user process once /bin/uios is exec'd. */
     {
        uiox_kix_psa_proc_t *init = uiox_kix_psa_proc_alloc(1u, 0u);
        if (init) {
            uint32_t slot = (uint32_t)(init - uiox_kix_psa_proc_table);
            uiox_kix_psa_sys_context_t *ctx = uiox_kix_psa_context_for(slot);

            if (ctx) {
                memset(ctx, 0, sizeof(*ctx));
                ctx->sc_proc_entry = init;
                ctx->sc_layer_top  = 0;
                ctx->sc_layers[0].rc_pc = (uintptr_t)kinit_main;
                ctx->sc_layers[0].rc_sp = (uintptr_t)(ctx->sc_kstack
                                              + sizeof(ctx->sc_kstack));
                init->p_addr = ctx;
                /* the switch frame: entry, stack top, no arg.  sp_top is
                 * the same expression rc_sp already uses at 213-214. */
                arch_context_init(&ctx->sc_switch,
                    (void (*)(void *))kinit_main,
                    (unsigned long)(ctx->sc_kstack
                                    + sizeof(ctx->sc_kstack)),
                    (void *)0);

/* adated during OS changes
* Why:
* sc_uarea is declared at context.h:73 — void *sc_uarea; /x u area location x/ — and 
* appears nowhere else in the tree. , both in the declaration. So it's a documented pointer 
* that is dead: nothing assigns it, nothing reads it.
* 
* u_proc is the U-area's first field and is zero today, because the memset(ctx, 0, sizeof(*ctx)) 
* on the line above clears the context, not the U-area. Setting it here is what makes 
* a pool entry self-identifying.
*/
                ctx->sc_uarea = uiox_kix_psa_uarea_for(slot);
                if (ctx->sc_uarea){
                    //((uiox_kix_psa_u_area_t *)ctx->sc_uarea)->u_proc = init;
                    /* pool storage, and uarea_for() is a pure index: a reused slot
                    * carries the previous process's handlers, uid, open-file table
                    * and regions.  Clear BEFORE the back-pointer, or it is wiped. */
                    memset(ctx->sc_uarea, 0, sizeof(uiox_kix_psa_u_area_t));
                    ((uiox_kix_psa_u_area_t *)ctx->sc_uarea)->u_proc = init;
                }
            }
            if (uiox_kix_psa_proc_set_state(init, UIOX_KIX_PSA_PROC_READY) == 0)
                uiox_kix_psa_sched_enqueue(init);
        }
    }
 
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

     /* added for tick fire due to context switch update*/
     uiox_clock_tick_hook = clock_tick;
 
     s_pcs_ready = true;
 }
 