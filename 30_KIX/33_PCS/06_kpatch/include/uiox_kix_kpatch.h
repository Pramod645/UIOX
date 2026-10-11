/**
 * @file  uiox_kix_kpatch.h
 * @brief UIOX Live Kernel Patching — master umbrella include.
 * @version 1.1.0  @date 2026-10-02
 */

 #ifndef UIOX_KIX_KPATCH_H
 #define UIOX_KIX_KPATCH_H
 
 #include "uiox_kix_kp_types.h"
 #include "uiox_kix_kp_arch.h"
 #include "uiox_kix_kp_mem.h"
 #include "uiox_kix_kp_arg.h"
 #include "uiox_kix_kp_patch.h"
 
 #define UIOX_KIX_KPATCH_VERSION_STR  "UIOX kpatch v1.1"
 #define UIOX_KIX_KPATCH_URL          "github.com/Pramod645/UIOX"
 
 static inline const char *uiox_kix_kp_state_name(uiox_kix_kp_state_t s) {
     switch (s) {
     case UIOX_KIX_KP_STATE_UNREGISTERED: return "UNREGISTERED";
     case UIOX_KIX_KP_STATE_REGISTERED:   return "REGISTERED";
     case UIOX_KIX_KP_STATE_ENABLED:      return "ENABLED";
     case UIOX_KIX_KP_STATE_DISABLED:     return "DISABLED";
     case UIOX_KIX_KP_STATE_ERROR:        return "ERROR";
     default:                             return "UNKNOWN";
     }
 }
 
 static inline const char *uiox_kix_kp_err_str(uiox_kix_kp_err_t e) {
     switch (e) {
     case UIOX_KIX_KP_OK:            return "OK";
     case UIOX_KIX_KP_ERR_INVAL:     return "INVAL";
     case UIOX_KIX_KP_ERR_NOMEM:     return "NOMEM";
     case UIOX_KIX_KP_ERR_ALREADY:   return "ALREADY_PATCHED";
     case UIOX_KIX_KP_ERR_NOTFOUND:  return "NOTFOUND";
     case UIOX_KIX_KP_ERR_BUSY:      return "TABLE_FULL";
     case UIOX_KIX_KP_ERR_FAULT:     return "FAULT";
     case UIOX_KIX_KP_ERR_UNSUP:     return "UNSUPPORTED";
     case UIOX_KIX_KP_ERR_ACTIVE:    return "STILL_ACTIVE";
     case UIOX_KIX_KP_ERR_PERM:      return "PERM";
     case UIOX_KIX_KP_EFAULT:        return "EFAULT";
     default:                        return "UNKNOWN";
     }
 }
 
 #endif /* UIOX_KIX_KPATCH_H */
 