/**
 * @file    uiox_ram_ecc.h
 * @brief   UIOX RAM ECC error correction and scrubbing engine.
 * @date    2026-06-03
 */

 #ifndef UIOX_RAM_ECC_H
 #define UIOX_RAM_ECC_H
 
 #include "uiox_ram_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_RAM_ECC_LOG_SIZE    64
 
 typedef enum {
     UIOX_RAM_ECC_CORRECTABLE   = 0,
     UIOX_RAM_ECC_UNCORRECTABLE,
     UIOX_RAM_ECC_MULTI_BIT,
 } uiox_ram_ecc_type_t;
 
 typedef struct {
     uiox_ram_ecc_type_t type;
     uint64_t            addr;
     uint64_t            ts_ms;
     uint32_t            syndrome;   /**< ECC syndrome bits                 */
     uint8_t             bit_pos;    /**< Corrected bit position            */
 } uiox_ram_ecc_entry_t;
 
 typedef struct {
     uiox_ram_if_t       *rif;
     uiox_ram_ecc_entry_t log[UIOX_RAM_ECC_LOG_SIZE];
     uint8_t              log_head;
     uint32_t             total_ce;  /**< Total correctable errors          */
     uint32_t             total_ue;  /**< Total uncorrectable errors        */
     uint64_t             scrub_pos; /**< Current scrub position (phys)    */
     uint64_t             scrub_end;
     uint32_t             scrub_chunk_bytes; /**< Bytes per scrub tick      */
     bool                 scrub_running;
 } uiox_ram_ecc_t;
 
 int  uiox_ram_ecc_init    (uiox_ram_ecc_t *ecc, uiox_ram_if_t *rif);
 void uiox_ram_ecc_tick    (uiox_ram_ecc_t *ecc, uint32_t now_ms);
 int  uiox_ram_ecc_start_scrub(uiox_ram_ecc_t *ecc,
                                uint64_t phys_start, uint64_t size,
                                uint32_t chunk_bytes);
 void uiox_ram_ecc_poll    (uiox_ram_ecc_t *ecc);
 void uiox_ram_ecc_print_log(const uiox_ram_ecc_t *ecc);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_RAM_ECC_H */
 