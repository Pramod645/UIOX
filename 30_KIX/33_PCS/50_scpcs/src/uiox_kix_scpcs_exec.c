/*
 * 30_KIX/33_PCS/50_scpcs/src/uiox_kix_scpcs_exec.c
 *
 * Kernel side of the exec trap.
 *
 * Reached ONLY through SYS_EXECV (250) from userland: 20_uios calls
 * uix_execv(), which traps, and this runs in kernel mode.  Nothing in
 * 50_UIX links against it — that boundary is the point of the trap.
 *
 * Everything below is an existing call.  The region API (PSA algorithms
 * 3/4/6) does the mapping and the file read; this file only names the
 * addresses and the entry point.
 *
 * ── what this does NOT do yet ────────────────────────────────────────
 * argc/argv are ignored.  argv arrives as a userland pointer and there
 * is no copyin() in the tree, so the first version enters main() with
 * no arguments.  uiox_sh.c's main(void) takes none, which is why that
 * is safe — not because the arguments crossed the boundary.
 *
 * @version 1.0.0  @date 2026-11-12
 */
#include "uiox_kix_psa_region.h"
#include "uiox_kix_psa_context.h"
#include "uiox_kix_pagemap.h"
#include "namei.h"
#include "inode.h"

/* From `llvm-readelf -h -l build/arm64/sh`:
 *   entry 0x400000, ET_EXEC — no PT_INTERP, no relocations, no slide.
 *   text  0x400000  FileSiz 0x3320   MemSiz 0x3320   file off 0x010000
 *   data  0x413320  FileSiz 0x0180   MemSiz 0x8009a8 file off 0x013320
 * Data MemSiz exceeds FileSiz by 0x800828 — that is .bss, which readi()
 * never fills.  Frames come from the physical allocator already zeroed,
 * which is why the gap is safe. */
#define SH_ENTRY       0x400000UL
#define SH_TEXT_VADDR  0x400000UL
#define SH_TEXT_SIZE   0x3320U
#define SH_TEXT_OFF    0x010000U
#define SH_DATA_VADDR  0x413320UL
#define SH_DATA_SIZE   0x8009a8U
#define SH_DATA_OFF    0x013320U
#define SH_STACK_SIZE  0x010000U        /* 64 KB */

static uiox_kix_psa_sys_context_t uiox_sh_ctx;
static struct uiox_kix_psa_proc     uiox_sh_proc;   /* placeholder: no proc layer */

/* loadreg sets PTE_VALID|PTE_USER for every page and takes no flags
 * argument, so a data region comes out read-only and the shell faults on
 * its first store to a global.  Widening loadreg's signature is the
 * cleaner fix; this is two lines until then. */
static void mark_writable(uiox_kix_psa_region_t *rp)
{
    uint32_t i;
    if (!rp || !rp->r_pgtbl) return;
    for (i = 0u; i < rp->r_npages; i++)
        rp->r_pgtbl[i].pte_flags |= UIOX_KIX_PSA_PTE_WRITE;
}

int uiox_kix_scpcs_exec(const char *path, const char *const *argv)
{
    InCoreInode *root;
    InCoreInode *ip;
    uiox_kix_psa_region_t  *trp, *drp, *srp;
    uiox_kix_psa_pregion_t *tpr, *dpr, *spr;
    uintptr_t stack_top;

    (void)argv;   /* no copyin() yet — see the header note */

    if (!path) return -1;

    /* The walk needs a starting inode and the device it stays on. */
    root = iget(0u);                 /* ROOT_DEV */
    if (!root) return -1;

    ip = namei(path, root, 0u, 0u);
    if (!ip) return -1;

    /* ── text: R, executable, shared by design ────────────────── */
    trp = uiox_kix_psa_allocreg(ip, UIOX_KIX_PSA_REG_TEXT);
    if (!trp) { iput(ip); return -1; }
    tpr = uiox_kix_psa_attachreg(trp, &uiox_sh_proc, SH_TEXT_VADDR,
                                 UIOX_KIX_PSA_REG_TEXT);
    if (!tpr) { iput(ip); return -1; }
    uiox_kix_psa_loadreg(tpr, SH_TEXT_VADDR, ip, SH_TEXT_OFF, SH_TEXT_SIZE);

    /* ── data: RW.  REG_DATA is private after first write ─────── */
    drp = uiox_kix_psa_allocreg(ip, UIOX_KIX_PSA_REG_DATA);
    if (!drp) { iput(ip); return -1; }
    dpr = uiox_kix_psa_attachreg(drp, &uiox_sh_proc, SH_DATA_VADDR,
                                 UIOX_KIX_PSA_REG_DATA);
    if (!dpr) { iput(ip); return -1; }
    uiox_kix_psa_loadreg(dpr, SH_DATA_VADDR, ip, SH_DATA_OFF, SH_DATA_SIZE);
    mark_writable(drp);

    /* ── stack: kernel chooses the address, not userland ──────── */
    stack_top = 0x50000000UL;        /* userland high, below the kernel */
    srp = uiox_kix_psa_allocreg((struct inode *)0, UIOX_KIX_PSA_REG_STACK);
    if (!srp) { iput(ip); return -1; }
    spr = uiox_kix_psa_attachreg(srp, &uiox_sh_proc,
                                 stack_top - SH_STACK_SIZE,
                                 UIOX_KIX_PSA_REG_STACK);
    if (!spr) { iput(ip); return -1; }
    uiox_kix_psa_growreg(spr, (int32_t)SH_STACK_SIZE);
    mark_writable(srp);

    iput(ip);   /* namei's contract */

    /* ── the address map ──────────────────────────────────────── */
    uiox_sh_ctx.pc_user.uas_text_base  = SH_TEXT_VADDR;
    uiox_sh_ctx.pc_user.uas_text_size  = SH_TEXT_SIZE;
    uiox_sh_ctx.pc_user.uas_data_base  = SH_DATA_VADDR;
    uiox_sh_ctx.pc_user.uas_data_size  = SH_DATA_SIZE;      /* MemSiz */
    uiox_sh_ctx.pc_user.uas_stack_base = stack_top - SH_STACK_SIZE;
    uiox_sh_ctx.pc_user.uas_stack_size = SH_STACK_SIZE;

    /* ── the entry frame ──────────────────────────────────────── */
    uiox_sh_ctx.pc_regs.rc_pc    = SH_ENTRY;
    uiox_sh_ctx.pc_regs.rc_sp    = stack_top & ~0xFUL;
    uiox_sh_ctx.pc_regs.rc_carry = 0u;   /* or the first svc inherits an error */

    /* ── arm the switch ───────────────────────────────────────────
     * arch_context_init writes a KERNEL-mode status field: EL1h on
     * arm64, SPP=1 on riscv64, SVC mode on arm32, rflags-only on
     * x86_64.  Until each arch gets a user-mode variant AND its
     * context_switch ends with eret/sret/movs/iretq instead of ret,
     * this shell runs at EL1 — a prompt, no svc traps, and nothing in
     * the build to tell you which level you got. */
    arch_context_init(&uiox_sh_ctx.sc_switch, (void *)SH_ENTRY,
                      stack_top, (void *)0);

    return 0;   /* the caller switches into it */
}
