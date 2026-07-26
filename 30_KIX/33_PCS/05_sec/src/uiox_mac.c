/**
 * @file  uiox_mac.c
 * @brief UIOX Security — MAC access-control engine, label manager,
 *        VFS hooks, audit log, and syscall handlers.
 * @date  2026-07-08
 */
#include "../include/uiox_mac.h"

extern void uiox_fw_printf(const char *fmt, ...);
extern uint64_t uiox_sec_plat_time_ms(void);

/* ── No-libc helpers ──────────────────────────────────────────────────── */
static void mc_memset(void *d, int v, size_t n)
{ uint8_t *p = (uint8_t *)d; while (n--) *p++ = (uint8_t)v; }

static void mc_memcpy(void *d, const void *s, size_t n)
{ uint8_t *dp = (uint8_t *)d; const uint8_t *sp = (const uint8_t *)s;
  while (n--) *dp++ = *sp++; }

static bool mc_streq(const char *a, const char *b, size_t max)
{ for (size_t i = 0; i < max; i++) {
    if (a[i] != b[i]) return false;
    if (a[i] == '\0')  return true; } return true; }

static void mc_strncpy(char *d, const char *s, size_t n)
{ size_t i = 0; while (i < n - 1u && s[i]) { d[i] = s[i]; i++; } d[i] = '\0'; }

/* =========================================================================
 * String helpers
 * ====================================================================== */
const char *uiox_mac_decision_str(uiox_mac_decision_t d)
{
    switch (d) {
    case UIOX_MAC_ALLOW: return "ALLOW";
    case UIOX_MAC_DENY:  return "DENY";
    case UIOX_MAC_AUDIT: return "AUDIT";
    default:             return "?";
    }
}

const char *uiox_mac_class_str(uiox_mac_class_t c)
{
    switch (c) {
    case UIOX_MAC_CLASS_FILE:    return "file";
    case UIOX_MAC_CLASS_DIR:     return "dir";
    case UIOX_MAC_CLASS_SOCKET:  return "socket";
    case UIOX_MAC_CLASS_IPC:     return "ipc";
    case UIOX_MAC_CLASS_PROCESS: return "process";
    case UIOX_MAC_CLASS_DEVICE:  return "device";
    case UIOX_MAC_CLASS_KERNEL:  return "kernel";
    default:                     return "?";
    }
}

const char *uiox_mac_mode_str(uiox_mac_mode_t m)
{
    switch (m) {
    case UIOX_MAC_MODE_DISABLED:   return "disabled";
    case UIOX_MAC_MODE_PERMISSIVE: return "permissive";
    case UIOX_MAC_MODE_ENFORCING:  return "enforcing";
    default:                       return "?";
    }
}

const char *uiox_sec_err_str(uiox_sec_err_t e)
{
    switch (e) {
    case UIOX_SEC_OK:           return "OK";
    case UIOX_SEC_ERR_INVAL:    return "INVAL";
    case UIOX_SEC_ERR_NOMEM:    return "NOMEM";
    case UIOX_SEC_ERR_PERM:     return "PERM";
    case UIOX_SEC_ERR_BADLABEL: return "BADLABEL";
    case UIOX_SEC_ERR_NOPOLICY: return "NOPOLICY";
    case UIOX_SEC_ERR_OVERFLOW: return "OVERFLOW";
    case UIOX_SEC_ERR_NOTFOUND: return "NOTFOUND";
    case UIOX_SEC_ERR_BADMAGIC: return "BADMAGIC";
    case UIOX_SEC_ERR_READONLY: return "READONLY";
    case UIOX_SEC_ERR_RANGE:    return "RANGE";
    default:                    return "?";
    }
}

/* =========================================================================
 * Internal: append one audit entry
 * ====================================================================== */
