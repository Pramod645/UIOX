#include "../include/brk.h"


/* ── brk_check_legal ─────────────────────────────────────────── */
int brk_check_legal(uintptr_t new_brk, struct u_area *u)
{
    (void)u;
    if (new_brk < MIN_BRK_ADDR) {
        printf("[brk] ERROR: new break 0x%lx below minimum\n",
               (unsigned long)new_brk);
        return 0;
    }
    if (new_brk > MAX_BRK_ADDR) {
        printf("[brk] ERROR: new break 0x%lx exceeds maximum\n",
               (unsigned long)new_brk);
        return 0;
    }
    return 1;
}

/* ── get_current_brk ─────────────────────────────────────────── */
uintptr_t get_current_brk(struct u_area *u)
{
    (void)u;
    /* In a real kernel: walk the data region pregion to find the
       current heap top. Simulation: return a fixed base address. */
    return (uintptr_t)MIN_BRK_ADDR;
}

/* ── brk_zero_new_space ──────────────────────────────────────── */
void brk_zero_new_space(uintptr_t old_brk, uintptr_t new_brk)
{
    /* Zero new heap pages — in simulation we just log */
    printf("[brk] zeroing 0x%lx bytes [0x%lx, 0x%lx)\n",
           (unsigned long)(new_brk - old_brk),
           (unsigned long)old_brk,
           (unsigned long)new_brk);
}

/* ── Algorithm brk (Section 9) ───────────────────────────────── */
uintptr_t kernel_brk(uintptr_t new_brk)
{
    struct u_area *u = (struct u_area *)0;   /* &u in real kernel */

    uintptr_t old_brk = get_current_brk(u);

    /* Align new break to page boundary */
    new_brk = (new_brk + BRK_ALIGN - 1) & ~(uintptr_t)(BRK_ALIGN - 1);

    if (!brk_check_legal(new_brk, u)) {
        printf("[brk] ERROR: illegal break address 0x%lx\n",
               (unsigned long)new_brk);
        return (uintptr_t)-1;
    }

    if (new_brk == old_brk) {
        printf("[brk] no change, break=0x%lx\n", (unsigned long)old_brk);
        return old_brk;
    }

    if (new_brk > old_brk) {
        /* Expanding heap */
        printf("[brk] expanding heap 0x%lx -> 0x%lx (+%lu bytes)\n",
               (unsigned long)old_brk, (unsigned long)new_brk,
               (unsigned long)(new_brk - old_brk));
        brk_zero_new_space(old_brk, new_brk);
    } else {
        /* Shrinking heap */
        printf("[brk] shrinking heap 0x%lx -> 0x%lx (-%lu bytes)\n",
               (unsigned long)old_brk, (unsigned long)new_brk,
               (unsigned long)(old_brk - new_brk));
    }

    return old_brk;
}
