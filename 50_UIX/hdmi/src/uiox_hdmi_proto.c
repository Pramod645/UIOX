/**
 * @file    uiox_hdmi_proto.c
 * @brief   UIOX HDMI protocol layer implementation.
 * @date    2026-05-28
 */

 #include "uiox_hdmi_proto.h"
 #include <string.h>
 #include <errno.h>
 
 /* -------------------------------------------------------------------------
  * Infoframe checksum: sum of all bytes (type+version+length+payload) = 0
  * ---------------------------------------------------------------------- */
 
 static uint8_t infoframe_checksum(const uint8_t *buf, uint8_t len)
 {
     uint8_t sum = 0;
     for (uint8_t i = 0; i < len; i++) sum += buf[i];
     return (uint8_t)(0x100u - sum);
 }
 
 int uiox_hdmi_proto_init(uiox_hdmi_proto_t *proto,
                           uiox_hdmi_if_t    *hif,
                           uiox_hdmi_sink_t  *sink)
 {
     if (!proto || !hif || !sink) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->hif  = hif;
     proto->sink = sink;
     proto->cec_la                 = UIOX_CEC_LA_PLAYBACK1;
     proto->vendor_id              = 0x000000u;  /* No vendor              */
     proto->osd_name               = "UIOX";
     proto->infoframe_interval_ms  = 1000u;      /* Resend every 1 second  */
     return 0;
 }
 
 int uiox_hdmi_proto_send_avi(uiox_hdmi_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     uiox_hdmi_pkt_t *pkt = uiox_hdmi_buf_alloc_pkt();
     if (!pkt) return -ENOMEM;
 
     pkt->type    = UIOX_HDMI_PKT_AVI;
     pkt->version = 2u;
     pkt->length  = 13u;
 
     /* Byte 1: Y[1:0] A C[1:0] M[1:0] R[3:0] */
     uiox_hdmi_colorspace_t cs = proto->hif->cs;
     uint8_t Y = (cs == UIOX_HDMI_CS_RGB)      ? 0u :
                 (cs == UIOX_HDMI_CS_YCbCr444)  ? 2u :
                 (cs == UIOX_HDMI_CS_YCbCr422)  ? 1u : 3u; /* YCbCr420=3 */
     pkt->payload[0] = (uint8_t)((Y << 5u) |
                                  (proto->avi.active_format ? 0x10u : 0u) |
                                   proto->avi.bar_info << 2u |
                                   proto->avi.scan_info);
     /* Byte 2: C[1:0] M[1:0] R[3:0] */
     pkt->payload[1] = (uint8_t)((proto->avi.colorimetry << 6u) |
                                  (proto->avi.picture_aspect << 4u) |
                                   proto->avi.active_aspect);
     /* Byte 3: IT CN PR */
     pkt->payload[2] = (uint8_t)((proto->avi.it_content ? 0x80u : 0u) |
                                  (proto->avi.ext_colorimetry << 4u) |
                                  (proto->avi.rgb_quant << 2u) |
                                   proto->avi.nups);
     /* Byte 4: VIC */
     pkt->payload[3] = proto->sink->current_mode.vic;
     /* Bytes 5-13: bar data */
     pkt->payload[4]  = proto->avi.pr & 0x0Fu;
     pkt->payload[5]  = (uint8_t)(proto->avi.top & 0xFFu);
     pkt->payload[6]  = (uint8_t)(proto->avi.top >> 8u);
     pkt->payload[7]  = (uint8_t)(proto->avi.bottom & 0xFFu);
     pkt->payload[8]  = (uint8_t)(proto->avi.bottom >> 8u);
     pkt->payload[9]  = (uint8_t)(proto->avi.left & 0xFFu);
     pkt->payload[10] = (uint8_t)(proto->avi.left >> 8u);
     pkt->payload[11] = (uint8_t)(proto->avi.right & 0xFFu);
     pkt->payload[12] = (uint8_t)(proto->avi.right >> 8u);
 
     /* Checksum byte at index 3 of full packet header */
     uint8_t hdr[3] = { pkt->type, pkt->version, pkt->length };
     uint8_t sum = 0;
     for (int i = 0; i < 3; i++) sum += hdr[i];
     for (int i = 0; i < pkt->length; i++) sum += pkt->payload[i];
     pkt->checksum = (uint8_t)(0x100u - sum);
 
     /* Pack into 31-byte HDMI packet: [HB0][HB1][HB2][PB0..PB27] */
     uint8_t raw[31];
     raw[0] = pkt->type;
     raw[1] = pkt->version;
     raw[2] = pkt->length;
     raw[3] = pkt->checksum;
     memcpy(&raw[4], pkt->payload, 27u);
 
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)proto->hif->hw->priv;
     int rc = -ENOSYS;
     if (ops && ops->infoframe_send)
         rc = ops->infoframe_send(proto->hif->hw, raw, sizeof(raw));
 
     uiox_hdmi_buf_free_pkt(pkt);
     return rc;
 }
 
 int uiox_hdmi_proto_send_audio_if(uiox_hdmi_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     uiox_hdmi_pkt_t *pkt = uiox_hdmi_buf_alloc_pkt();
     if (!pkt) return -ENOMEM;
 
     pkt->type    = UIOX_HDMI_PKT_AUDIO;
     pkt->version = 1u;
     pkt->length  = 10u;
 
     const uiox_hdmi_audio_info_t *a = &proto->audio_info;
     pkt->payload[0] = (uint8_t)((a->coding_type << 4u) |
                                  (a->channel_count & 0x07u));
     pkt->payload[1] = (uint8_t)((a->sample_freq << 2u) | a->sample_size);
     pkt->payload[2] = 0u;  /* CT_EXT */
     pkt->payload[3] = a->channel_alloc;
     pkt->payload[4] = (uint8_t)((a->down_mix ? 0x80u : 0u) |
                                  (a->level_shift << 3u) |
                                   a->lfe_level);
 
     uint8_t raw[31] = {0};
     raw[0] = pkt->type; raw[1] = pkt->version; raw[2] = pkt->length;
     memcpy(&raw[4], pkt->payload, pkt->length);
     uint8_t sum = 0;
     for (int i = 0; i < 3 + (int)pkt->length; i++) sum += raw[i];
     raw[3] = (uint8_t)(0x100u - sum);
 
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)proto->hif->hw->priv;
     int rc = -ENOSYS;
     if (ops && ops->infoframe_send)
         rc = ops->infoframe_send(proto->hif->hw, raw, sizeof(raw));
 
     uiox_hdmi_buf_free_pkt(pkt);
     return rc;
 }
 
 int uiox_hdmi_proto_send_hdr(uiox_hdmi_proto_t  *proto,
                               const uiox_hdmi_hdr_t *hdr)
 {
     if (!proto || !hdr) return -EINVAL;
     memcpy(&proto->hdr, hdr, sizeof(*hdr));
     proto->hdr_enabled = true;
 
     uiox_hdmi_pkt_t *pkt = uiox_hdmi_buf_alloc_pkt();
     if (!pkt) return -ENOMEM;
 
     pkt->type    = UIOX_HDMI_PKT_HDRVS;
     pkt->version = 1u;
     pkt->length  = 26u;
 
     pkt->payload[0] = hdr->eotf;
     pkt->payload[1] = hdr->metadata_type;
     for (int i = 0; i < 3; i++) {
         pkt->payload[2 + i*4]     = (uint8_t)(hdr->display_primaries_x[i] & 0xFF);
         pkt->payload[2 + i*4 + 1] = (uint8_t)(hdr->display_primaries_x[i] >> 8);
         pkt->payload[2 + i*4 + 2] = (uint8_t)(hdr->display_primaries_y[i] & 0xFF);
         pkt->payload[2 + i*4 + 3] = (uint8_t)(hdr->display_primaries_y[i] >> 8);
     }
     pkt->payload[14] = (uint8_t)(hdr->white_point_x & 0xFF);
     pkt->payload[15] = (uint8_t)(hdr->white_point_x >> 8);
     pkt->payload[16] = (uint8_t)(hdr->white_point_y & 0xFF);
     pkt->payload[17] = (uint8_t)(hdr->white_point_y >> 8);
     pkt->payload[18] = (uint8_t)(hdr->max_luminance & 0xFF);
     pkt->payload[19] = (uint8_t)(hdr->max_luminance >> 8);
     pkt->payload[20] = (uint8_t)(hdr->min_luminance & 0xFF);
     pkt->payload[21] = (uint8_t)(hdr->min_luminance >> 8);
     pkt->payload[22] = (uint8_t)(hdr->max_cll & 0xFF);
     pkt->payload[23] = (uint8_t)(hdr->max_cll >> 8);
     pkt->payload[24] = (uint8_t)(hdr->max_fall & 0xFF);
     pkt->payload[25] = (uint8_t)(hdr->max_fall >> 8);
 
     uint8_t raw[31] = {0};
     raw[0] = pkt->type; raw[1] = pkt->version; raw[2] = pkt->length;
     memcpy(&raw[4], pkt->payload, pkt->length);
     uint8_t sum = 0;
     for (int i = 0; i < 3 + (int)pkt->length; i++) sum += raw[i];
     raw[3] = (uint8_t)(0x100u - sum);
 
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)proto->hif->hw->priv;
     int rc = -ENOSYS;
     if (ops && ops->infoframe_send)
         rc = ops->infoframe_send(proto->hif->hw, raw, sizeof(raw));
 
     uiox_hdmi_buf_free_pkt(pkt);
     return rc;
 }
 
 int uiox_hdmi_proto_send_spd(uiox_hdmi_proto_t *proto,
                                const char *vendor, const char *product)
 {
     if (!proto || !vendor || !product) return -EINVAL;
     uiox_hdmi_pkt_t *pkt = uiox_hdmi_buf_alloc_pkt();
     if (!pkt) return -ENOMEM;
 
     pkt->type    = UIOX_HDMI_PKT_SPD;
     pkt->version = 1u;
     pkt->length  = 25u;
 
     memset(pkt->payload, 0x20u, 25u);  /* space-fill */
     uint8_t vlen = (uint8_t)(strlen(vendor)  < 8u ? strlen(vendor)  : 8u);
     uint8_t plen = (uint8_t)(strlen(product) < 16u? strlen(product) : 16u);
     memcpy(pkt->payload,      vendor,  vlen);
     memcpy(pkt->payload + 8, product, plen);
     pkt->payload[24] = 0x00u;  /* Source Device Information: unknown      */
    
     uint8_t raw[31] = {0};
     raw[0] = pkt->type; raw[1] = pkt->version; raw[2] = pkt->length;
     memcpy(&raw[4], pkt->payload, pkt->length);
     uint8_t sum = 0;
     for (int i = 0; i < 3 + (int)pkt->length; i++) sum += raw[i];
     raw[3] = (uint8_t)(0x100u - sum);
    
     const uiox_hdmi_hw_ops_t *ops =
         (const uiox_hdmi_hw_ops_t *)proto->hif->hw->priv;
     int rc = -ENOSYS;
     if (ops && ops->infoframe_send)
         rc = ops->infoframe_send(proto->hif->hw, raw, sizeof(raw));
     uiox_hdmi_buf_free_pkt(pkt);
     return rc;
}
    
    int uiox_hdmi_proto_send_gcp(uiox_hdmi_proto_t *proto, bool mute)
    {
        if (!proto) return -EINVAL;
        proto->avmute = mute;
        uint8_t raw[7] = {
            UIOX_HDMI_PKT_GCP, 0x00u, 0x00u,
            mute ? 0x01u : 0x00u,  /* Set_AVMUTE or Clear_AVMUTE            */
            0x00u, 0x00u, 0x00u
        };
        const uiox_hdmi_hw_ops_t *ops =
            (const uiox_hdmi_hw_ops_t *)proto->hif->hw->priv;
        if (!ops || !ops->infoframe_send) return -ENOSYS;
        return ops->infoframe_send(proto->hif->hw, raw, sizeof(raw));
    }
    
    int uiox_hdmi_proto_cec_send(uiox_hdmi_proto_t *proto,
                                   uint8_t dst_la, uint8_t opcode,
                                   const uint8_t *params, uint8_t plen)
    {
        if (!proto) return -EINVAL;
        uint8_t msg[16];
        uint8_t len = 0;
        msg[len++] = (uint8_t)((proto->cec_la << 4u) | (dst_la & 0x0Fu));
        msg[len++] = opcode;
        if (params && plen > 0) {
            if (plen > 14u) plen = 14u;
            memcpy(&msg[len], params, plen);
            len += plen;
        }
        const uiox_hdmi_hw_ops_t *ops =
            (const uiox_hdmi_hw_ops_t *)proto->hif->hw->priv;
        if (!ops || !ops->cec_send) return -ENOSYS;
        return ops->cec_send(proto->hif->hw, dst_la, msg, len);
    }
    
    int uiox_hdmi_proto_cec_recv(uiox_hdmi_proto_t *proto,
                                   uint8_t *src_la, uint8_t *opcode,
                                   uint8_t *params, uint8_t *plen)
    {
        if (!proto || !src_la || !opcode || !params || !plen) return -EINVAL;
        const uiox_hdmi_hw_ops_t *ops =
            (const uiox_hdmi_hw_ops_t *)proto->hif->hw->priv;
        if (!ops || !ops->cec_recv) return -ENOSYS;
        uint8_t msg[16];
        uint8_t msg_len = 0;
        int rc = ops->cec_recv(proto->hif->hw, src_la, msg, &msg_len);
        if (rc < 0 || msg_len < 2u) return rc < 0 ? rc : -EAGAIN;
        *opcode = msg[1];
        *plen   = msg_len - 2u;
        if (*plen > 0) memcpy(params, &msg[2], *plen);
        return 0;
    }
    
void uiox_hdmi_proto_tick(uiox_hdmi_proto_t *proto, uint32_t now_ms)
{
    if (!proto) return;
    if ((now_ms - proto->last_infoframe_ms) < proto->infoframe_interval_ms)
        return;
    proto->last_infoframe_ms = now_ms;
    uiox_hdmi_proto_send_avi(proto);
    uiox_hdmi_proto_send_audio_if(proto);
    if (proto->hdr_enabled)
        uiox_hdmi_proto_send_hdr(proto, &proto->hdr);
}
