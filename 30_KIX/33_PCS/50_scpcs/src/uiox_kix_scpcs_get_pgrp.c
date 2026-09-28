/*
 * 33_PCS/50_scpcs/src/uiox_kix_scpcs_get_pgrp.c
 *
 * getpgrp() — Group 3, Miscellaneous.
 *
 * ── the concept is in scope, the field is not ───────────────────────
 * Process groups are named in the algorithm text's Miscellaneous column,
 * so this is not an invention.  What is missing is anywhere to store one:
 * the process table entry carries no group field.
 *
 * The private signal structure does carry one — which is the shape of the
 * problem rather than a solution.  The field exists on a model nothing
 * else uses and is absent from the model everything binds to.
 *
 * ── why ENOSYS and not 0 ────────────────────────────────────────────
 * Returning 0 would claim the caller is in process group 0, which is the
 * swapper's group.  That is not merely wrong, it is a wrong answer of a
 * kind a caller cannot detect: process group IDs are opaque, so any value
 * is plausible.
 *
 * TO FINISH: add a group field to the process table entry, inherit it in
 * the fork path, and this becomes a one-field return like getpid.
 *
 * @version 1.0.0  @date 2026-09-29
 */
#include "../include/uiox_kix_scpcs.h"

int64_t uiox_kix_scpcs_get_pgrp(void)
{
    if (!uiox_kix_scps_current()) return SCPCS_ESRCH;
    return SCPCS_ENOSYS;   /* no group field on the process table entry */
}
