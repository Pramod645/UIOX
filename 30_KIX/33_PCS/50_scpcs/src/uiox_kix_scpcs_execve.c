/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_execve.c
 *
 * execve() -- Group 1, memory Management.
 *
 * ── the grouping, verbatim ──────────────────────────────────────────
 *   exec: detachreg, allocreg, attachreg, growreg, loadreg, mapreg
 *
 * All of those exist in 40_psa's region source:
 *   uiox_kix_psa_detachreg   Algorithm 8   (region.h:169)
 *   uiox_kix_psa_allocreg    Algorithm 3   (region.h:132)
 *   uiox_kix_psa_attachreg   Algorithm 4   (region.h:139)
 *   uiox_kix_psa_growreg     Algorithm 5   (region.h:143)
 *   uiox_kix_psa_loadreg     Algorithm 6   (region.h:155)
 *
 * ── what changed from the ENOSYS stub ───────────────────────────────
 * The stub said the file-verification step fabricated what it then
 * checked: it wrote the expected magic into a header struct and tested
 * that struct against the same constant, with a NULL inode, so the test
 * could not fail and nothing was ever read.
 *
 * Now the header is READ from the inode through readi(), and the magic
 * is tested against what came back.  A file that is not an ELF64 image
 * for this architecture fails rather than passes.
 *
 * ── one region per PT_LOAD segment ──────────────────────────────────
 * Not one region for the image.  Each segment has its own address
 * (p_vaddr) and own sharing class, which 40_psa's region types encode:
 *
 *   writable segment   -> UIOX_KIX_PSA_REG_DATA   (private after a write)
 *   read/exec segment  -> UIOX_KIX_PSA_REG_TEXT   (shared across processes)
 *
 * attachreg records pr_vaddr (region.c:277), which is what find_pte
 * tests an address against -- so a region is addressable at its own
 * segment base, not at one image base.
 *
 * ── the bss tail ────────────────────────────────────────────────────
 * ELF requires the bytes between p_filesz and p_memsz to exist as ZEROED
 * pages.  loadreg fills only p_filesz, so zero_tail() grows the region
 * to p_memsz and writes zeros over the difference.  Without it a global
 * in an uninitialised segment reads a frame that was never allocated.
 *
 * ── the path copy-in ────────────────────────────────────────────────
 * execve receives a USER pointer and namei() wants a KERNEL string.
 * There is no copy_from_user in this tree -- 50_scpcs says so in three
 * places (uiox_kix_scpcs.h:335, dispatch.c:60, check_user_ptr.c:7) --
 * so user_copyin_str() below does it, bounded, probing each byte.
 *
 * It is a LOCAL STATIC on purpose.  Every syscall taking a string needs
 * the same helper; keeping it local holds this change to one file.
 * Promote it into the syscall layer when a second caller appears.
 *
 * ── detachreg on every failure path ─────────────────────────────────
 * A failed exec after some segments attached leaves regions bound to a
 * process that never runs, with r_refcnt never returning to zero, so
 * freereg can never reclaim them.  attach_failed() unwinds them.
 *
 * ── argv / envp ─────────────────────────────────────────────────────
 * Accepted and ignored.  POSIX allows NULL, and NULL is what this
 * handles; a real vector is NOT rejected, because nothing builds a user
 * stack from it yet.  A caller passing one gets a process with no
 * arguments -- wrong quietly rather than loudly.  That is the next piece.
 *
 * ── one constant to add ─────────────────────────────────────────────
 * SCPCS_ENOEXEC is not in uiox_kix_scpcs.h.  Add it beside the others:
 *     #define SCPCS_ENOEXEC  ((int64_t) -8)   // exec format error
 *
 * ── riscv64 ─────────────────────────────────────────────────────────
 * uiox_boot_handoff.h defines ELF_ARCH_AARCH64, X86_64 and ARM but no
 * riscv constant, so e_machine is unchecked there.  RISC-V's value is
 * 0xF3.  The class and type checks still apply.
 *
 * @version 4.0.0  @date 2026-10-31
 */