static void audit_append(uiox_mac_policy_ctx_t   *ctx,
                          const uiox_mac_label_t  *subj,
                          const uiox_mac_label_t  *obj,
                          uiox_mac_class_t         cls,
                          uiox_mac_perm_t          perm,
                          uiox_mac_decision_t      dec,
                          uint32_t                 pid,
                          const char              *comm)
{
    uint32_t idx = ctx->audit_head % UIOX_MAC_AUDIT_LOG_SIZE;
    uiox_mac_audit_entry_t *e = &ctx->audit_log[idx];
    mc_memset(e, 0, sizeof(*e));

    mc_memcpy(&e->subject,  subj, sizeof(*subj));
    mc_memcpy(&e->object,   obj,  sizeof(*obj));
    e->obj_class    = cls;
    e->requested    = perm;
    e->decision     = dec;
    e->pid          = pid;
    e->timestamp_ms = uiox_sec_plat_time_ms();
    if (comm) mc_strncpy(e->comm, comm, 16u);

    ctx->audit_head++;
    if (ctx->audit_count < UIOX_MAC_AUDIT_LOG_SIZE)
        ctx->audit_count++;
}

/* =========================================================================
 * Core access check
 * ====================================================================== */
uiox_mac_decision_t uiox_mac_check(uiox_mac_policy_ctx_t   *ctx,
                                    const uiox_mac_label_t  *subject,
                                    const uiox_mac_label_t  *object,
                                    uiox_mac_class_t         obj_class,
                                    uiox_mac_perm_t          perm,
                                    uint32_t                 pid,
                                    const char              *comm)
{
    /* DISABLED mode — always allow, no logging */
    if (!ctx || ctx->mode == UIOX_MAC_MODE_DISABLED)
        return UIOX_MAC_ALLOW;

    /* Validate labels */
    if (!subject || !object || !subject->valid || !object->valid)
        return UIOX_MAC_DENY;

    /* Walk rule table looking for a matching allow rule */
    uiox_mac_perm_t granted = UIOX_MAC_PERM_NONE;
    uiox_mac_perm_t audited = UIOX_MAC_PERM_NONE;

    for (uint32_t i = 0; i < ctx->rule_count; i++) {
        const uiox_mac_rule_t *r = &ctx->rules[i];
        if (!r->active)                          continue;
        if (r->subject_type != subject->type_id) continue;
        if (r->object_type  != object->type_id)  continue;
        if (r->obj_class    != obj_class)         continue;

        granted |= r->allow;
        audited |= r->audit;
    }

    bool allowed = ((granted & perm) == perm);
    bool should_audit = !!(audited & perm) || !allowed;

    if (!allowed) {
        /* Log denial */
        if (should_audit)
            audit_append(ctx, subject, object, obj_class, perm,
                         UIOX_MAC_DENY, pid, comm);

        uiox_fw_printf("[mac] %s  pid=%u  subj=%s  obj=%s  "
                       "class=%s  perm=0x%x\n",
                       ctx->mode == UIOX_MAC_MODE_PERMISSIVE
                           ? "PERMISSIVE_DENY" : "DENY",
                       pid,
                       subject->context, object->context,
                       uiox_mac_class_str(obj_class), perm);

        /* Permissive mode: log but allow */
        if (ctx->mode == UIOX_MAC_MODE_PERMISSIVE) {
            audit_append(ctx, subject, object, obj_class, perm,
                         UIOX_MAC_AUDIT, pid, comm);
            return UIOX_MAC_AUDIT;
        }
        return UIOX_MAC_DENY;
    }

    /* Allowed — log if audit rule applies */
    if (audited & perm)
        audit_append(ctx, subject, object, obj_class, perm,
                     UIOX_MAC_AUDIT, pid, comm);

    return UIOX_MAC_ALLOW;
}

/* =========================================================================
 * Convenience enforce wrapper
 * ====================================================================== */
