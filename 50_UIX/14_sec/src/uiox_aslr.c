/**
 * @file  uiox_aslr.c
 * @brief UIOX Security — ASLR engine.
 *
 * Address randomisation model:
 *   randomised_base = canonical_base
 *                   + (random_bits & mask) << PAGE_SHIFT
 *
 * where mask = (1 << entropy_bits) - 1.
 *
 * All addresses are page-aligned. The CSPRNG is seeded once at boot
 * from the hardware entropy source via uiox_sec_plat_random().
 *
 * @date  2026-07-08
 */
#include "../include/uiox_aslr.h"

extern void uiox_fw_printf(const char *fmt, ...);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void as_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

/* =========================================================================
 * Weak platform entropy defaults
 * ====================================================================== */

/* 32-bit xorshift PRNG — ONLY for bring-up / simulation.
 * Override uiox_sec_plat_random() with a real TRNG in production. */
static uint32_t s_lfsr_state = 0xDEADBEEFu;

static uint32_t lfsr_next(void)
{
    s_lfsr_state ^= s_lfsr_state << 13;
    s_lfsr_state ^= s_lfsr_state >> 17;
    s_lfsr_state ^= s_lfsr_state <<  5;
    return s_lfsr_state;
}

__attribute__((weak))
void uiox_sec_plat_random(uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; ) {
        uint32_t r = lfsr_next();
        size_t chunk = (len - i < 4u) ? (len - i) : 4u;
        for (size_t j = 0; j < chunk; j++, i++)
            buf[i] = (uint8_t)(r >> (j * 8u));
    }
}

__attribute__((weak))
uint64_t uiox_sec_plat_time_ms(void) { return 0u; }

/* =========================================================================
 * Internal: read @bits of entropy as a uint64_t
 * ====================================================================== */
static uint64_t random_bits(uint8_t bits)
{
    if (bits == 0u) return 0u;
    if (bits > 63u) bits = 63u;

    uint8_t buf[8];
    uiox_sec_plat_random(buf, 8u);

    uint64_t v = 0u;
    for (int i = 0; i < 8; i++)
        v |= ((uint64_t)buf[i]) << (i * 8u);

    return v & ((1ull << bits) - 1ull);
}

/* =========================================================================
 * Default canonical bases (ARM64 user-space layout)
 * ====================================================================== */
#define BASE_EXEC   0x0000000000400000ull  /* Default non-PIE load addr     */
#define BASE_STACK  0x0000007FFFFFFF0000ull/* Top of user stack             */
#define BASE_HEAP   0x0000000010000000ull  /* brk() start hint              */
#define BASE_MMAP   0x0000000040000000ull  /* mmap() region start           */
#define BASE_VDSO   0x0000000000010000ull  /* vDSO hint                     */
#define BASE_KSTACK 0xFFFF800000000000ull  /* Kernel stack pool base        */

/* =========================================================================
 * Init
 * ====================================================================== */
