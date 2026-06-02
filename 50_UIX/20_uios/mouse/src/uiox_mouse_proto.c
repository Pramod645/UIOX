/**
 * @file    uiox_mouse_proto.c
 * @brief   UIOX Mouse protocol layer implementation.
 * @date    2026-06-01
 */

 #include "uiox_mouse_proto.h"
 #include <string.h>
 #include <errno.h>
 
 int uiox_mouse_proto_init(uiox_mouse_proto_ctx_t *ctx,
                            uiox_mouse_if_t        *mif,
                            uiox_mouse_proto_t      proto)
 {
     if (!ctx || !mif) return -EINVAL;
     memset(ctx, 0, sizeof(*ctx));
     ctx->mif    = mif;
     ctx->proto  = proto;
 
     switch (proto) {
     case UIOX_MOUSE_PROTO_PS2_STD:
         ctx->ps2_expected = 3u; break;
     case UIOX_MOUSE_PROTO_PS2_INTELLI:
         ctx->ps2_expected = 4u; break;
     case UIOX_MOUSE_PROTO_HID_BOOT:
         ctx->btn_byte_off   = 0u;
         ctx->x_byte_off     = 1u;
         ctx->y_byte_off     = 2u;
         ctx->wheel_byte_off = 3u;
         break;
     case UIOX_MOUSE_PROTO_SERIAL_MS:
         break;
     default:
         break;
     }
     ctx->primed = true;
     return 0;
 }
 
 bool uiox_mouse_proto_ps2_valid(uint8_t first_byte)
 {
     /* PS/2 bit 3 (always-1 sync bit) must be set */
     return (first_byte & 0x08u) != 0u;
 }
 
 int uiox_mouse_proto_decode(uiox_mouse_proto_ctx_t *ctx,
                              const uint8_t *data, uint8_t len,
                              uiox_mouse_raw_t *raw_out)
 {
     if (!ctx || !data || !raw_out || !len) return -EINVAL;
     memset(raw_out, 0, sizeof(*raw_out));
     raw_out->connected = true;
 
     switch (ctx->proto) {
 
     /* ------------------------------------------------------------------
      * USB HID Boot Protocol (3-byte fixed):
      *   Byte 0: buttons [L|R|M|B4|B5|0|0|0]
      *   Byte 1: X displacement (signed)
      *   Byte 2: Y displacement (signed)
      *   Byte 3: Wheel (optional, signed)
      * ----------------------------------------------------------------*/
     case UIOX_MOUSE_PROTO_HID_BOOT:
         if (len < 3u) return 0;
         raw_out->buttons = data[0] & 0x1Fu;
         raw_out->dx      = (int16_t)(int8_t)data[1];
         raw_out->dy      = (int16_t)(int8_t)data[2];
         raw_out->dz      = (len >= 4u) ? (int8_t)data[3] : 0;
         memcpy(raw_out->raw, data, len < UIOX_MOUSE_RAW_BUF_LEN ? len : UIOX_MOUSE_RAW_BUF_LEN);
         raw_out->raw_len = len;
         return 1;
 
     /* ------------------------------------------------------------------
      * PS/2 Standard (3 bytes) / IntelliMouse (4 bytes):
      *   Byte 0: [Y-OVF|X-OVF|Y-SGN|X-SGN|1|M|R|L]
      *   Byte 1: X movement (2's complement)
      *   Byte 2: Y movement (2's complement, inverted)
      *   Byte 3: [0|0|0|0|W3|W2|W1|W0] scroll (IntelliMouse)
      * ----------------------------------------------------------------*/
     case UIOX_MOUSE_PROTO_PS2_STD:
     case UIOX_MOUSE_PROTO_PS2_INTELLI: {
         /* Accumulate bytes */
         for (uint8_t i = 0; i < len && ctx->ps2_len < 4u; i++) {
             if (ctx->ps2_len == 0 && !uiox_mouse_proto_ps2_valid(data[i]))
                 continue;  /* Sync error — skip byte */
             ctx->ps2_buf[ctx->ps2_len++] = data[i];
         }
         if (ctx->ps2_len < ctx->ps2_expected) return 0; /* Need more bytes */
 
         uint8_t *b = ctx->ps2_buf;
         /* Buttons: L=bit0, R=bit1, M=bit2 */
         raw_out->buttons = b[0] & 0x07u;
         /* X: sign-extended from 9-bit (bit 4 of byte 0 = sign) */
         raw_out->dx = (int16_t)(int8_t)b[1];
         if (b[0] & 0x10u) raw_out->dx |= (int16_t)0xFF00;
         /* Y: sign-extended, PS/2 Y is inverted vs screen */
         raw_out->dy = -(int16_t)(int8_t)b[2];
         if (b[0] & 0x20u) raw_out->dy = -(raw_out->dy | (int16_t)0xFF00);
         /* Scroll (IntelliMouse) */
         if (ctx->ps2_expected == 4u)
             raw_out->dz = (int8_t)(b[3] & 0x0Fu) - ((b[3] & 0x08u) ? 16 : 0);
 
         memcpy(raw_out->raw, b, ctx->ps2_len);
         raw_out->raw_len = ctx->ps2_len;
         ctx->ps2_len = 0;   /* reset for next packet */
         return 1;
     }
 
     /* ------------------------------------------------------------------
      * Serial Microsoft 2-button (3 bytes):
      *   Byte 0: 0x40 | (L<<5) | (R<<4) | Y[7:6] | X[7:6]
      *   Byte 1: X[5:0]
      *   Byte 2: Y[5:0]
      * ----------------------------------------------------------------*/
     case UIOX_MOUSE_PROTO_SERIAL_MS: {
         for (uint8_t i = 0; i < len && ctx->serial_len < 3u; i++) {
             if (ctx->serial_len == 0 && !(data[i] & 0x40u)) continue;
             ctx->serial_buf[ctx->serial_len++] = data[i];
         }
         if (ctx->serial_len < 3u) return 0;
         uint8_t *b = ctx->serial_buf;
         raw_out->buttons = ((b[0] >> 4u) & 0x02u) | ((b[0] >> 5u) & 0x01u);
         int8_t x = (int8_t)(((b[0] & 0x03u) << 6u) | (b[1] & 0x3Fu));
         int8_t y = (int8_t)(((b[0] & 0x0Cu) << 4u) | (b[2] & 0x3Fu));
         raw_out->dx = (int16_t)x;
         raw_out->dy = (int16_t)y;
         ctx->serial_len = 0;
         return 1;
     }
 
     /* ------------------------------------------------------------------
      * Synaptics / ELAN (I2C): coordinates come as absolute, convert to delta
      * ----------------------------------------------------------------*/
     case UIOX_MOUSE_PROTO_SYNAPTICS:
     case UIOX_MOUSE_PROTO_ELAN:
         if (len < 6u) return 0;
         raw_out->dx = (int16_t)(((data[1] & 0x1Fu) << 8u) | data[2]);
         raw_out->dy = (int16_t)(((data[4] & 0x1Fu) << 8u) | data[5]);
         raw_out->buttons = data[0] & 0x03u;
         memcpy(raw_out->raw, data, 6u);
         raw_out->raw_len = 6u;
         return 1;
 
     default:
         return -ENOTSUP;
     }
 }
 
 int uiox_mouse_proto_ps2_cmd(uiox_mouse_proto_ctx_t *ctx,
                               uint8_t cmd, uint8_t *ack_out)
 {
     if (!ctx || !ctx->mif || !ctx->mif->hw) return -EINVAL;
     const uiox_mouse_hw_ops_t *ops =
         (const uiox_mouse_hw_ops_t *)ctx->mif->hw->priv;
     if (!ops || !ops->i2c_write) return -ENOSYS;
     /* For PS/2 over I2C adapter, write single command byte */
     return ops->i2c_write(ctx->mif->hw, 0x00u, &cmd, 1u);
 }
 