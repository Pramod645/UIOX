/**
 * @file  uiox_emmc_hw.c
 * @brief UIOX eMMC HAL implementation.
 * @date  2026-06-12
 */

 #include "uiox_emmc_hw.h"
 
 #define OPS(hw) ((const uiox_emmc_hw_ops_t *)(hw)->ops)
 
 int uiox_emmc_hw_init(uiox_emmc_hw_t *hw,
                        const uiox_emmc_hw_ops_t *ops)
 {
     if (!hw || !ops || !ops->init) return -EINVAL;
     hw->ops          = (void *)ops;
     hw->pending_irq   = 0u;
     hw->dev_ready     = false;
     hw->cache_enabled = false;
     hw->bus_width     = 1u;
     hw->speed         = UIOX_EMMC_HW_SPEED_IDENT;
     hw->active_part   = UIOX_EMMC_HW_PART_USER;
     hw->rca           = 0u;
     memset(&hw->ident, 0, sizeof(hw->ident));
     return ops->init(hw);
 }
 
 void uiox_emmc_hw_deinit(uiox_emmc_hw_t *hw)
 {
     if (!hw || !hw->ops) return;
     if (OPS(hw)->deinit) OPS(hw)->deinit(hw);
     hw->ops = NULL;
 }
 
 int uiox_emmc_hw_power_on(uiox_emmc_hw_t *hw)
 {
     if (!hw || !hw->ops) return -EINVAL;
     if (!OPS(hw)->power_on) return -ENOSYS;
     return OPS(hw)->power_on(hw);
 }
 
 void uiox_emmc_hw_power_off(uiox_emmc_hw_t *hw)
 {
     if (!hw || !hw->ops) return;
     if (OPS(hw)->power_off) OPS(hw)->power_off(hw);
 }
 
 uint32_t uiox_emmc_hw_reg_read(uiox_emmc_hw_t *hw, uint32_t offset)
 {
     if (!hw || !hw->ops || !OPS(hw)->reg_read) return 0u;
     return OPS(hw)->reg_read(hw, offset);
 }
 
 void uiox_emmc_hw_reg_write(uiox_emmc_hw_t *hw,
                               uint32_t offset, uint32_t val)
 {
     if (!hw || !hw->ops || !OPS(hw)->reg_write) return;
     OPS(hw)->reg_write(hw, offset, val);
 }
 
 int uiox_emmc_hw_set_clock(uiox_emmc_hw_t *hw, uint32_t hz)
 {
     if (!hw || !hw->ops) return -EINVAL;
     if (!OPS(hw)->set_clock) return -ENOSYS;
     int rc = OPS(hw)->set_clock(hw, hz);
     if (rc == 0) hw->clk_hz = hz;
     return rc;
 }
 
 int uiox_emmc_hw_set_bus_width(uiox_emmc_hw_t *hw, uint8_t width)
 {
     if (!hw || !hw->ops) return -EINVAL;
     if (!OPS(hw)->set_bus_width) return -ENOSYS;
     int rc = OPS(hw)->set_bus_width(hw, width);
     if (rc == 0) hw->bus_width = width;
     return rc;
 }
 
 int uiox_emmc_hw_set_speed(uiox_emmc_hw_t *hw, uiox_emmc_hw_speed_t speed)
 {
     if (!hw || !hw->ops) return -EINVAL;
     if (!OPS(hw)->set_speed_mode) return -ENOSYS;
     int rc = OPS(hw)->set_speed_mode(hw, speed);
     if (rc == 0) hw->speed = speed;
     return rc;
 }
 
 int uiox_emmc_hw_send_cmd(uiox_emmc_hw_t *hw, uint8_t cmd,
                            uint32_t arg, uiox_emmc_resp_t resp_type,
                            uint32_t *resp)
 {
     if (!hw || !hw->ops) return -EINVAL;
     if (!OPS(hw)->send_cmd) return -ENOSYS;
     return OPS(hw)->send_cmd(hw, cmd, arg, resp_type, resp);
 }
 
 int uiox_emmc_hw_read_blocks(uiox_emmc_hw_t *hw, uint32_t lba,
                               uint8_t *buf, uint32_t count)
 {
     if (!hw || !hw->ops || !buf) return -EINVAL;
     if (!OPS(hw)->read_blocks) return -ENOSYS;
     return OPS(hw)->read_blocks(hw, lba, buf, count);
 }
 
 int uiox_emmc_hw_write_blocks(uiox_emmc_hw_t *hw, uint32_t lba,
                                const uint8_t *buf, uint32_t count)
 {
     if (!hw || !hw->ops || !buf) return -EINVAL;
     if (!OPS(hw)->write_blocks) return -ENOSYS;
     return OPS(hw)->write_blocks(hw, lba, buf, count);
 }
 
 int uiox_emmc_hw_read_ext_csd(uiox_emmc_hw_t *hw, uint8_t *buf)
 {
     if (!hw || !hw->ops || !buf) return -EINVAL;
     if (!OPS(hw)->read_ext_csd) return -ENOSYS;
     return OPS(hw)->read_ext_csd(hw, buf);
 }
 
 int uiox_emmc_hw_write_ext_csd(uiox_emmc_hw_t *hw,
                                 uint8_t index, uint8_t val)
 {
     if (!hw || !hw->ops) return -EINVAL;
     if (!OPS(hw)->write_ext_csd) return -ENOSYS;
     return OPS(hw)->write_ext_csd(hw, index, val);
 }
 
 int uiox_emmc_hw_tuning(uiox_emmc_hw_t *hw)
 {
     if (!hw || !hw->ops) return -EINVAL;
     if (!OPS(hw)->tuning) return 0;   /* tuning optional */
     return OPS(hw)->tuning(hw);
 }
 
 uint8_t uiox_emmc_hw_crc7(uiox_emmc_hw_t *hw,
                             const uint8_t *d, uint8_t len)
 {
     if (!hw || !hw->ops || !OPS(hw)->crc7) return 0u;
     return OPS(hw)->crc7(d, len);
 }
 
 uint16_t uiox_emmc_hw_crc16(uiox_emmc_hw_t *hw,
                               const uint8_t *d, uint32_t len)
 {
     if (!hw || !hw->ops || !OPS(hw)->crc16) return 0u;
     return OPS(hw)->crc16(d, len);
 }
 
 /* ═════════════════════════════════════════════════════════════════════
 * EXT_CSD decode
 *
 * Every offset is a name from uiox_emmc_hw.h — no literals and no
 * arithmetic.  Size comes from SEC_COUNT/CACHE_SIZE (:229/:231),
 * health from the three defines at :232-234, and the support bits from
 * the EXT_CSD_* block at :207-226.
 *
 * Little-endian throughout, as JEDEC JESD84-B51 §8.4 specifies.
 *
 * A card that answers CMD1 but returns an all-0xFF EXT_CSD is real but
 * broken: the multi-byte reads come out as 0xFFFFFFFF, which
 * capacity_sectors exposes rather than hides.
 * ═════════════════════════════════════════════════════════════════════ */
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void uiox_emmc_hw_parse_ext_csd(uiox_emmc_hw_t *hw, const uint8_t *csd)
{
    if (!hw || !csd) return;

    /* ── size — what num_blocks() reports ───────────────────────────
     * SEC_COUNT is in 512-byte units, which is what BCACHE_SECTOR_SIZE
     * counts, so the block figure needs no arithmetic. */
    hw->ident.capacity_sectors = (uint64_t)le32(&csd[EXT_CSD_SEC_COUNT]);
    hw->ident.capacity_bytes   = hw->ident.capacity_sectors * 512u;

    /* ── cache ────────────────────────────────────────────────────── */
    hw->ident.cache_size_kb = le32(&csd[EXT_CSD_CACHE_SIZE]);

    /* ── health ───────────────────────────────────────────────────── */
    hw->ident.pre_eol_info = csd[EXT_CSD_PRE_EOL_INFO];
    hw->ident.life_est_a   = csd[EXT_CSD_DEVICE_LIFE_EST_A] & 0x0Fu;
    hw->ident.life_est_b   = csd[EXT_CSD_DEVICE_LIFE_EST_B] & 0x0Fu;

    /* ── support flags ───────────────────────────────────────────────
     * Each reads the byte the header names; the bit within it is the
     * standard's.  A card lacking a feature reports 0, and the identity
     * ioctl says so rather than claiming support. */
    hw->ident.cache_supported = (csd[EXT_CSD_CACHE_CTRL] & 0x10u) != 0u;
    hw->ident.pon_supported   = (csd[EXT_CSD_POWER_OFF_NOTIF] & 0x01u) != 0u;
    hw->ident.bkops_supported = (csd[EXT_CSD_BKOPS_STATUS] != 0u);
}
