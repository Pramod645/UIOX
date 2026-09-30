/**
 * @file  uiox_chg_policy.h
 * @brief UIOX Charger policy layer — USB-C PD negotiation, CV/CC profile.
 * @date  2026-06-11
 */

 #ifndef UIOX_CHG_POLICY_H
 #define UIOX_CHG_POLICY_H
 
 #include "uiox_chg_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * USB-C PD message opcodes (simplified PD 3.1 subset)
  * ====================================================================== */
 
 #define PD_MSG_REQUEST              0x02u
 #define PD_MSG_ACCEPT               0x03u
 #define PD_MSG_REJECT               0x04u
 #define PD_MSG_CAPABILITIES         0x01u
 #define PD_MSG_GET_CAPABILITIES     0x07u
 #define PD_MSG_HARD_RESET           0x06u
 #define PD_MSG_PPS_STATUS           0x26u
 
 /* Max PDO entries in a source capabilities message */
 #define UIOX_CHG_MAX_PDOS           8u
 
 /* =========================================================================
  * Power Data Object (PDO) — fixed supply
  * ====================================================================== */
 
 typedef struct {
     uint32_t voltage_mv;    /**< Fixed voltage (mV)                        */
     uint32_t current_ma;    /**< Max current (mA)                          */
     bool     pps;           /**< True = Programmable Power Supply PDO      */
     uint32_t pps_vmin_mv;   /**< PPS: min voltage (mV)                    */
     uint32_t pps_vmax_mv;   /**< PPS: max voltage (mV)                    */
     uint32_t pps_imax_ma;   /**< PPS: max current (mA)                    */
 } uiox_chg_pdo_t;
 
 /* =========================================================================
  * Charge profile (CV/CC curve)
  * ====================================================================== */
 
 typedef struct {
     uint32_t precharge_ma;   /**< Pre-charge current (mA)                  */
     uint32_t fast_charge_ma; /**< Fast-charge (CC phase) current (mA)      */
     uint32_t cv_mv;          /**< Constant-voltage target (mV)             */
     uint32_t taper_ma;       /**< Taper/termination current (mA)           */
     uint32_t iin_lim_ma;     /**< Input current limit (mA)                 */
     uint32_t vindpm_mv;      /**< VINDPM threshold (mV)                    */
 } uiox_chg_profile_t;
 
 /* =========================================================================
  * PD negotiation state machine
  * ====================================================================== */
 
 typedef enum {
     UIOX_CHG_PD_IDLE         = 0,
     UIOX_CHG_PD_WAIT_CAPS,
     UIOX_CHG_PD_EVALUATING,
     UIOX_CHG_PD_REQUESTING,
     UIOX_CHG_PD_CONTRACT_OK,
     UIOX_CHG_PD_HARD_RESET,
     UIOX_CHG_PD_ERROR,
 } uiox_chg_pd_state_t;
 
 /* =========================================================================
  * Policy context
  * ====================================================================== */
 
 typedef struct {
     uiox_chg_if_t       *cif;
     uiox_chg_profile_t   profile;
     /* PD negotiation */
     uiox_chg_pd_state_t  pd_state;
     uiox_chg_pdo_t       pdos[UIOX_CHG_MAX_PDOS];
     uint8_t              num_pdos;
     uint8_t              selected_pdo;
     uint32_t             contracted_mv;  /**< Agreed VBUS voltage (mV)    */
     uint32_t             contracted_ma;  /**< Agreed current (mA)         */
     uint32_t             pd_retry_count;
     /* Barrel-jack profile (fixed) */
     uint32_t             barrel_mv;      /**< Measured / assumed (mV)     */
     uint32_t             barrel_ma;      /**< Assumed current limit (mA)  */
 } uiox_chg_policy_t;
 
 /* =========================================================================
  * Policy API
  * ====================================================================== */
 
 int  uiox_chg_policy_init       (uiox_chg_policy_t *pol,
                                   uiox_chg_if_t *cif,
                                   const uiox_chg_profile_t *profile);
 
 /* Called when a source is detected */
 int  uiox_chg_policy_on_plug    (uiox_chg_policy_t *pol,
                                   uiox_chg_src_t src);
 
 /* Called when source is removed */
 void uiox_chg_policy_on_unplug  (uiox_chg_policy_t *pol);
 
 /* PD message received from PHY */
 int  uiox_chg_policy_pd_rx      (uiox_chg_policy_t *pol,
                                   const uint8_t *buf, uint8_t len);
 
 /* Tick: runs PD state machine + watchdog kick */
 void uiox_chg_policy_tick       (uiox_chg_policy_t *pol, uint32_t now_ms);
 
 /* Apply a profile (runtime re-configuration) */
 int  uiox_chg_policy_set_profile(uiox_chg_policy_t *pol,
                                   const uiox_chg_profile_t *profile);
 
 /* Query negotiated contract */
 bool uiox_chg_policy_pd_active  (const uiox_chg_policy_t *pol);
 uint32_t uiox_chg_policy_vbus_mv(const uiox_chg_policy_t *pol);
 uint32_t uiox_chg_policy_ibat_ma(const uiox_chg_policy_t *pol);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_CHG_POLICY_H */
 