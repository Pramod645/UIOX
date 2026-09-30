/**
 * @file  uiox_chg_policy.c
 * @brief UIOX Charger policy — USB-C PD negotiation, CV/CC profile.
 * @date  2026-06-11
 */

 #include "uiox_chg_policy.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 int uiox_chg_policy_init(uiox_chg_policy_t *pol,
                           uiox_chg_if_t *cif,
                           const uiox_chg_profile_t *profile)
 {
     if (!pol || !cif || !profile) return -EINVAL;
     memset(pol, 0, sizeof(*pol));
     pol->cif      = cif;
     pol->profile  = *profile;
     pol->pd_state = UIOX_CHG_PD_IDLE;
     pol->barrel_mv = 19000u;  /* Common 19 V barrel default */
     pol->barrel_ma =  3000u;  /* 57 W barrel default        */
     return 0;
 }
 
 int uiox_chg_policy_on_plug(uiox_chg_policy_t *pol, uiox_chg_src_t src)
 {
     if (!pol) return -EINVAL;
 
     if (src == UIOX_CHG_SRC_USBC_PD) {
         /* Begin PD negotiation: request source capabilities */
         pol->pd_state   = UIOX_CHG_PD_WAIT_CAPS;
         pol->num_pdos   = 0u;
         pol->pd_retry_count = 0u;
         uint8_t msg[2]  = { PD_MSG_GET_CAPABILITIES, 0x00u };
         int rc = uiox_chg_if_pd_tx(pol->cif, msg, sizeof(msg));
         printf("  [policy] PD: sent GET_CAPABILITIES  rc=%d\n", rc);
         return rc;
     }
 
     if (src == UIOX_CHG_SRC_BARREL) {
         /* Barrel jack: apply fixed profile directly */
         printf("  [policy] Barrel jack: %u mV / %u mA\n",
                pol->barrel_mv, pol->barrel_ma);
         pol->contracted_mv = pol->barrel_mv;
         pol->contracted_ma = pol->barrel_ma;
         uiox_chg_if_set_iin_lim(pol->cif, pol->profile.iin_lim_ma);
         uiox_chg_if_set_ichg(pol->cif,
                               pol->profile.fast_charge_ma);
         uiox_chg_if_set_vchg(pol->cif, pol->profile.cv_mv);
         return 0;
     }
 
     /* USB Standard / BC1.2 */
     uint32_t iin = (src == UIOX_CHG_SRC_USB_DCP)  ? 1500u
                  : (src == UIOX_CHG_SRC_USB_CDP)  ? 1500u
                  : 500u;
     pol->contracted_mv = 5000u;
     pol->contracted_ma = iin;
     uiox_chg_if_set_iin_lim(pol->cif, iin);
     uiox_chg_if_set_ichg(pol->cif,
                           (iin < pol->profile.fast_charge_ma)
                           ? iin
                           : pol->profile.fast_charge_ma);
     uiox_chg_if_set_vchg(pol->cif, pol->profile.cv_mv);
     printf("  [policy] USB std: iin=%u mA\n", iin);
     return 0;
 }
 
 void uiox_chg_policy_on_unplug(uiox_chg_policy_t *pol)
 {
     if (!pol) return;
     pol->pd_state      = UIOX_CHG_PD_IDLE;
     pol->contracted_mv = 0u;
     pol->contracted_ma = 0u;
     pol->num_pdos      = 0u;
     printf("  [policy] Source removed — PD state reset\n");
 }
 
 int uiox_chg_policy_pd_rx(uiox_chg_policy_t *pol,
                             const uint8_t *buf, uint8_t len)
 {
     if (!pol || !buf || len < 1u) return -EINVAL;
 
     uint8_t opcode = buf[0] & 0x1Fu;
     printf("  [policy] PD RX opcode=0x%02X  state=%d\n",
            opcode, (int)pol->pd_state);
 
     if (opcode == PD_MSG_CAPABILITIES &&
         pol->pd_state == UIOX_CHG_PD_WAIT_CAPS) {
         /*
          * Parse PDOs from source capabilities message.
          * Wire format: buf[0]=opcode, buf[1..N] = 4-byte PDO entries.
          */
         pol->num_pdos = 0u;
         uint8_t pdo_bytes = (uint8_t)(len > 1u ? len - 1u : 0u);
         uint8_t n = (uint8_t)(pdo_bytes / 4u);
         if (n > UIOX_CHG_MAX_PDOS) n = UIOX_CHG_MAX_PDOS;
 
         for (uint8_t i = 0u; i < n; i++) {
             const uint8_t *p = &buf[1u + i * 4u];
             uint32_t raw = (uint32_t)p[0]
                          | ((uint32_t)p[1] << 8u)
                          | ((uint32_t)p[2] << 16u)
                          | ((uint32_t)p[3] << 24u);
 
             uiox_chg_pdo_t *pdo = &pol->pdos[pol->num_pdos++];
             /* Fixed-supply PDO decode (bits [19:10]=voltage, [9:0]=current) */
             pdo->voltage_mv = ((raw >> 10u) & 0x3FFu) * 50u;
             pdo->current_ma = (raw & 0x3FFu) * 10u;
             pdo->pps        = false;
             printf("  [policy]   PDO[%u]: %u mV / %u mA\n",
                    i, pdo->voltage_mv, pdo->current_ma);
         }
 
         /* Select best PDO: highest voltage within hw limit */
         uint8_t best = 0u;
         uint32_t best_pwr = 0u;
         for (uint8_t i = 0u; i < pol->num_pdos; i++) {
             uiox_chg_pdo_t *pdo = &pol->pdos[i];
             if (pdo->voltage_mv > pol->cif->hw->vbus_max_mv) continue;
             uint32_t pwr = pdo->voltage_mv / 1000u *
                            pdo->current_ma / 1000u;
             if (pwr > best_pwr) { best_pwr = pwr; best = i; }
         }
         pol->selected_pdo = best;
 
         /* Send Request */
         pol->pd_state = UIOX_CHG_PD_REQUESTING;
         uint8_t req[5];
         req[0] = PD_MSG_REQUEST;
         req[1] = (uint8_t)(best + 1u);   /* PDO position (1-based) */
         req[2] = (uint8_t)(pol->pdos[best].current_ma / 10u);
         req[3] = (uint8_t)(pol->pdos[best].voltage_mv / 50u);
         req[4] = 0x00u;
         uiox_chg_if_pd_tx(pol->cif, req, sizeof(req));
         printf("  [policy] PD: REQUEST PDO[%u] %u mV %u mA\n",
                best, pol->pdos[best].voltage_mv,
                pol->pdos[best].current_ma);
         return 0;
     }
 
     if (opcode == PD_MSG_ACCEPT &&
         pol->pd_state == UIOX_CHG_PD_REQUESTING) {
         pol->contracted_mv = pol->pdos[pol->selected_pdo].voltage_mv;
         pol->contracted_ma = pol->pdos[pol->selected_pdo].current_ma;
         pol->pd_state      = UIOX_CHG_PD_CONTRACT_OK;
         printf("  [policy] PD contract: %u mV / %u mA\n",
                pol->contracted_mv, pol->contracted_ma);
         /* Apply contracted current */
         uint32_t iin = pol->contracted_ma;
         if (iin > pol->cif->hw->iin_max_ma)
             iin = pol->cif->hw->iin_max_ma;
         uiox_chg_if_set_iin_lim(pol->cif, iin);
         uiox_chg_if_set_ichg(pol->cif, pol->profile.fast_charge_ma);
         uiox_chg_if_set_vchg(pol->cif, pol->profile.cv_mv);
         return 0;
     }
 
     if (opcode == PD_MSG_REJECT || opcode == PD_MSG_HARD_RESET) {
         pol->pd_state = UIOX_CHG_PD_HARD_RESET;
         printf("  [policy] PD HARD RESET / REJECT received\n");
         /* Fall back to 5 V / 900 mA */
         pol->contracted_mv = 5000u;
         pol->contracted_ma =  900u;
         uiox_chg_if_set_iin_lim(pol->cif, 900u);
         uiox_chg_if_set_ichg(pol->cif, 900u);
         return -EPROTO;
     }
     return 0;
 }
 
 void uiox_chg_policy_tick(uiox_chg_policy_t *pol, uint32_t now_ms)
 {
     if (!pol) return;
     (void)now_ms;
 
     /* Kick watchdog every tick */
     uiox_chg_hw_wdog_reset(pol->cif->hw);
     pol->cif->stats.wdog_kicks++;
 
     /* PD timeout: retry if no caps received */
     if (pol->pd_state == UIOX_CHG_PD_WAIT_CAPS) {
         pol->pd_retry_count++;
         if (pol->pd_retry_count > 50u) {    /* ~500 ms at 10 ms tick */
             printf("  [policy] PD caps timeout — fallback 5V\n");
             pol->contracted_mv  = 5000u;
             pol->contracted_ma  =  900u;
             pol->pd_state       = UIOX_CHG_PD_ERROR;
             uiox_chg_if_set_iin_lim(pol->cif, 900u);
             uiox_chg_if_set_ichg(pol->cif, 900u);
         }
     }
 }
 
 int uiox_chg_policy_set_profile(uiox_chg_policy_t *pol,
                                   const uiox_chg_profile_t *profile)
 {
     if (!pol || !profile) return -EINVAL;
     pol->profile = *profile;
     /* Re-apply if already in contract */
     if (pol->pd_state == UIOX_CHG_PD_CONTRACT_OK ||
         pol->contracted_mv > 0u) {
         uiox_chg_if_set_ichg(pol->cif, profile->fast_charge_ma);
         uiox_chg_if_set_vchg(pol->cif, profile->cv_mv);
         uiox_chg_if_set_iin_lim(pol->cif, profile->iin_lim_ma);
     }
     return 0;
 }
 
 bool uiox_chg_policy_pd_active(const uiox_chg_policy_t *pol)
 { return pol && pol->pd_state == UIOX_CHG_PD_CONTRACT_OK; }
 
 uint32_t uiox_chg_policy_vbus_mv(const uiox_chg_policy_t *pol)
 { return pol ? pol->contracted_mv : 0u; }
 
 uint32_t uiox_chg_policy_ibat_ma(const uiox_chg_policy_t *pol)
 { return pol ? pol->profile.fast_charge_ma : 0u; }
 