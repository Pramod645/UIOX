/**
 * @file  uiox_ksign.h
 * @brief UIOX Signed Kernel — master umbrella include.
 * @version 1.0.0
 */

 #ifndef UIOX_KSIGN_H
 #define UIOX_KSIGN_H
 
 #include "uiox_ksign_types.h"
 #include "uiox_ksign_crypto.h"
 #include "uiox_ksign_key.h"
 #include "uiox_ksign_image.h"
 #include "uiox_ksign_verify.h"
 #include "uiox_ksign_measure.h"
 #include "uiox_ksign_runtime.h"
 
 #define UIOX_KSIGN_VERSION_STR  "UIOX ksign v1.0"
 #define UIOX_KSIGN_URL          "github.com/Pramod645/UIOX"
 
 /* Integration hook — call this from uiox_fw_main() Stage 0d */
 uiox_ks_err_t uiox_ksign_boot_verify(const void *kernel_image,
                                         size_t image_size,
                                         bool sim_mode);
 
 /* Integration hook — call from main.c Stage 7 */
 uiox_ks_err_t uiox_ksign_runtime_start(uintptr_t text_base,
                                           size_t text_size,
                                           uintptr_t rodata_base,
                                           size_t rodata_size);
 
 /* Integration hook — call from scheduler tick */
 void          uiox_ksign_tick(uint64_t now_ms);
 
 /* Global context accessors */
 uiox_ks_verify_ctx_t  *uiox_ksign_get_verify_ctx (void);
 uiox_ks_measure_ctx_t *uiox_ksign_get_measure_ctx(void);
 uiox_ks_rt_ctx_t      *uiox_ksign_get_rt_ctx     (void);
 
 #endif /* UIOX_KSIGN_H */
 