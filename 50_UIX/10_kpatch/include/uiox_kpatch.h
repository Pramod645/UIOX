/**
 * @file  uiox_kpatch.h
 * @brief UIOX Live Kernel Patching — master umbrella include.
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_KPATCH_H
 #define UIOX_KPATCH_H
 
 #include "uiox_kp_types.h"
 #include "uiox_kp_arch.h"
 #include "uiox_kp_mem.h"
 #include "uiox_kp_patch.h"
 
 #define UIOX_KPATCH_VERSION_STR  "UIOX kpatch v1.0"
 #define UIOX_KPATCH_URL          "github.com/Pramod645/UIOX"
 
 /* Quick status string */
 static inline const char *uiox_kp_state_name(uiox_kp_state_t s) {
     switch (s) {
     case UIOX_KP_STATE_UNREGISTERED: return "UNREGISTERED";
     case UIOX_KP_STATE_REGISTERED:   return "REGISTERED";
     case UIOX_KP_STATE_ENABLED:      return "ENABLED";
     case UIOX_KP_STATE_DISABLED:     return "DISABLED";
     case UIOX_KP_STATE_ERROR:        return "ERROR";
     default:                          return "UNKNOWN";
     }
 }
 
 static inline const char *uiox_kp_err_str(uiox_kp_err_t e) {
     switch (e) {
     case UIOX_KP_OK:          return "OK";
     case UIOX_KP_ERR_INVAL:   return "INVAL";
     case UIOX_KP_ERR_NOMEM:   return "NOMEM";
     case UIOX_KP_ERR_ALREADY: return "ALREADY_PATCHED";
     case UIOX_KP_ERR_NOTFOUND:return "NOTFOUND";
     case UIOX_KP_ERR_BUSY:    return "TABLE_FULL";
     case UIOX_KP_ERR_FAULT:   return "FAULT";
     case UIOX_KP_ERR_UNSUP:   return "UNSUPPORTED";
     case UIOX_KP_ERR_ACTIVE:  return "STILL_ACTIVE";
     case UIOX_KP_ERR_PERM:    return "PERM";
     default:                   return "UNKNOWN";
     }
 }
 
 #endif /* UIOX_KPATCH_H */
 