uiox_sec_err_t uiox_mac_enforce(uiox_mac_policy_ctx_t  *ctx,
                                  const uiox_mac_label_t *subject,
                                  const uiox_mac_label_t *object,
                                  uiox_mac_class_t        obj_class,
                                  uiox_mac_perm_t         perm,
                                  uint32_t                pid,
                                  const char             *comm)
{
    uiox_mac_decision_t d =
        uiox_mac_check(ctx, subject, object, obj_class, perm, pid, comm);
    return (d == UIOX_MAC_DENY) ? UIOX_SEC_ERR_PERM : UIOX_SEC_OK;
}

/* =========================================================================
 * Label parse — "user:role:type:level" → uiox_mac_label_t
 * ====================================================================== */
uiox_sec_err_t uiox_mac_label_parse(uiox_mac_policy_ctx_t *ctx,
                                     const char            *context,
                                     uiox_mac_label_t      *out)
{
    if (!ctx || !context || !out) return UIOX_SEC_ERR_INVAL;

    mc_memset(out, 0, sizeof(*out));
    mc_strncpy(out->context, context, UIOX_MAC_CONTEXT_LEN);

    /* Extract the type field (3rd colon-separated token) */
    const char *p    = context;
    uint32_t    colon = 0u;
    char        type_name[UIOX_MAC_LABEL_LEN];
    uint32_t    ti = 0u;
    char        level_str[16];
    uint32_t    li = 0u;
    bool        in_level = false;

    mc_memset(type_name, 0, sizeof(type_name));
    mc_memset(level_str, 0, sizeof(level_str));

    while (*p) {
        if (*p == ':') {
            colon++;
            p++;
            continue;
        }
        if (colon == 2u && ti < UIOX_MAC_LABEL_LEN - 1u)
            type_name[ti++] = *p;
        if (colon == 3u && li < 15u)
            level_str[li++] = *p;
        p++;
    }

    /* Resolve type name → id */
    uiox_sec_err_t rc =
        uiox_mac_policy_lookup_type(ctx, type_name, &out->type_id);
    if (rc != UIOX_SEC_OK) {
        /* Unknown type — assign sentinel; permissive mode may still allow */
        out->type_id = (uint32_t)-1;
    }

    /* Parse level (simple decimal) */
    out->level = 0u;
    for (uint32_t i = 0; i < li; i++) {
        if (level_str[i] >= '0' && level_str[i] <= '9')
            out->level = out->level * 10u + (uint32_t)(level_str[i] - '0');
        else
            break;
    }

    out->valid = true;
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Label format → string
 * ====================================================================== */
uiox_sec_err_t uiox_mac_label_format(const uiox_mac_label_t *label,
                                       char *buf, size_t buf_size)
{
    if (!label || !buf || buf_size == 0u) return UIOX_SEC_ERR_INVAL;
    mc_strncpy(buf, label->context, buf_size);
    return UIOX_SEC_OK;
}

/* =========================================================================
 * Label copy / equal
 * ====================================================================== */
void uiox_mac_label_copy(uiox_mac_label_t       *dst,
                          const uiox_mac_label_t *src)
{
    if (!dst || !src) return;
    mc_memcpy(dst, src, sizeof(*dst));
}

bool uiox_mac_label_equal(const uiox_mac_label_t *a,
                           const uiox_mac_label_t *b)
{
    if (!a || !b) return false;
    return (a->type_id == b->type_id) &&
           (a->level   == b->level)   &&
           mc_streq(a->context, b->context, UIOX_MAC_CONTEXT_LEN);
}

/* =========================================================================
 * Label transition on exec()
 * ====================================================================== */
uiox_sec_err_t uiox_mac_label_exec_transition(
        uiox_mac_policy_ctx_t  *ctx,
        const uiox_mac_label_t *parent,
        const uiox_mac_label_t *exec_file,
        uiox_mac_label_t       *out_child)
{
    if (!ctx || !parent || !exec_file || !out_child)
        return UIOX_SEC_ERR_INVAL;

    /*
     * Simplified model: check for an explicit type_transition rule.
     * A full implementation would walk a separate transition table.
     * For now: inherit parent label unless exec_file has a different type,
     * in which case adopt exec_file's type (domain transition).
     */
    uiox_mac_label_copy(out_child, parent);

    if (exec_file->type_id != parent->type_id &&
        exec_file->type_id != (uint32_t)-1) {
        out_child->type_id = exec_file->type_id;
        mc_strncpy(out_child->context, exec_file->context,
                   UIOX_MAC_CONTEXT_LEN);
    }

    return UIOX_SEC_OK;
}

/* =========================================================================
 * Label transition on file create
 * ====================================================================== */
uiox_sec_err_t uiox_mac_label_file_transition(
        uiox_mac_policy_ctx_t  *ctx,
        const uiox_mac_label_t *parent_dir,
        const uiox_mac_label_t *creating_process,
        uiox_mac_class_t        obj_class,
        uiox_mac_label_t       *out_file)
{
    if (!ctx || !parent_dir || !creating_process || !out_file)
        return UIOX_SEC_ERR_INVAL;

    (void)obj_class;  /* extended in full implementation */

    /* Default: new file inherits parent directory label */
    uiox_mac_label_copy(out_file, parent_dir);
    return UIOX_SEC_OK;
}

/* =========================================================================
 * VFS hooks
 * ====================================================================== */
uiox_sec_err_t uiox_mac_vfs_open(uiox_mac_policy_ctx_t  *ctx,
                                   const uiox_mac_label_t *proc_label,
                                   const uiox_mac_label_t *file_label,
                                   uint32_t                open_flags,
                                   uint32_t                pid)
{
    /* Determine permissions needed from open flags */
    uiox_mac_perm_t perm = UIOX_MAC_PERM_READ; /* O_RDONLY default */

    /* O_WRONLY=1, O_RDWR=2, O_APPEND=1024 */
    if (open_flags & 1u)  perm |= UIOX_MAC_PERM_WRITE;
    if (open_flags & 2u)  perm |= (UIOX_MAC_PERM_READ | UIOX_MAC_PERM_WRITE);
    if (open_flags & 1024u) perm |= UIOX_MAC_PERM_APPEND;

    return uiox_mac_enforce(ctx, proc_label, file_label,
                             UIOX_MAC_CLASS_FILE, perm, pid, NULL);
}

uiox_sec_err_t uiox_mac_vfs_unlink(uiox_mac_policy_ctx_t  *ctx,
                                     const uiox_mac_label_t *proc_label,
                                     const uiox_mac_label_t *file_label,
                                     uint32_t                pid)
{
    return uiox_mac_enforce(ctx, proc_label, file_label,
                             UIOX_MAC_CLASS_FILE,
                             UIOX_MAC_PERM_DELETE, pid, NULL);
}

uiox_sec_err_t uiox_mac_vfs_exec(uiox_mac_policy_ctx_t  *ctx,
                                   const uiox_mac_label_t *proc_label,
                                   const uiox_mac_label_t *file_label,
                                   uiox_mac_label_t       *out_new_label,
                                   uint32_t                pid)
{
    /* First check: process must be allowed to execute the file */
    uiox_sec_err_t rc =
        uiox_mac_enforce(ctx, proc_label, file_label,
                         UIOX_MAC_CLASS_FILE,
                         UIOX_MAC_PERM_EXEC, pid, NULL);
    if (rc != UIOX_SEC_OK) return rc;

    /* Then compute the new process label via domain transition */
    return uiox_mac_label_exec_transition(ctx, proc_label,
                                           file_label, out_new_label);
}

/* =========================================================================
 * Audit log
 * ====================================================================== */
void uiox_mac_audit_flush(uiox_mac_policy_ctx_t *ctx)
{
    if (!ctx) return;
    uint32_t start = (ctx->audit_count >= UIOX_MAC_AUDIT_LOG_SIZE)
                     ? ctx->audit_head  /* wrapped */
                     : 0u;

    for (uint32_t i = 0; i < ctx->audit_count; i++) {
        const uiox_mac_audit_entry_t *e =
            &ctx->audit_log[(start + i) % UIOX_MAC_AUDIT_LOG_SIZE];
        uiox_fw_printf("[mac-audit] %s  pid=%-5u  %-8s  subj=%-32s  "
                       "obj=%-32s  perm=0x%03x\n",
                       uiox_mac_decision_str(e->decision),
                       e->pid,
                       uiox_mac_class_str(e->obj_class),
                       e->subject.context,
                       e->object.context,
                       e->requested);
    }
}

void uiox_mac_audit_print(const uiox_mac_policy_ctx_t *ctx, uint32_t count)
{
    if (!ctx || count == 0u) return;
    if (count > ctx->audit_count) count = ctx->audit_count;

    uiox_fw_printf("[mac-audit] Last %u entries:\n", count);
    uint32_t start = (ctx->audit_head + UIOX_MAC_AUDIT_LOG_SIZE - count)
                     % UIOX_MAC_AUDIT_LOG_SIZE;

    for (uint32_t i = 0; i < count; i++) {
        const uiox_mac_audit_entry_t *e =
            &ctx->audit_log[(start + i) % UIOX_MAC_AUDIT_LOG_SIZE];
        uiox_fw_printf("  [%03u] %-5s  pid=%-5u  %-7s  %s → %s  perm=0x%x\n",
                       i,
                       uiox_mac_decision_str(e->decision),
                       e->pid,
                       uiox_mac_class_str(e->obj_class),
                       e->subject.context,
                       e->object.context,
                       e->requested);
    }
}

/* =========================================================================
 * Syscall handlers
 * ====================================================================== */
static uiox_sec_ctx_t *g_sec_ctx = NULL;
void uiox_sec_set_global_ctx(uiox_sec_ctx_t *ctx) { g_sec_ctx = ctx; }

long sys_getlabel(long fd_or_pid, long buf, long buf_size, long flags)
{
    (void)fd_or_pid; (void)flags;
    if (!g_sec_ctx || !buf || buf_size < (long)UIOX_MAC_CONTEXT_LEN)
        return (long)UIOX_SEC_ERR_INVAL;
    /* Production: resolve fd/pid → label; copy_to_user() */
    mc_strncpy((char *)(uintptr_t)buf,
               UIOX_MAC_LABEL_KERNEL, (size_t)buf_size);
    return (long)UIOX_SEC_OK;
}

long sys_setlabel(long fd_or_pid, long label, long label_sz, long flags)
{
    (void)flags;
    if (!g_sec_ctx || !label || label_sz <= 0)
        return (long)UIOX_SEC_ERR_INVAL;
    /* Production: resolve fd/pid → object; MAC check SETLABEL; apply */
    uiox_fw_printf("[mac] sys_setlabel called (stub)\n");
    return (long)UIOX_SEC_OK;
}

long sys_getpolicy(long buf, long buf_sz, long a2, long a3)
{
    (void)a2; (void)a3;
    if (!g_sec_ctx || !buf || buf_sz < 4) return (long)UIOX_SEC_ERR_INVAL;
    *(uint32_t *)(uintptr_t)buf = (uint32_t)g_sec_ctx->mac.mode;
    return (long)UIOX_SEC_OK;
}

long sys_setpolicy(long mode, long a1, long a2, long a3)
{
    (void)a1; (void)a2; (void)a3;
    if (!g_sec_ctx) return (long)UIOX_SEC_ERR_INVAL;
    return (long)uiox_mac_policy_set_mode(&g_sec_ctx->mac,
                                           (uiox_mac_mode_t)mode);
}

long sys_aslr_status(long buf, long buf_sz, long a2, long a3)
{
    (void)a2; (void)a3;
    if (!g_sec_ctx || !buf ||
        buf_sz < (long)sizeof(uiox_aslr_ctx_t))
        return (long)UIOX_SEC_ERR_INVAL;
    mc_memcpy((void *)(uintptr_t)buf,
               &g_sec_ctx->aslr, sizeof(uiox_aslr_ctx_t));
    return (long)UIOX_SEC_OK;
}
