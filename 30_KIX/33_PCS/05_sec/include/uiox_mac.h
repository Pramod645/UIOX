/**
 * @file  uiox_mac.h
 * @brief UIOX Security — MAC access-control engine and label management.
 *
 * This is the hot path called on every security-sensitive operation.
 * All VFS, IPC, socket, and process operations must pass through
 * uiox_mac_check() before the kernel grants access.
 *
 * Label lifecycle:
 *   Boot         → kernel objects labelled from initial SID table
 *   exec()       → process inherits parent label (or policy transition)
 *   file create  → inherits parent directory label (or type_transition rule)
 *   setlabel()   → explicit relabelling (requires SETLABEL permission)
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_MAC_H
#define UIOX_MAC_H

#include "uiox_sec_types.h"
#include "uiox_mac_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Syscall numbers (40_SystemCallInterface)
 * ====================================================================== */
#define SYS_GETLABEL       250u   /**< Get label of fd / pid              */
#define SYS_SETLABEL       251u   /**< Set label (requires privilege)     */
#define SYS_GETPOLICY      252u   /**< Get MAC enforcement mode           */
#define SYS_SETPOLICY      253u   /**< Set MAC enforcement mode (root)    */
#define SYS_ASLR_STATUS    254u   /**< Query ASLR configuration           */

/* =========================================================================
 * Well-known initial labels (used during early boot before policy loads)
 * ====================================================================== */
#define UIOX_MAC_LABEL_KERNEL    "system_u:system_r:kernel_t:s15"
#define UIOX_MAC_LABEL_INIT      "system_u:system_r:init_t:s0"
#define UIOX_MAC_LABEL_UNLABELLED "system_u:object_r:unlabelled_t:s0"

/* =========================================================================
 * MAC engine API
 * ====================================================================== */

/**
 * @brief Core access check — hot path for every security-sensitive call.
 *
 * @param ctx        Policy context.
 * @param subject    Label of the requesting process.
 * @param object     Label of the resource being accessed.
 * @param obj_class  Class of the object (file / socket / process / ...).
 * @param perm       Permission(s) being requested (may be OR-combined).
 * @param pid        PID of subject (for audit log).
 * @param comm       Process name (for audit log, may be NULL).
 * @return UIOX_MAC_ALLOW or UIOX_MAC_DENY or UIOX_MAC_AUDIT.
 */
uiox_mac_decision_t uiox_mac_check(uiox_mac_policy_ctx_t   *ctx,
                                    const uiox_mac_label_t  *subject,
                                    const uiox_mac_label_t  *object,
                                    uiox_mac_class_t         obj_class,
                                    uiox_mac_perm_t          perm,
                                    uint32_t                 pid,
                                    const char              *comm);

/**
 * @brief Convenience wrapper — returns UIOX_SEC_OK if allowed,
 *        UIOX_SEC_ERR_PERM if denied. Preferred in VFS / syscall paths.
 */
uiox_sec_err_t uiox_mac_enforce(uiox_mac_policy_ctx_t  *ctx,
                                  const uiox_mac_label_t *subject,
                                  const uiox_mac_label_t *object,
                                  uiox_mac_class_t        obj_class,
                                  uiox_mac_perm_t         perm,
                                  uint32_t                pid,
                                  const char             *comm);

/* =========================================================================
 * Label management
 * ====================================================================== */

/**
 * @brief Parse a "user:role:type:level" context string into a label.
 *        Resolves type name → type_id using the loaded policy.
 */
uiox_sec_err_t uiox_mac_label_parse(uiox_mac_policy_ctx_t *ctx,
                                     const char            *context,
                                     uiox_mac_label_t      *out);

/**
 * @brief Render a label back to its "user:role:type:level" string.
 */
uiox_sec_err_t uiox_mac_label_format(const uiox_mac_label_t *label,
                                       char *buf, size_t buf_size);

/**
 * @brief Compute the inherited label for a new process on exec().
 *        Applies type_transition rules; falls back to parent label.
 */
uiox_sec_err_t uiox_mac_label_exec_transition(
        uiox_mac_policy_ctx_t  *ctx,
        const uiox_mac_label_t *parent,
        const uiox_mac_label_t *exec_file,
        uiox_mac_label_t       *out_child);

/**
 * @brief Compute the inherited label for a new file on create().
 *        Applies file type_transition rules; falls back to dir label.
 */
uiox_sec_err_t uiox_mac_label_file_transition(
        uiox_mac_policy_ctx_t  *ctx,
        const uiox_mac_label_t *parent_dir,
        const uiox_mac_label_t *creating_process,
        uiox_mac_class_t        obj_class,
        uiox_mac_label_t       *out_file);

/**
 * @brief Copy @src label into @dst.
 */
void uiox_mac_label_copy(uiox_mac_label_t       *dst,
                          const uiox_mac_label_t *src);

/**
 * @brief Return true if two labels are identical.
 */
bool uiox_mac_label_equal(const uiox_mac_label_t *a,
                           const uiox_mac_label_t *b);

/* =========================================================================
 * Audit log API
 * ====================================================================== */

/** Flush the audit log to the console / audit daemon. */
void uiox_mac_audit_flush(uiox_mac_policy_ctx_t *ctx);

/** Print the last @count audit entries. */
void uiox_mac_audit_print(const uiox_mac_policy_ctx_t *ctx,
                           uint32_t                     count);

/* =========================================================================
 * VFS hooks — called from 32_FileSystem
 * ====================================================================== */

/** Called on open() — check READ/WRITE/EXEC against file label. */
uiox_sec_err_t uiox_mac_vfs_open (uiox_mac_policy_ctx_t  *ctx,
                                    const uiox_mac_label_t *proc_label,
                                    const uiox_mac_label_t *file_label,
                                    uint32_t                open_flags,
                                    uint32_t                pid);

/** Called on unlink() / rmdir(). */
uiox_sec_err_t uiox_mac_vfs_unlink(uiox_mac_policy_ctx_t  *ctx,
                                     const uiox_mac_label_t *proc_label,
                                     const uiox_mac_label_t *file_label,
                                     uint32_t                pid);

/** Called on exec() — compute child label + check EXEC. */
uiox_sec_err_t uiox_mac_vfs_exec  (uiox_mac_policy_ctx_t  *ctx,
                                     const uiox_mac_label_t *proc_label,
                                     const uiox_mac_label_t *file_label,
                                     uiox_mac_label_t       *out_new_label,
                                     uint32_t                pid);

/* =========================================================================
 * Syscall handlers
 * ====================================================================== */
long sys_getlabel  (long fd_or_pid, long buf,  long buf_size, long flags);
long sys_setlabel  (long fd_or_pid, long label, long label_sz, long flags);
long sys_getpolicy (long buf,       long buf_sz, long a2,      long a3);
long sys_setpolicy (long mode,      long a1,     long a2,      long a3);
long sys_aslr_status(long buf,      long buf_sz, long a2,      long a3);

/* =========================================================================
 * Helpers
 * ====================================================================== */
const char *uiox_mac_decision_str (uiox_mac_decision_t d);
const char *uiox_mac_class_str    (uiox_mac_class_t c);
const char *uiox_mac_mode_str     (uiox_mac_mode_t m);
const char *uiox_sec_err_str      (uiox_sec_err_t e);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_MAC_H */