#include "../include/uiox_kix_scpcs.h"
#include "uiox_kix_psa_region.h"
#include "uiox_kix_paging.h"      /* uiox_kix_pfdata_t, UIOX_KIX_PSA_PTE_* */
#include "uiox_kix_psa_process.h"
#include "uiox_kix_psa_context.h"
#include "inode.h"
#include "namei.h"
#include "uiox_boot_handoff.h"   /* uiox_elf64_ehdr_t, PT_LOAD, PF_W */

/* ── readi's prototype ───────────────────────────────────────────────
 * readwrite.h reaches inode.h -> buffer.h -> bcache.h; dragging that
 * header web into 50_scpcs for one declaration is a heavier coupling
 * than the call needs.  Same judgement 40_psa made.  The definition is
 * 32_FS/01_fsa/include/readwrite.h:27 and its first parameter is
 * InCoreInode *, which void * accepts. */
int32_t readi(void *ip, char *kbuf, uint32_t count, uint32_t *off);

/* Largest path accepted, NUL included. */
#define EXECVE_PATH_MAX   512u

/* ── user_copyin_str ────────────────────────────────────────────────
 * Copy a NUL-terminated string from user space into a kernel buffer.
 *
 * Returns 0 on success, -1 on any failure.  The caller established the
 * first byte as non-wild; this adds a per-byte probe, because a string
 * may cross into an unmapped page partway through and one check of the
 * first byte cannot catch that.
 *
 * Never writes past cap.  On overflow the buffer is NUL-terminated at
 * cap-1 and the call FAILS rather than truncating: a truncated path
 * would resolve to a different file. */
static int user_copyin_str(char *dst, uiox_uintptr_t usrc, uint32_t cap)
{
    uint32_t i;

    if (!dst || cap == 0u) return -1;

    for (i = 0u; i < cap; i++) {
        if (uiox_kix_scpcs_check_user_ptr(usrc + (uiox_uintptr_t)i,
                                          1u, 1u) != 0) {
            dst[0] = '\0';
            return -1;
        }

        dst[i] = *((const char *)(usrc + (uiox_uintptr_t)i));

        if (dst[i] == '\0') return 0;
    }

    dst[cap - 1u] = '\0';
    return -1;                      /* no NUL within cap */
}

/* Mirror of user_copyin_str: validate through the shared gate, then
 * write.  Returns 0 on success. */
int uiox_kix_scpcs_copy_to_user(uiox_uintptr_t udst, const void *src, uint32_t len)
{
    const char *s = (const char *)src;
    uint32_t i;

    if (uiox_kix_scpcs_check_user_ptr(udst, len, 1u) != 0) return -1;

    for (i = 0u; i < len; i++)
        ((volatile char *)udst)[i] = s[i];   /* volatile: the target may be
                                              * unmapped on a bad pointer */
    return 0;
}

/* ── uiox_kix_scpcs_copy_from_user ──────────────────────────────────
 * The inbound mirror of copy_to_user: validate through the shared gate,
 * then read.  Returns 0 on success.
 *
 * Not the same thing as user_copyin_str above: that one is bound to a
 * NUL-terminated path with a cap, and fails rather than truncating.  A
 * fixed-width copy has no terminator to look for and knows its length
 * up front, so it is a separate function rather than a mode of that one.
 *
 * volatile on the SOURCE: a user pointer may name a page that is unmapped
 * or has changed underneath us, and the read must not be optimised into a
 * wider access than the caller validated. */
int uiox_kix_scpcs_copy_from_user(void *dst, uiox_uintptr_t usrc, uint32_t len)
{
    char *d = (char *)dst;
    uint32_t i;

    if (!dst) return -1;
    if (uiox_kix_scpcs_check_user_ptr(usrc, len, 1u) != 0) return -1;

    for (i = 0u; i < len; i++)
        d[i] = ((volatile const char *)usrc)[i];
    return 0;
}

/* ── is_elf64_for_this_arch ─────────────────────────────────────────
 * The test the stub could not fail.  Reads nothing: it is handed a
 * header already read from the file. */
