/**
 * @file    uiox_pmic_rail.h
 * @brief   UIOX PMIC rail manager (buck, LDO, load switch).
 * @date    2026-06-04
 */
//Layer 2b — Rail Manager
 #ifndef UIOX_PMIC_RAIL_H
 #define UIOX_PMIC_RAIL_H
 
 #include "uiox_pmic_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_PMIC_RAIL_NAME_MAX  16
 #define UIOX_PMIC_MAX_RAIL_COUNT 16
 
 /* =========================================================================
  * Rail type
  * ====================================================================== */
 
 typedef enum {
     UIOX_PMIC_RAIL_BUCK    = 0,
     UIOX_PMIC_RAIL_LDO,
     UIOX_PMIC_RAIL_SWITCH,
     UIOX_PMIC_RAIL_BOOST,
 } uiox_pmic_rail_type_t;
 
 /* =========================================================================
  * Rail descriptor
  * ====================================================================== */
 
 typedef struct {
     char                  name[UIOX_PMIC_RAIL_NAME_MAX];
     uiox_pmic_rail_type_t type;
     uint8_t               rail_id;     /**< Hardware rail index            */
     uint16_t              vset_reg;    /**< Voltage set register           */
     uint16_t              ctrl_reg;    /**< Enable/mode control register   */
     uint8_t               en_bit;      /**< Enable bit in ctrl_reg         */
     uint8_t               vset_mask;   /**< Voltage field mask             */
     uint32_t              min_mv;      /**< Minimum voltage (mV)           */
     uint32_t              max_mv;      /**< Maximum voltage (mV)           */
     uint32_t              step_mv;     /**< Voltage step (mV)              */
     uint32_t              cur_mv;      /**< Current set voltage (mV)       */
     uint32_t              boot_mv;     /**< Default boot voltage           */
     uint32_t              max_ma;      /**< Maximum current (mA)           */
     bool                  enabled;
     bool                  always_on;   /**< Cannot be disabled             */
     uint8_t               consumers;   /**< Reference count                */
 } uiox_pmic_rail_t;
 
 /* =========================================================================
  * Rail table
  * ====================================================================== */
 
 typedef struct {
     uiox_pmic_if_t   *pif;
     uiox_pmic_rail_t  rails[UIOX_PMIC_MAX_RAIL_COUNT];
     uint8_t           num_rails;
 } uiox_pmic_rail_mgr_t;
 
 /* =========================================================================
  * Rail API
  * ====================================================================== */
 
 int  uiox_pmic_rail_init      (uiox_pmic_rail_mgr_t *mgr,
                                 uiox_pmic_if_t *pif);
 
 /** Register a rail descriptor. */
 int  uiox_pmic_rail_register  (uiox_pmic_rail_mgr_t *mgr,
                                 const uiox_pmic_rail_t *rail);
 
 /** Enable a rail (reference-counted). */
 int  uiox_pmic_rail_enable    (uiox_pmic_rail_mgr_t *mgr,
                                 const char *name);
 
 /** Disable a rail (release reference). */
 int  uiox_pmic_rail_disable   (uiox_pmic_rail_mgr_t *mgr,
                                 const char *name);
 
 /** Set voltage on a rail. */
 int  uiox_pmic_rail_set_mv    (uiox_pmic_rail_mgr_t *mgr,
                                 const char *name, uint32_t mv);
 
 /** Get current voltage on a rail. */
 int  uiox_pmic_rail_get_mv    (const uiox_pmic_rail_mgr_t *mgr,
                                 const char *name, uint32_t *mv_out);
 
 /** Find rail by name. */
 uiox_pmic_rail_t *uiox_pmic_rail_find(uiox_pmic_rail_mgr_t *mgr,
                                        const char *name);
 
 /** Restore all rails to boot voltages. */
 void uiox_pmic_rail_restore_all(uiox_pmic_rail_mgr_t *mgr);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_PMIC_RAIL_H */
 