uiox_sec_err_t uiox_aslr_init(uiox_aslr_ctx_t *ctx, uint8_t level)
{
    if (!ctx) return UIOX_SEC_ERR_INVAL;

    as_memset(ctx, 0, sizeof(*ctx));
    ctx->level = level;

    /* Set default entropy bits per region */
    ctx->entropy_bits[UIOX_ASLR_REGION_EXEC]   =
        (level >= UIOX_ASLR_LEVEL_FULL)    ? UIOX_ASLR_EXEC_BITS    : 0u;
    ctx->entropy_bits[UIOX_ASLR_REGION_STACK]  = UIOX_ASLR_STACK_BITS;
    ctx->entropy_bits[UIOX_ASLR_REGION_HEAP]   =
        (level >= UIOX_ASLR_LEVEL_PARTIAL) ? UIOX_ASLR_HEAP_BITS    : 0u;
    ctx->entropy_bits[UIOX_ASLR_REGION_MMAP]   =
        (level >= UIOX_ASLR_LEVEL_FULL)    ? UIOX_ASLR_MMAP_BITS    : 0u;
    ctx->entropy_bits[UIOX_ASLR_REGION_VDSO]   =
        (level >= UIOX_ASLR_LEVEL_FULL)    ? UIOX_ASLR_VDSO_BITS    : 0u;
    ctx->entropy_bits[UIOX_ASLR_REGION_KSTACK] =
        (level >= UIOX_ASLR_LEVEL_KSTACK)  ? UIOX_ASLR_KSTACK_BITS  : 0u;

    /* Seed PRNG from hardware entropy */
    uint8_t seed[8];
    uiox_sec_plat_random(seed, 8u);
    for (int i = 0; i < 4; i++)
        s_lfsr_state ^= ((uint32_t)seed[i] << (i * 8u));

    ctx->initialized = true;
    uiox_fw_printf("[aslr] init: level=%u\n", level);
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Randomise all regions for a new process mm
 * ====================================================================== */
uiox_sec_err_t uiox_aslr_randomise_mm(uiox_aslr_ctx_t *ctx,
                                        uiox_aslr_mm_t  *mm,
                                        bool             is_pie,
                                        bool             is_compat)
{
    if (!ctx || !mm || !ctx->initialized) return UIOX_SEC_ERR_INVAL;

    as_memset(mm, 0, sizeof(*mm));
    ctx->compat32 = is_compat;

    /* 32-bit compat: narrower address space */
    uint64_t stack_max  = is_compat ? 0x00000000BFFF0000ull : BASE_STACK;
    uint64_t mmap_start = is_compat ? 0x0000000040000000ull : BASE_MMAP;
    uint64_t heap_start = is_compat ? 0x0000000008000000ull : BASE_HEAP;

    /* Helper macro: apply entropy to a base address */
#define RANDOMISE(base, region_id) \
    ((base) + (random_bits(ctx->entropy_bits[(region_id)]) \
               << UIOX_ASLR_PAGE_SHIFT))

    /* EXEC — only randomised for PIE binaries */
    uint64_t exec_base = is_pie
        ? RANDOMISE(0x0000000000100000ull, UIOX_ASLR_REGION_EXEC)
        : BASE_EXEC;

    mm->exec_base   = exec_base;
    mm->stack_base  = RANDOMISE(stack_max,  UIOX_ASLR_REGION_STACK);
    mm->heap_base   = RANDOMISE(heap_start, UIOX_ASLR_REGION_HEAP);
    mm->mmap_base   = RANDOMISE(mmap_start, UIOX_ASLR_REGION_MMAP);
    mm->vdso_base   = RANDOMISE(BASE_VDSO,  UIOX_ASLR_REGION_VDSO);
    mm->kstack_base = 0u; /* set separately by uiox_aslr_kstack() */

#undef RANDOMISE

    /* Fill per-region descriptors */
    static const uint64_t bases[UIOX_ASLR_REGION__COUNT] = {
        BASE_EXEC, BASE_STACK, BASE_HEAP, BASE_MMAP, BASE_VDSO, BASE_KSTACK
    };
    static const uint64_t *finals[UIOX_ASLR_REGION__COUNT];
    /* Point at the results we just computed */
    uint64_t *result_ptrs[UIOX_ASLR_REGION__COUNT] = {
        &mm->exec_base, &mm->stack_base, &mm->heap_base,
        &mm->mmap_base, &mm->vdso_base,  &mm->kstack_base
    };

    for (uint32_t i = 0; i < UIOX_ASLR_REGION__COUNT; i++) {
        mm->regions[i].region        = (uiox_aslr_region_t)i;
        mm->regions[i].entropy_bits  = ctx->entropy_bits[i];
        mm->regions[i].base_hint     = bases[i];
        mm->regions[i].randomised    = *result_ptrs[i];
        mm->regions[i].enabled       = (ctx->entropy_bits[i] > 0u);
    }
    (void)finals;

    mm->generation++;
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Kernel stack randomisation
 * ====================================================================== */
uint64_t uiox_aslr_kstack(const uiox_aslr_ctx_t *ctx,
                            uint64_t               stack_area,
                            size_t                 stack_size)
{
    if (!ctx || !ctx->initialized) return stack_area;

    uint8_t bits = ctx->entropy_bits[UIOX_ASLR_REGION_KSTACK];
    if (bits == 0u) return stack_area;

    uint64_t offset = random_bits(bits) << UIOX_ASLR_PAGE_SHIFT;
    /* Clamp within the allocated stack area, leaving room for the stack */
    if (offset + 0x1000u > (uint64_t)stack_size)
        offset = 0u;

    return stack_area + offset;
}

/* =========================================================================
 * Per-mmap() hint randomisation (prevents heap-spray)
 * ====================================================================== */
uint64_t uiox_aslr_mmap_hint(const uiox_aslr_ctx_t *ctx,
                               uint64_t               preferred)
{
    if (!ctx || ctx->level < UIOX_ASLR_LEVEL_FULL) return preferred;

    uint64_t offset = random_bits(UIOX_ASLR_MMAP_BITS) << UIOX_ASLR_PAGE_SHIFT;
    /* Stay in the mmap region; wrap at 512 GiB above preferred */
    return preferred + (offset & 0x0000007FFFFFFFFFull);
}

/* =========================================================================
 * Set entropy for one region (sysctl interface)
 * ====================================================================== */
uiox_sec_err_t uiox_aslr_set_entropy(uiox_aslr_ctx_t   *ctx,
                                       uiox_aslr_region_t region,
                                       uint8_t            bits)
{
    if (!ctx || region >= UIOX_ASLR_REGION__COUNT) return UIOX_SEC_ERR_INVAL;
    if (bits < UIOX_ASLR_ENTROPY_MIN_BITS) bits = UIOX_ASLR_ENTROPY_MIN_BITS;
    if (bits > UIOX_ASLR_ENTROPY_MAX_BITS) bits = UIOX_ASLR_ENTROPY_MAX_BITS;

    ctx->entropy_bits[region] = bits;
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Print
 * ====================================================================== */
static const char *region_names[UIOX_ASLR_REGION__COUNT] = {
    "exec", "stack", "heap", "mmap", "vdso", "kstack"
};

void uiox_aslr_print(const uiox_aslr_ctx_t *ctx,
                      const uiox_aslr_mm_t  *mm)
{
    if (!ctx) return;
    uiox_fw_printf("[aslr] level=%u  compat32=%s\n",
                   ctx->level, ctx->compat32 ? "YES" : "NO");
    uiox_fw_printf("  %-8s  %5s  %s\n", "REGION", "BITS", "RANDOMISED BASE");
    for (uint32_t i = 0; i < UIOX_ASLR_REGION__COUNT; i++) {
        uiox_fw_printf("  %-8s  %5u  ", region_names[i],
                       ctx->entropy_bits[i]);
        if (mm)
            uiox_fw_printf("0x%016llx\n",
                           (unsigned long long)mm->regions[i].randomised);
        else
            uiox_fw_printf("(no mm)\n");
    }
}