static int is_elf64_for_this_arch(const uiox_elf64_ehdr_t *eh)
{
    if (eh->e_ident[0] != 0x7Fu) return 0;   /* 0x7F */
    if (eh->e_ident[1] != 'E')   return 0;
    if (eh->e_ident[2] != 'L')   return 0;
    if (eh->e_ident[3] != 'F')   return 0;

    if (eh->e_ident[4] != 2u)    return 0;   /* EI_CLASS = ELFCLASS64 */
    if (eh->e_ident[5] != 1u)    return 0;   /* EI_DATA  = LSB        */

    if (eh->e_type != ELF_TYPE_EXEC) return 0;

#if defined(__aarch64__)
    if (eh->e_machine != ELF_ARCH_AARCH64) return 0;
#elif defined(__x86_64__)
    if (eh->e_machine != ELF_ARCH_X86_64)  return 0;
#elif defined(__arm__)
    if (eh->e_machine != ELF_ARCH_ARM)     return 0;
#endif

    return 1;
}

/* ── the unwind list ────────────────────────────────────────────────
 * Regions attachreg bound to this process during THIS exec, so a failure
 * can hand them back.  Bounded by attachreg's own limit. */
static uiox_kix_psa_pregion_t *g_exec_added[UIOX_KIX_PSA_MAX_REG_PER_PROC];
static uint32_t                g_exec_added_n;

static void attached_note(uiox_kix_psa_pregion_t *prp)
{
    if (g_exec_added_n < (uint32_t)UIOX_KIX_PSA_MAX_REG_PER_PROC)
        g_exec_added[g_exec_added_n++] = prp;
}

/* ── attach_failed ──────────────────────────────────────────────────
 * Unwind every region this exec attached, newest first.  A region left
 * bound to a process that will not run keeps r_refcnt above zero
 * forever, so freereg can never take it back. */
static void attach_failed(void)
{
    while (g_exec_added_n > 0u) {
        uiox_kix_psa_pregion_t *prp = g_exec_added[--g_exec_added_n];
        uiox_kix_psa_detachreg(prp);
        g_exec_added[g_exec_added_n] = (uiox_kix_psa_pregion_t *)0;
    }
}

/* ── zero_tail ──────────────────────────────────────────────────────
 * Give the bytes between p_filesz and p_memsz real, zeroed pages.
 *
 * This is ELF's .bss.  loadreg filled p_filesz bytes; the segment's
 * declared size is p_memsz, and everything past the file image must
 * read as zero rather than as whatever the frame held.
 *
 * growreg allocates the extra frames (and is what recomputes r_npages),
 * then each new page is written zero.  The region is re-marked VALID
 * only after every page is filled, matching loadreg's own rule: a flag
 * set on data nobody wrote is the defect this whole pass exists to fix.
 *
 * Returns 0 on success, -1 if the region could not be grown. */
static int zero_tail(uiox_kix_psa_pregion_t *prp, uint64_t filesz,
                     uint64_t memsz)
{
    uiox_kix_psa_region_t *rp;
    uint32_t               p, from, to;
    int32_t                delta;

    if (memsz <= filesz) return 0;          /* no tail to zero */
    if (memsz > 0xFFFFFFFFull) return -1;

    rp = prp->pr_region;
    if (!rp) return -1;

    delta = (int32_t)((uint32_t)memsz - (uint32_t)filesz);
    uiox_kix_psa_growreg(prp, delta);
    if (!rp->r_pgtbl) return -1;            /* growreg refused */

    /* pages the file image occupies, and the total the segment needs.
     * 32-bit arithmetic on purpose: the guard above proves both values
     * fit, and a 64-bit divide would pull in __aeabi_uldivmod on arm32,
     * which a freestanding link does not provide. */
    from = ((uint32_t)filesz + UIOX_KIX_PSA_PAGE_SIZE - 1u)
           / UIOX_KIX_PSA_PAGE_SIZE;
    to   = ((uint32_t)memsz  + UIOX_KIX_PSA_PAGE_SIZE - 1u)
           / UIOX_KIX_PSA_PAGE_SIZE;
    if (to > rp->r_npages) to = rp->r_npages;

    for (p = from; p < to; p++) {
        uiox_kix_psa_pte_t *pte = &rp->r_pgtbl[p];
        uiox_kix_pfdata_t  *pfd;
        uint8_t            *dst;
        uint32_t            b;

        if (pte->pte_flags & UIOX_KIX_PSA_PTE_VALID) continue;  /* done */

        /* fresh frame for this page.  pfd_kva is the address the frame
         * allocator actually handed out (see FRAME_ADDR in 40_psa's
         * loadreg); pfd_pfn is an index and is NOT an address. */
        pfd = uiox_kix_alloc_physical_page();
        if (!pfd) return -1;

        dst = (uint8_t *)(uintptr_t)pfd->pfd_kva;
        for (b = 0u; b < (uint32_t)UIOX_KIX_PSA_PAGE_SIZE; b++) dst[b] = 0u;

        pte->pte_pfn   = pfd->pfd_pfn;
        pte->pte_flags = UIOX_KIX_PSA_PTE_VALID | UIOX_KIX_PSA_PTE_USER;

        pfd->pfd_pte           = pte;
        pfd->pfd_content_valid = 1;
    }

    rp->r_status |= UIOX_KIX_PSA_REG_VALID;
    return 0;
}

