/**
 * @file  uiox_klibc.h
 * @brief Forwarder — the canonical copy is 30_KIX/common/uiox_klibc.h.
 *
 * This layer's own -I entry ($(INCDIR)) must still resolve the name, and
 * the per-file rule adds the source's own directory first, so a quoted
 * #include "uiox_klibc.h" stops here rather than falling through to
 * ../common.  Rather than maintain a second divergent copy — the fork that
 * caused the base-type and errno collisions — this file only points at
 * the one definition.
 */
#ifndef UIOX_KLIBC_FWD_H
#define UIOX_KLIBC_FWD_H
#include "../../../common/uiox_klibc.h"
#endif /* UIOX_KLIBC_FWD_H */
