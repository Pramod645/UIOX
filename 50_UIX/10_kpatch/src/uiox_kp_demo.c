/**
 * @file  uiox_kp_demo.c
 * @brief UIOX Live Kernel Patching — demo exercising all kpatch APIs.
 * @date  2026-07-07
 */

 #include "../include/uiox_kpatch.h"

 /* =========================================================================
  * Simulated kernel functions to be patched
  * ====================================================================== */
 
 /* Original: buggy scheduler that always returns priority 0 */
 static int __attribute__((noinline)) uiox_sched_get_prio(int pid)
 {
     (void)pid;
     return 0;   /* BUG: always returns 0 */
 }
 
 /* Original: memory allocator that returns NULL under pressure */
 static void * __attribute__((noinline)) uiox_kmalloc_buggy(size_t size)
 {
     (void)size;
     return NULL;  /* BUG: always fails */
 }
 
 /* Trampoline pointer — new_func calls original via this */
 static int   (*kp_orig_sched_get_prio)(int pid)  = NULL;
 static void *(*kp_orig_kmalloc_buggy) (size_t sz) = NULL;
 
 /* =========================================================================
  * Replacement functions (the patches)
  * ====================================================================== */
 
 /* Fixed scheduler: returns pid % 10 as priority */
 static int kp_new_sched_get_prio(int pid)
 {
     /* Optionally call original via trampoline */
     int orig_val = kp_orig_sched_get_prio ? kp_orig_sched_get_prio(pid) : 0;
     (void)orig_val;
     return pid % 10;  /* FIX: proper priority */
 }
 
 /* Fixed allocator: returns a static buffer */
 static uint8_t s_fake_heap[4096];
 static size_t  s_fake_heap_ptr = 0u;
 
 static void *kp_new_kmalloc_buggy(size_t size)
 {
     /* Optionally call original */
     if (kp_orig_kmalloc_buggy) kp_orig_kmalloc_buggy(size);
     /* FIX: serve from static heap */
     if (s_fake_heap_ptr + size > sizeof(s_fake_heap)) return NULL;
     void *ptr = &s_fake_heap[s_fake_heap_ptr];
     s_fake_heap_ptr += size;
     return ptr;
 }
 
 /* =========================================================================
  * Patch module definition
  * ====================================================================== */
 
 static uiox_kp_module_t s_fix_module = {
     .name        = "uiox-fix-v1",
     .version     = 1u,
     .num_patches = 2u,
     .patches = {
         UIOX_KP_PATCH("sched_get_prio",
                        uiox_sched_get_prio,
                        kp_new_sched_get_prio),
         UIOX_KP_PATCH("kmalloc_buggy",
                        uiox_kmalloc_buggy,
                        kp_new_kmalloc_buggy),
     },
     .loaded = false,
 };
 
 /* =========================================================================
  * External: firmware printf
  * ====================================================================== */
 
 extern void uiox_fw_printf(const char *fmt, ...);
 
 /* =========================================================================
  * Demo main
  * ====================================================================== */
 
 void uiox_kp_demo(void)
 {
     uiox_fw_printf("\n=== UIOX Live Kernel Patching Demo ===\n\n");
 
     /* ── Step 1: Engine init ──────────────────────────────── */
     uiox_fw_printf("--- Step 1: Engine init ---\n");
     uiox_kp_err_t rc = uiox_kp_engine_init();
     uiox_fw_printf("  engine_init: %s\n", uiox_kp_err_str(rc));
 
     /* ── Step 2: Verify original (buggy) behaviour ────────── */
     uiox_fw_printf("\n--- Step 2: Original (buggy) behaviour ---\n");
     int prio_before = uiox_sched_get_prio(42);
     void *ptr_before = uiox_kmalloc_buggy(128u);
     uiox_fw_printf("  sched_get_prio(42) = %d  (expected 0 — buggy)\n",
                     prio_before);
     uiox_fw_printf("  kmalloc_buggy(128) = %p  (expected NULL — buggy)\n",
                     ptr_before);
 
     /* ── Step 3: Load patch module ────────────────────────── */
     uiox_fw_printf("\n--- Step 3: Load patch module ---\n");
 
     /* Wire up trampoline pointers so new_func can call orig */
     kp_orig_sched_get_prio = (int (*)(int))(
         s_fix_module.patches[0].trampoline ?
         s_fix_module.patches[0].trampoline :
         (uintptr_t)uiox_sched_get_prio);
     kp_orig_kmalloc_buggy = (void *(*)(size_t))(
         s_fix_module.patches[1].trampoline ?
         s_fix_module.patches[1].trampoline :
         (uintptr_t)uiox_kmalloc_buggy);
 
     rc = uiox_kp_module_load(&s_fix_module);
     uiox_fw_printf("  module_load: %s\n", uiox_kp_err_str(rc));
 
     /* Update trampoline pointers now they are set */
     if (s_fix_module.patches[0].trampoline)
         kp_orig_sched_get_prio = (int(*)(int))
                                   s_fix_module.patches[0].trampoline;
     if (s_fix_module.patches[1].trampoline)
         kp_orig_kmalloc_buggy  = (void*(*)(size_t))
                                   s_fix_module.patches[1].trampoline;
 
     /* ── Step 4: Print patch table ────────────────────────── */
     uiox_fw_printf("\n--- Step 4: Patch table ---\n");
     uiox_kp_print_table();
 
     /* ── Step 5: Verify patched behaviour ─────────────────── */
     uiox_fw_printf("\n--- Step 5: Patched behaviour ---\n");
     int prio_after  = uiox_sched_get_prio(42);
     void *ptr_after = uiox_kmalloc_buggy(128u);
     uiox_fw_printf("  sched_get_prio(42) = %d   (expected 2 — fixed)\n",
                     prio_after);
     uiox_fw_printf("  kmalloc_buggy(128) = %p   (non-NULL — fixed)\n",
                     ptr_after);
 
     /* ── Step 6: Query by name ─────────────────────────────── */
     uiox_fw_printf("\n--- Step 6: Query by name ---\n");
     uiox_kp_patch_t *p = uiox_kp_find_by_name("sched_get_prio");
     if (p)
         uiox_fw_printf("  found '%s'  state=%s  calls=%u\n",
                         p->name,
                         uiox_kp_state_name(p->state),
                         p->call_count);
 
     /* ── Step 7: Disable a patch individually ──────────────── */
     uiox_fw_printf("\n--- Step 7: Disable sched_get_prio patch ---\n");
     if (p) {
         rc = uiox_kp_disable(p);
         uiox_fw_printf("  disable: %s\n", uiox_kp_err_str(rc));
         int prio_dis = uiox_sched_get_prio(42);
         uiox_fw_printf("  sched_get_prio(42) = %d  (back to buggy 0)\n",
                         prio_dis);
         /* Re-enable */
         rc = uiox_kp_enable(p);
         uiox_fw_printf("  re-enable: %s\n", uiox_kp_err_str(rc));
     }
 
     /* ── Step 8: Memory pool status ───────────────────────── */
     uiox_fw_printf("\n--- Step 8: Memory pool ---\n");
     uiox_kp_mem_print();
 
     /* ── Step 9: Syscall interface ─────────────────────────── */
     uiox_fw_printf("\n--- Step 9: Syscall interface ---\n");
     uiox_kp_state_t st = UIOX_KP_STATE_UNREGISTERED;
     long sret = sys_kpatch_status((long)"sched_get_prio", (long)&st, 0, 0);
     uiox_fw_printf("  sys_kpatch_status('sched_get_prio') = %s  state=%s\n",
                     uiox_kp_err_str((uiox_kp_err_t)sret),
                     uiox_kp_state_name(st));
 
     /* ── Step 10: Unload patch module ──────────────────────── */
     uiox_fw_printf("\n--- Step 10: Unload patch module ---\n");
     rc = uiox_kp_module_unload(&s_fix_module);
     uiox_fw_printf("  module_unload: %s\n", uiox_kp_err_str(rc));
 
     int prio_final = uiox_sched_get_prio(42);
     uiox_fw_printf("  sched_get_prio(42) after unload = %d  (0 — original)\n",
                     prio_final);
 
     /* ── Step 11: Engine deinit ────────────────────────────── */
     uiox_fw_printf("\n--- Step 11: Engine deinit ---\n");
     uiox_kp_engine_deinit();
     uiox_fw_printf("  engine deinit OK\n");
 
     uiox_fw_printf("\n=== UIOX kpatch Demo complete ===\n");
 }
 