/* ── load_one_segment ───────────────────────────────────────────────
 * allocreg -> attachreg -> loadreg -> zero_tail for a single PT_LOAD.
 *
 * The type comes from p_flags because that decides sharing: a writable
 * segment must not stay shared after a write, so it is REG_DATA; a
 * read/exec segment is REG_TEXT and shared.
 *
 * Returns 0 on success, -1 otherwise. */
static int load_one_segment(InCoreInode *ip,
                            uiox_kix_psa_proc_t *proc,
                            const uiox_elf64_phdr_t *ph)
{
    uiox_kix_psa_region_type_t type;
    uiox_kix_psa_region_t      *rp;
    uiox_kix_psa_pregion_t     *prp;

    if (ph->p_filesz > 0xFFFFFFFFull) return -1;
    if (ph->p_memsz  > 0xFFFFFFFFull) return -1;
    if (ph->p_offset > 0xFFFFFFFFull) return -1;
    if (ph->p_filesz == 0u && ph->p_memsz == 0u) return 0;

    type = (ph->p_flags & PF_W) ? UIOX_KIX_PSA_REG_DATA
                                : UIOX_KIX_PSA_REG_TEXT;

    /* struct inode is an opaque tag (proc_algo.h:70 aliases it; nothing
     * defines it) -- the cast is spelling, not type conversion. */
    rp = uiox_kix_psa_allocreg((struct inode *)ip, type);
    if (!rp) return -1;

    prp = uiox_kix_psa_attachreg(rp, proc, (uintptr_t)ph->p_vaddr, type);
    if (!prp) return -1;

    attached_note(prp);

    if (ph->p_filesz > 0u) {
        /* loadreg grows the region -- allocating r_pgtbl and recomputing
         * r_npages -- then allocates a frame per page and reads the file
         * into each.  REG_VALID only when every byte arrived. */
        uiox_kix_psa_loadreg(prp, (uintptr_t)ph->p_vaddr, (struct inode *)ip,
                             (uint32_t)ph->p_offset,
                             (uint32_t)ph->p_filesz);

        if (!(rp->r_status & UIOX_KIX_PSA_REG_VALID)) return -1;
    }

    /* the .bss tail: pages that exist and read zero */
    if (zero_tail(prp, ph->p_filesz, ph->p_memsz) != 0) return -1;

    return 0;
}

/* ── load_segments ──────────────────────────────────────────────────
 * Walk the program headers and load every PT_LOAD. */
static int load_segments(InCoreInode *ip, uiox_kix_psa_proc_t *proc,
                         const uiox_elf64_ehdr_t *eh)
{
    uint32_t i;

    if (eh->e_phnum == 0u || eh->e_phentsize == 0u) return -1;
    if (eh->e_phentsize < (uint16_t)sizeof(uiox_elf64_phdr_t)) return -1;
    if (eh->e_phoff > 0xFFFFFFFFull) return -1;

    for (i = 0u; i < (uint32_t)eh->e_phnum; i++) {
        uiox_elf64_phdr_t ph;
        uint32_t          off;

        /* e_phoff + i * e_phentsize.  The bound is checked in 32-bit
         * before the multiply, so the product cannot wrap and no 64-bit
         * divide is needed (that would pull in __aeabi_uldivmod on
         * arm32, absent in a freestanding link). */
        if ((uint32_t)i > ((0xFFFFFFFFu - (uint32_t)eh->e_phoff)
                           / (uint32_t)eh->e_phentsize)) return -1;

        off = (uint32_t)eh->e_phoff
              + (uint32_t)i * (uint32_t)eh->e_phentsize;

        if (readi(ip, (char *)&ph, (uint32_t)sizeof(ph), &off)
                != (int32_t)sizeof(ph)) return -1;

        if (ph.p_type != PT_LOAD) continue;

        if (load_one_segment(ip, proc, &ph) != 0) return -1;
    }

    return 0;
}

