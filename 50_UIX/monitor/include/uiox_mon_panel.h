/**
 * @file    uiox_mon_panel.h
 * @brief   UIOX Monitor panel abstraction (EDID, modes, power sequence).
 *
 * Manages display panel identity and capabilities:
 *   - EDID block parsing (CEA-861 / E-EDID 1.4)
 *   - Standard timing mode database
 *   - Panel power-on / power-off sequencing
 *   - Preferred / native resolution selection
 *   - Panel-specific init sequences (MIPI DSI panels)
 *
 * @date    2026-05-27
 */
//Layer 2b — Panel Abstraction
 #ifndef UIOX_MON_PANEL_H
 #define UIOX_MON_PANEL_H
 
 #include "uiox_mon_hw.h"
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_MON_EDID_SIZE        128
 #define UIOX_MON_MAX_MODES        32
 #define UIOX_MON_PANEL_NAME_MAX   14
 
 /* =========================================================================
  * Display mode
  * ====================================================================== */
 
 typedef struct {
     uiox_mon_timing_t  timing;
     bool               preferred;    /**< EDID preferred mode             */
     bool               native;       /**< Native panel resolution         */
     bool               cea;          /**< CEA-861 VIC mode                */
     uint8_t            vic;          /**< CEA VIC code (0 = not CEA)     */
 } uiox_mon_mode_t;
 
 /* =========================================================================
  * EDID parsed data
  * ====================================================================== */
 
 typedef struct {
     uint8_t   raw[UIOX_MON_EDID_SIZE];   /**< Raw EDID bytes              */
     bool      valid;                       /**< Checksum passed            */
     char      manufacturer[4];             /**< 3-char + null              */
     uint16_t  product_code;
     uint32_t  serial;
     char      name[UIOX_MON_PANEL_NAME_MAX];
     uint16_t  h_size_mm;           /**< Physical width (mm)               */
     uint16_t  v_size_mm;           /**< Physical height (mm)              */
     uint8_t   gamma_x100;          /**< Display gamma × 100              */
     bool      dpms_standby;
     bool      dpms_suspend;
     bool      dpms_off;
     bool      digital;             /**< Digital (vs analog) input         */
     uint8_t   color_depth;         /**< Bits per colour (6/8/10/12/16)   */
     uiox_mon_mode_t modes[UIOX_MON_MAX_MODES];
     uint8_t   num_modes;
 } uiox_mon_edid_t;
 
 /* =========================================================================
  * Panel descriptor
  * ====================================================================== */
 
 typedef struct {
     const char          *name;
     uiox_mon_edid_t      edid;
     uiox_mon_mode_t      current_mode;
     bool                 powered;
 
     /* Panel power sequence delays (ms) */
     uint32_t             power_on_delay_ms;
     uint32_t             power_off_delay_ms;
     uint32_t             backlight_on_delay_ms;
 
     /* Fixed panel timing (for embedded panels without EDID) */
     const uiox_mon_timing_t *fixed_timing;
 } uiox_mon_panel_t;
 
 /* =========================================================================
  * Panel API
  * ====================================================================== */
 
 /** Probe panel: read EDID and populate panel descriptor. */
 int  uiox_mon_panel_probe   (uiox_mon_panel_t *panel, uiox_mon_hw_t *hw);
 
 /** Parse raw EDID bytes into panel->edid structure. */
 int  uiox_mon_panel_parse_edid(uiox_mon_panel_t *panel);
 
 /** Select best mode for given preferred resolution (0,0 = native). */
 int  uiox_mon_panel_select_mode(uiox_mon_panel_t *panel,
                                  uint16_t pref_w, uint16_t pref_h,
                                  uint8_t  pref_hz);
 
 /** Execute panel power-on sequence. */
 int  uiox_mon_panel_power_on (uiox_mon_panel_t *panel, uiox_mon_hw_t *hw);
 
 /** Execute panel power-off sequence. */
 void uiox_mon_panel_power_off(uiox_mon_panel_t *panel, uiox_mon_hw_t *hw);
 
 /** Return pointer to currently selected mode timing. */
 const uiox_mon_timing_t *uiox_mon_panel_timing(const uiox_mon_panel_t *p);
 
 /** Dump panel info to stdout. */
 void uiox_mon_panel_print   (const uiox_mon_panel_t *panel);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_MON_PANEL_H */
 