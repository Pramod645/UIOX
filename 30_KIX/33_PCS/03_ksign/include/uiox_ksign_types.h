/**
 * @file  uiox_ksign_types.h
 * @brief UIOX Signed Kernel — base types, magic numbers, error codes.
 *
 * Integrates with:
 *   02_FwHal/uiox_fw_secboot.h  — SHA-256 + existing image header
 *   40_SystemCallInterface       — sys_kernel_verify()
 *   33_ProcessControlSubsystem   — integrity check on context switch
 *
 * @version 1.0.0
 * @date    2026-07-07
 */

 #ifndef UIOX_KSIGN_TYPES_H
 #define UIOX_KSIGN_TYPES_H
 
 #include <stdint.h>
 #include <stdbool.h>
 #include <stddef.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Error codes
  * ====================================================================== */
 
 typedef enum {
     UIOX_KS_OK              =  0,
     UIOX_KS_ERR_INVAL       = -1,
     UIOX_KS_ERR_NOMEM       = -2,
     UIOX_KS_ERR_BADMAGIC    = -3,  /**< Wrong image/key magic           */
     UIOX_KS_ERR_BADVERSION  = -4,  /**< Unsupported format version      */
     UIOX_KS_ERR_HASH        = -5,  /**< Hash mismatch                   */
     UIOX_KS_ERR_SIG         = -6,  /**< Signature invalid               */
     UIOX_KS_ERR_CERT        = -7,  /**< Certificate chain broken        */
     UIOX_KS_ERR_REVOKED     = -8,  /**< Key revoked in KRL              */
     UIOX_KS_ERR_ROLLBACK    = -9,  /**< Anti-rollback check failed      */
     UIOX_KS_ERR_NOTFOUND    = -10, /**< Key / cert not found            */
     UIOX_KS_ERR_EXPIRED     = -11, /**< Key or cert expired             */
     UIOX_KS_ERR_TAMPERED    = -12, /**< Runtime integrity violation     */
     UIOX_KS_ERR_UNSUP       = -13,
     UIOX_KS_ERR_IO          = -14,
 } uiox_ks_err_t;
 
 /* =========================================================================
  * Magic numbers
  * ====================================================================== */
 
 #define UIOX_KS_IMG_MAGIC       0x554B5349u  /**< "UKSI" Kernel Sig Image */
 #define UIOX_KS_KEY_MAGIC       0x554B4B45u  /**< "UKKE" Kernel Key Entry */
 #define UIOX_KS_KRL_MAGIC       0x554B4B52u  /**< "UKKR" Key Revoc List  */
 #define UIOX_KS_LOG_MAGIC       0x554B4C47u  /**< "UKLG" Measurement Log */
 #define UIOX_KS_FORMAT_VERSION  1u
 
 /* =========================================================================
  * Digest sizes
  * ====================================================================== */
 
 #define UIOX_KS_SHA256_LEN      32u
 #define UIOX_KS_SHA384_LEN      48u
 #define UIOX_KS_SHA512_LEN      64u
 #define UIOX_KS_DIGEST_MAX      64u
 
 /* =========================================================================
  * Signature algorithm identifiers
  * ====================================================================== */
 
 typedef enum {
     UIOX_KS_ALG_NONE        = 0,
     UIOX_KS_ALG_RSA2048_SHA256  = 1,
     UIOX_KS_ALG_RSA4096_SHA256  = 2,
     UIOX_KS_ALG_ECDSA_P256  = 3,
     UIOX_KS_ALG_ED25519     = 4,
 } uiox_ks_alg_t;
 
 /* =========================================================================
  * Hash algorithm identifiers
  * ====================================================================== */
 
 typedef enum {
     UIOX_KS_HASH_SHA256 = 0,
     UIOX_KS_HASH_SHA384 = 1,
     UIOX_KS_HASH_SHA512 = 2,
 } uiox_ks_hash_alg_t;
 
 /* =========================================================================
  * Key usage flags (bitmask)
  * ====================================================================== */
 
 #define UIOX_KS_USAGE_SIGN_KERNEL  (1u << 0)
 #define UIOX_KS_USAGE_SIGN_MODULE  (1u << 1)
 #define UIOX_KS_USAGE_SIGN_BOOT    (1u << 2)
 #define UIOX_KS_USAGE_CERT_SIGN    (1u << 3)
 #define UIOX_KS_USAGE_KRL_SIGN     (1u << 4)
 
 /* =========================================================================
  * PCR indices (TPM-compatible)
  * ====================================================================== */
 
 #define UIOX_KS_PCR_FIRMWARE     0u   /**< Firmware measurement         */
 #define UIOX_KS_PCR_KERNEL_CODE  1u   /**< Kernel .text section hash    */
 #define UIOX_KS_PCR_KERNEL_DATA  2u   /**< Kernel .rodata hash          */
 #define UIOX_KS_PCR_CMDLINE      3u   /**< Kernel cmdline               */
 #define UIOX_KS_PCR_MODULES      4u   /**< Loaded modules               */
 #define UIOX_KS_PCR_RUNTIME      5u   /**< Runtime integrity checks     */
 #define UIOX_KS_PCR_MAX          8u
 
 /* =========================================================================
  * Utility macros
  * ====================================================================== */
 
 #define UIOX_KS_UNUSED(x)       ((void)(x))
 #define UIOX_KS_ARRAY_SIZE(a)   (sizeof(a)/sizeof((a)[0]))
 #define UIOX_KS_MIN(a,b)        ((a)<(b)?(a):(b))
 
 static inline const char *uiox_ks_err_str(uiox_ks_err_t e) {
     switch (e) {
     case UIOX_KS_OK:           return "OK";
     case UIOX_KS_ERR_INVAL:    return "EINVAL";
     case UIOX_KS_ERR_BADMAGIC: return "EBADMAGIC";
     case UIOX_KS_ERR_HASH:     return "EHASH_MISMATCH";
     case UIOX_KS_ERR_SIG:      return "ESIG_INVALID";
     case UIOX_KS_ERR_CERT:     return "ECERT_CHAIN";
     case UIOX_KS_ERR_REVOKED:  return "EKEY_REVOKED";
     case UIOX_KS_ERR_ROLLBACK: return "EROLLBACK";
     case UIOX_KS_ERR_EXPIRED:  return "EEXPIRED";
     case UIOX_KS_ERR_TAMPERED: return "ETAMPERED";
     default:                    return "EUNKNOWN";
     }
 }
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KSIGN_TYPES_H */
 