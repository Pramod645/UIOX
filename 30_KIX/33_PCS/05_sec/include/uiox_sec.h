/**
 * @file  uiox_sec.h
 * @brief UIOX Security — master header.
 *
 * Single include for all consumers. Aggregates ASLR + MAC under one
 * initialisation call (uiox_sec_init) and one global context struct.
 *
 * Typical call sequence:
 *
 *   // Kernel init
 *   uiox_sec_init(&g_sec, UIOX_ASLR_LEVEL_FULL, UIOX_MAC_MODE_ENFORCING);
 *   uiox_mac_policy_load(&g_sec.mac, policy_blob, policy_size);
 *   uiox_mac_policy_seal(&g_sec.mac);
 *
 *   // On exec() in 33_ProcessControlSubsystem:
 *   uiox_aslr_randomise_mm(&g_sec.aslr, &proc->mm, is_pie, false);
 *   uiox_mac_vfs_exec(&g_sec.mac, &parent->label,
 *                     &exec_file_label, &proc->label, proc->pid);
 *
 *   // On every VFS open() in 32_FileSystem:
 *   uiox_mac_vfs_open(&g_sec.mac, &proc->label,
 *                     &file->label, flags, proc->pid);
 *
 * @version 1.0.0
 * @date    2026-07-08
 */
#ifndef UIOX_SEC_H
#define UIOX_SEC_H

#include "uiox_sec_types.h"
#include "uiox_aslr.h"
#include "uiox_mac_policy.h"
#include "uiox_mac.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ASLR level constants (for uiox_sec_init / sysctl)
 * ====================================================================== */
#define UIOX_ASLR_LEVEL_OFF      0u
#define UIOX_ASLR_LEVEL_PARTIAL  1u
#define UIOX_ASLR_LEVEL_FULL     2u  /**< Default: exec+stack+heap+mmap+vDSO */
#define UIOX_ASLR_LEVEL_KSTACK   3u  /**< Full + kernel stack               */

/* =========================================================================
 * Master security context (one per system)
 * ====================================================================== */
typedef struct {
    uiox_aslr_ctx_t       aslr;
    uiox_mac_policy_ctx_t mac;
    bool                  initialized;
} uiox_sec_ctx_t;

/* =========================================================================
 * Lifecycle
 * ====================================================================== */

/**
 * @brief Initialise ASLR and MAC subsystems.
 *        Called once from kernel main() before any process is spawned.
 * @param aslr_level  UIOX_ASLR_LEVEL_* constant.
 * @param mac_mode    Initial MAC mode (usually PERMISSIVE until policy loads).
 */
uiox_sec_err_t uiox_sec_init(uiox_sec_ctx_t *ctx,
                               uint8_t         aslr_level,
                               uiox_mac_mode_t mac_mode);

/** Print a one-page security subsystem summary to the console. */
void uiox_sec_print(const uiox_sec_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
#endif /* UIOX_SEC_H */
