#include "../include/brk.h"
#include <string.h>
#include <stdio.h>

/* ── Simulated current break value ──────────────────────────── */
static uintptr_t current_brk = 0x500000;  /* initial break      */

/* ── get_current_brk ─────────────────────────────────────────
 * Return the current break value from the process data region.
 */
uintptr_t get_current_brk(struct u_area *u)
{
    (void)u;
    return current_brk;
}

/* ── brk_check_legal ─────────────────────────────────────────
 * Verify the new break value is within legal bounds.
 */
int brk_check_legal(uintptr_t new_brk, struct u_area *u)
{
    (void)u;
    if (new_brk < MIN_BRK_ADDR) {
        fprintf(stderr, "[brk] new break 0x%lx below minimum\n",
                (unsigned long)new_brk);
        return 0;
    }
    if (new_brk > MAX_BRK_ADDR) {
        fprintf(stderr, "[brk] new break 0x%lx exceeds maximum\n",
                (unsigned long)new_brk);
        return 0;
    }
    return 1;
}

/* ── brk_zero_new_space ──────────────────────────────────────
 * Zero-fill newly allocated data space between old and new brk.
 */
void brk_zero_new_space(uintptr_t old_brk, uintptr_t new_brk)
{
    if (new_brk <= old_brk) return;
    printf("[brk] zeroing 0x%lx bytes of new data space\n",
           (unsigned long)(new_brk - old_brk));
    /* In real kernel: memset((void*)old_brk, 0,
     *                        new_brk - old_brk);             */
}

/* ─────────────────────────────────────────────────────────────
 * 9. Algorithm brk
 *    input : new break (heap top) virtual address
 *    output: old break value; (uintptr_t)-1 on error
 */
uintptr_t kernel_brk(uintptr_t new_brk)
{
    struct u_area *u = NULL;  /* &u in real kernel             */

    uintptr_t old_brk = get_current_brk(u);
    printf("[brk] old_brk=0x%lx  new_brk=0x%lx\n",
           (unsigned long)old_brk, (unsigned long)new_brk);

    /* Align new_brk to page boundary */
    new_brk = (new_brk + BRK_ALIGN - 1) & ~(uintptr_t)(BRK_ALIGN - 1);

    /* ── Lock process data region ──────────────────────────── */
    printf("[brk] locking data region\n");
    /* region_lock(data_region); */

    if (new_brk > old_brk) {
        /* ── Region size increasing ────────────────────────── */
        if (!brk_check_legal(new_brk, u)) {
            /* Unlock data region */
            printf("[brk] unlocking data region (error)\n");
            /* region_unlock(data_region); */
            return (uintptr_t)-1;
        }
    }

    /* ── Change region size (algorithm growreg) ────────────── */
    int32_t delta = (int32_t)(new_brk - old_brk);
    printf("[brk] calling growreg delta=%d\n", delta);
    /* growreg(data_pregion, delta); */

    /* ── Zero out addresses in new data space (if growing) ─── */
    if (new_brk > old_brk)
        brk_zero_new_space(old_brk, new_brk);

    /* Update current break */
    current_brk = new_brk;

    /* ── Unlock process data region ────────────────────────── */
    printf("[brk] unlocking data region\n");
    /* region_unlock(data_region); */

    printf("[brk] brk complete: old=0x%lx new=0x%lx\n",
           (unsigned long)old_brk, (unsigned long)new_brk);
    return old_brk;
}
