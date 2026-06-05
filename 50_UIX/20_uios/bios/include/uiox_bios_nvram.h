/**
 * @file    uiox_bios_nvram.h
 * @brief   UIOX BIOS NVRAM: EFI variables, CMOS, ESCD.
 *
 * Manages:
 *   - EFI variable store (authenticated variable store in flash)
 *   - CMOS NVRAM (legacy RTC CMOS bytes 0x00..0x7F)
 *   - Boot order, setup options, Secure Boot keys
 *   - Variable GUIDs and attributes
 *
 * @date    2026-06-04
 */
//Layer 2b — NVRAM
 #ifndef UIOX_BIOS_NVRAM_H
 #define UIOX_BIOS_NVRAM_H
 
 #include "uiox_bios_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 #define UIOX_BIOS_VAR_NAME_MAX    64u
 #define UIOX_BIOS_VAR_DATA_MAX    1024u
 #define UIOX_BIOS_VAR_STORE_SIZE  (64u * 1024u) /**< 64 KB EFI var store   */
 #define UIOX_BIOS_MAX_VARS        64
 
 /* =========================================================================
  * EFI variable attributes
  * ====================================================================== */
 
 #define UIOX_EFI_VAR_NV         (1u << 0)  /**< Non-volatile             */
 #define UIOX_EFI_VAR_BS         (1u << 1)  /**< Boot services access     */
 #define UIOX_EFI_VAR_RT         (1u << 2)  /**< Runtime services access  */
 #define UIOX_EFI_VAR_AUTH       (1u << 3)  /**< Authenticated write      */
 #define UIOX_EFI_VAR_HW_ERR     (1u << 4)  /**< Hardware error record    */
 
 /* =========================================================================
  * EFI GUID (simplified 128-bit)
  * ====================================================================== */
 
 typedef struct {
     uint32_t data1;
     uint16_t data2;
     uint16_t data3;
     uint8_t  data4[8];
 } uiox_efi_guid_t;
 
 /* Well-known GUIDs */
 #define UIOX_GUID_GLOBAL   { 0x8BE4DF61u,0x93CAu,0x11D2u,{0xAA,0x0D,0x00,0xE0,0x98,0x03,0x2B,0x8C} }
 #define UIOX_GUID_SETUP    { 0xEC87D643u,0xEBA4u,0x4BB5u,{0xA1,0xE5,0x3F,0x3E,0x36,0xB2,0x0D,0xA9} }
 
 /* =========================================================================
  * EFI variable entry
  * ====================================================================== */
 
 typedef struct {
     uiox_efi_guid_t guid;
     char            name[UIOX_BIOS_VAR_NAME_MAX];
     uint8_t         data[UIOX_BIOS_VAR_DATA_MAX];
     uint32_t        data_size;
     uint32_t        attributes;
     bool            valid;
     bool            deleted;
 } uiox_bios_var_t;
 
 /* =========================================================================
  * CMOS layout (selected indices)
  * ====================================================================== */
 
 #define UIOX_CMOS_RTC_SECONDS   0x00u
 #define UIOX_CMOS_RTC_MINUTES   0x02u
 #define UIOX_CMOS_RTC_HOURS     0x04u
 #define UIOX_CMOS_RTC_DAY       0x07u
 #define UIOX_CMOS_RTC_MONTH     0x08u
 #define UIOX_CMOS_RTC_YEAR      0x09u
 #define UIOX_CMOS_STATUS_A      0x0Au
 #define UIOX_CMOS_STATUS_B      0x0Bu
 #define UIOX_CMOS_BOOT_DEV      0x3Du  /**< Custom: boot device index    */
 #define UIOX_CMOS_CHECKSUM_LO   0x2Eu
 #define UIOX_CMOS_CHECKSUM_HI   0x2Fu
 
 /* =========================================================================
  * NVRAM context
  * ====================================================================== */
 
 typedef struct {
     uiox_bios_if_t   *bif;
     uiox_bios_var_t   vars[UIOX_BIOS_MAX_VARS];
     uint8_t           var_count;
     uint32_t          store_offset; /**< Flash offset of EFI var store    */
     bool              dirty;        /**< Needs flush to flash             */
 } uiox_bios_nvram_t;
 
 /* =========================================================================
  * NVRAM API
  * ====================================================================== */
 
 int  uiox_bios_nvram_init        (uiox_bios_nvram_t *nv,
                                    uiox_bios_if_t    *bif,
                                    uint32_t           store_offset);
 
 /** Load EFI variable store from flash into RAM cache. */
 int  uiox_bios_nvram_load        (uiox_bios_nvram_t *nv);
 
 /** Flush dirty variables back to flash. */
 int  uiox_bios_nvram_flush       (uiox_bios_nvram_t *nv);
 
 /** Get an EFI variable by GUID + name. */
 int  uiox_bios_nvram_get_var     (uiox_bios_nvram_t    *nv,
                                    const uiox_efi_guid_t *guid,
                                    const char           *name,
                                    void                 *data_out,
                                    uint32_t             *size_out,
                                    uint32_t             *attrs_out);
 
 /** Set / create an EFI variable. */
 int  uiox_bios_nvram_set_var     (uiox_bios_nvram_t    *nv,
                                    const uiox_efi_guid_t *guid,
                                    const char           *name,
                                    const void           *data,
                                    uint32_t              size,
                                    uint32_t              attrs);
 
 /** Delete an EFI variable. */
 int  uiox_bios_nvram_del_var     (uiox_bios_nvram_t    *nv,
                                    const uiox_efi_guid_t *guid,
                                    const char           *name);
 
 /** Read CMOS byte. */
 uint8_t uiox_bios_nvram_cmos_get (uiox_bios_nvram_t *nv, uint8_t index);
 
 /** Write CMOS byte (updates checksum). */
 void uiox_bios_nvram_cmos_set    (uiox_bios_nvram_t *nv,
                                    uint8_t index, uint8_t val);
 
 /** Verify CMOS checksum. */
 bool uiox_bios_nvram_cmos_valid  (uiox_bios_nvram_t *nv);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_BIOS_NVRAM_H */
 