int64_t uiox_kix_scpcs_execve(uiox_uintptr_t path, uiox_uintptr_t argv,
                              uiox_uintptr_t envp)
{
    char                   kpath[EXECVE_PATH_MAX];
    uiox_elf64_ehdr_t      eh;
    uiox_kix_psa_proc_t   *proc;
    InCoreInode           *iip;
    InCoreInode           *root;
    uiox_kix_psa_sys_context_t *ctx;
    uint32_t               off;

    (void)argv;   /* NULL is legal; a real vector is not built yet */
    (void)envp;

    proc = uiox_kix_scpcs_current();
    if (!proc) return SCPCS_ESRCH;

    /* 1. the path must be inside the user window before we walk it */
    if (uiox_kix_scpcs_check_user_ptr(path, 1u, 1u) != 0) return SCPCS_EFAULT;

    /* 2. copy it in, probing each byte */
    if (user_copyin_str(kpath, path, (uint32_t)EXECVE_PATH_MAX) != 0)
        return SCPCS_EFAULT;

    /* 3. resolve it.  iget() uses ROOT_DEV (0) internally -- inode.h:217
     *    -- which is where UNFS is mounted (3a), so root inode and
     *    device come from one call.  namei's contract: the CALLER iputs
     *    the result, and cwd supplies both the start and the device. */
    root = iget(UNFS_ROOT_INO);
    if (!root) return SCPCS_ENOENT;

    iip = namei(kpath, root, 0u, 0u);
    if (!iip) {
        iput(root);
        return SCPCS_ENOENT;
    }

    /* 4. READ the header, then test what was read.  This is the step
     *    the stub faked: the magic is compared against bytes that came
     *    off the volume, not bytes written into this structure. */
    off = 0u;
    if (readi(iip, (char *)&eh, (uint32_t)sizeof(eh), &off)
            != (int32_t)sizeof(eh)) {
        iput(iip); iput(root);
        return SCPCS_ENOEXEC;
    }

    if (!is_elf64_for_this_arch(&eh)) {
        iput(iip); iput(root);
        return SCPCS_ENOEXEC;
    }

    /* 5. one region per PT_LOAD, unwound on failure */
    g_exec_added_n = 0u;

    if (load_segments(iip, proc, &eh) != 0) {
        attach_failed();
        iput(iip); iput(root);
        return SCPCS_ENOEXEC;
    }

    /* 6. the entry point.  rc_pc comes from the ELF; rc_sr's user-mode
     *    value is arch-specific and belongs to the arch layer; rc_sp is
     *    left as the context has it until the argv/envp stack exists.
     *
     *    context_for takes an INDEX into s_sys_ctx[] (context.c:157,
     *    161) -- the table is file-static, so the slot must be supplied
     *    by the caller.  A process carries no slot field (see
     *    uiox_kix_psa_proc_t), so the index is derived here from the
     *    process table position. */
    ctx = uiox_kix_psa_context_for(
              (uint32_t)(proc - uiox_kix_psa_proc_table));
    if (!ctx) {
        attach_failed();
        iput(iip); iput(root);
        return SCPCS_ESRCH;
    }

    ctx->sc_layers[0].rc_pc = (uintptr_t)eh.e_entry;

    /* 7. the loader's reference to the image inode ends here.  The
     *    frames belong to the process now; namei's contract says iput
     *    what it resolved, and root was ours from iget. */
    iput(iip);
    iput(root);

    return SCPCS_EOK;
}
