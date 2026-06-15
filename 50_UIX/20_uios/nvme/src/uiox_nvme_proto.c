/**
 * @file  uiox_nvme_proto.c
 * @brief UIOX NVMe protocol — controller init, queue mgmt, I/O, SMART.
 * @date  2026-06-12
 */

 #include "uiox_nvme_proto.h"
 #include <string.h>
 #include <errno.h>
 #include <stdio.h>
 
 /* Strip trailing spaces from ATA-style padded strings */
 static void str_trim(char *s, size_t maxlen)
 {
     for (int i = (int)maxlen - 1; i >= 0 && (s[i] == ' ' || s[i] == '\0');
          i--)
         s[i] = '\0';
 }
 
 int uiox_nvme_proto_init(uiox_nvme_proto_t *proto, uiox_nvme_if_t *nif)
 {
     if (!proto || !nif) return -EINVAL;
     memset(proto, 0, sizeof(*proto));
     proto->nif        = nif;
     proto->init_state = UIOX_NVME_INIT_IDLE;
     return 0;
 }
 
 int uiox_nvme_proto_identify_ctrl(uiox_nvme_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     static uint8_t buf[4096];
     memset(buf, 0, sizeof(buf));
 
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_IDENTIFY;
     cmd->sqe.nsid  = 0u;
     cmd->sqe.cdw10 = NVME_IDENTIFY_CNS_CTRL;
 
     int rc = uiox_nvme_if_admin(proto->nif, cmd, buf, 4096u);
     if (rc < 0) { uiox_nvme_cmd_free(cmd); return rc; }
     uiox_nvme_cmd_free(cmd);
 
     uiox_nvme_ctrl_id_t *id = &proto->nif->hw->ctrl_id;
 
     /* Vendor IDs at bytes 0–3 */
     id->vid   = (uint16_t)(buf[1] << 8u | buf[0]);
     id->ssvid = (uint16_t)(buf[3] << 8u | buf[2]);
 
     /* SN: bytes 4–23 (20 bytes) */
     memcpy(id->serial, &buf[4], 20u);
     id->serial[20] = '\0';
     str_trim(id->serial, 20u);
 
     /* MN: bytes 24–63 (40 bytes) */
     memcpy(id->model, &buf[24], 40u);
     id->model[40] = '\0';
     str_trim(id->model, 40u);
 
     /* FR: bytes 64–71 (8 bytes) */
     memcpy(id->fw_rev, &buf[64], 8u);
     id->fw_rev[8] = '\0';
     str_trim(id->fw_rev, 8u);
 
     /* MDTS: byte 77 */
     id->mdts = buf[77];
 
     /* Controller ID: bytes 78–79 */
     id->cntlid = buf[78];
 
     /* Version: bytes 80–83 */
     id->ver_major = buf[83];
     id->ver_minor = buf[82];
 
     /* NN: bytes 516–519 */
     id->nn = (uint32_t)buf[519] << 24u | (uint32_t)buf[518] << 16u
            | (uint32_t)buf[517] <<  8u | (uint32_t)buf[516];
 
     /* ONCS: bytes 520–521 — check TRIM/DSM support (bit 2) */
     uint16_t oncs = (uint16_t)(buf[521] << 8u | buf[520]);
     id->trim_supported = !!(oncs & (1u << 2u));
     id->volatile_wc    = !!(oncs & (1u << 5u));
 
     /* VWC: byte 525 */
     id->volatile_wc = !!(buf[525] & 0x01u);
 
     /* APSTA: byte 603 */
     id->apst_supported = !!(buf[603] & 0x01u);
 
     /* WCTEMP / CCTEMP: bytes 610–613 */
     id->warn_composite_temp = buf[610];
     id->crit_composite_temp = buf[612];
 
     printf("  [proto] Identify Ctrl: model='%.40s'  SN='%.20s'"
            "  FW='%.8s'\n",
            id->model, id->serial, id->fw_rev);
     printf("  [proto]   VID=0x%04X  NN=%u  TRIM=%d  APST=%d"
            "  VWC=%d\n",
            id->vid, id->nn,
            (int)id->trim_supported,
            (int)id->apst_supported,
            (int)id->volatile_wc);
     return 0;
 }
 
 int uiox_nvme_proto_identify_ns(uiox_nvme_proto_t *proto, uint32_t nsid)
 {
     if (!proto || nsid == 0u ||
         nsid > UIOX_NVME_MAX_NAMESPACES) return -EINVAL;
     static uint8_t buf[4096];
     memset(buf, 0, sizeof(buf));
 
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_IDENTIFY;
     cmd->sqe.nsid  = nsid;
     cmd->sqe.cdw10 = NVME_IDENTIFY_CNS_NS;
 
     int rc = uiox_nvme_if_admin(proto->nif, cmd, buf, 4096u);
     if (rc < 0) { uiox_nvme_cmd_free(cmd); return rc; }
     uiox_nvme_cmd_free(cmd);
 
     uiox_nvme_ns_t *ns = &proto->nif->hw->ns[nsid - 1u];
     ns->nsid = nsid;
 
     /* NSZE: bytes 0–7 */
     memcpy(&ns->nsze, &buf[0], 8u);
     /* NCAP: bytes 8–15 */
     memcpy(&ns->ncap, &buf[8], 8u);
     /* NUSE: bytes 16–23 */
     memcpy(&ns->nuse, &buf[16], 8u);
     /* LBA format: FLBAS byte 26 selects index; LBA data size from LBAF */
     uint8_t flbas = buf[26] & 0x0Fu;
     uint32_t lbads = buf[128u + flbas * 4u + 3u];  /* LBADS in LBAF */
     ns->lba_size = lbads ? (1u << lbads) : 512u;
     ns->active   = (ns->nsze > 0u);
 
     printf("  [proto] Identify NS %u: nsze=%llu  lba_size=%u\n",
            nsid,
            (unsigned long long)ns->nsze,
            ns->lba_size);
     return 0;
 }
 
 int uiox_nvme_proto_ns_list(uiox_nvme_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     static uint8_t buf[4096];
     memset(buf, 0, sizeof(buf));
 
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_IDENTIFY;
     cmd->sqe.nsid  = 0u;
     cmd->sqe.cdw10 = NVME_IDENTIFY_CNS_NS_LIST;
 
     int rc = uiox_nvme_if_admin(proto->nif, cmd, buf, 4096u);
     uiox_nvme_cmd_free(cmd);
     if (rc < 0) return rc;
 
     /* NS list: up to 1024 NSID entries (uint32_t each) */
     for (uint32_t i = 0u; i < 1024u; i++) {
         uint32_t id;
         memcpy(&id, &buf[i * 4u], 4u);
         if (id == 0u) break;
         if (id <= UIOX_NVME_MAX_NAMESPACES)
             uiox_nvme_proto_identify_ns(proto, id);
     }
     return 0;
 }
 
 int uiox_nvme_proto_create_iocq(uiox_nvme_proto_t *proto,
                                   uint16_t qid, uint16_t size,
                                   uint16_t vec)
 {
     if (!proto) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_CREATE_CQ;
     cmd->sqe.cdw10 = ((uint32_t)(size - 1u) << 16u) | qid;
     cmd->sqe.cdw11 = ((uint32_t)vec << 16u) | 0x3u;  /* IEN|PC */
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     uiox_nvme_cmd_free(cmd);
     printf("  [proto] Create IOCQ qid=%u size=%u vec=%u  rc=%d\n",
            qid, size, vec, rc);
     return rc;
 }
 
 int uiox_nvme_proto_create_iosq(uiox_nvme_proto_t *proto,
                                   uint16_t qid, uint16_t size,
                                   uint16_t cqid)
 {
     if (!proto) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_CREATE_SQ;
     cmd->sqe.cdw10 = ((uint32_t)(size - 1u) << 16u) | qid;
     cmd->sqe.cdw11 = ((uint32_t)cqid << 16u) | 0x1u;  /* PC */
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     uiox_nvme_cmd_free(cmd);
     printf("  [proto] Create IOSQ qid=%u size=%u cqid=%u  rc=%d\n",
            qid, size, cqid, rc);
     return rc;
 }
 
 int uiox_nvme_proto_delete_iosq(uiox_nvme_proto_t *proto, uint16_t qid)
 {
     if (!proto) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_DELETE_SQ;
     cmd->sqe.cdw10 = qid;
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     uiox_nvme_cmd_free(cmd);
     return rc;
 }
 
 int uiox_nvme_proto_delete_iocq(uiox_nvme_proto_t *proto, uint16_t qid)
 {
     if (!proto) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_DELETE_CQ;
     cmd->sqe.cdw10 = qid;
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     uiox_nvme_cmd_free(cmd);
     return rc;
 }
 
 int uiox_nvme_proto_set_queues(uiox_nvme_proto_t *proto,
                                 uint16_t nsq, uint16_t ncq)
 {
     if (!proto) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_SET_FEATURES;
     cmd->sqe.cdw10 = NVME_FEAT_NUM_QUEUES;
     cmd->sqe.cdw11 = ((uint32_t)(ncq - 1u) << 16u) | (nsq - 1u);
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     if (rc == 0) {
         /* Allocate the granted queues from CDW0 of completion */
         uint16_t nsqa = (uint16_t)((cmd->cqe.dw0 & 0xFFFFu) + 1u);
         uint16_t ncqa = (uint16_t)((cmd->cqe.dw0 >> 16u) + 1u);
         proto->nif->hw->num_io_queues = nsqa < ncqa ? nsqa : ncqa;
         printf("  [proto] Set features queues: %u SQ + %u CQ granted\n",
                nsqa, ncqa);
     }
     uiox_nvme_cmd_free(cmd);
     return rc;
 }
 
 int uiox_nvme_proto_set_apst(uiox_nvme_proto_t *proto, bool enable)
 {
     if (!proto) return -EINVAL;
     if (!proto->nif->hw->ctrl_id.apst_supported) return -ENOTSUP;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_SET_FEATURES;
     cmd->sqe.cdw10 = NVME_FEAT_AUTO_POWER_STATE;
     cmd->sqe.cdw11 = enable ? 1u : 0u;
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     printf("  [proto] APST %s  rc=%d\n", enable ? "enabled" : "disabled",
            rc);
     uiox_nvme_cmd_free(cmd);
     return rc;
 }
 
 int uiox_nvme_proto_set_volatile_wc(uiox_nvme_proto_t *proto, bool en)
 {
     if (!proto) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_SET_FEATURES;
     cmd->sqe.cdw10 = NVME_FEAT_VOLATILE_WC;
     cmd->sqe.cdw11 = en ? 1u : 0u;
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     if (rc == 0) proto->nif->hw->volatile_wc_enabled = en;
     printf("  [proto] Volatile WC %s  rc=%d\n",
            en ? "enabled" : "disabled", rc);
     uiox_nvme_cmd_free(cmd);
     return rc;
 }
 
 int uiox_nvme_proto_smart_log(uiox_nvme_proto_t *proto, uint8_t *buf)
 {
     if (!proto || !buf) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_GET_LOG_PAGE;
     cmd->sqe.nsid  = 0xFFFFFFFFu;  /* Global (all namespaces) */
     cmd->sqe.cdw10 = NVME_LOG_SMART |
                      ((512u / 4u - 1u) << 16u);  /* NUMDL = 127 DW */
     int rc = uiox_nvme_if_admin(proto->nif, cmd, buf, 512u);
     printf("  [proto] SMART log  rc=%d\n", rc);
     uiox_nvme_cmd_free(cmd);
     return rc;
 }
 
 int uiox_nvme_proto_ctrl_init(uiox_nvme_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     int rc;
 
     /* Reset and enable controller */
     proto->init_state = UIOX_NVME_INIT_RESET;
     rc = uiox_nvme_if_start(proto->nif);
     if (rc < 0) goto err;
     printf("  [proto] Controller reset + enable OK\n");
 
     /* Identify Controller */
     proto->init_state = UIOX_NVME_INIT_IDENTIFY_CTRL;
     rc = uiox_nvme_proto_identify_ctrl(proto);
     if (rc < 0) goto err;
 
     /* Set number of I/O queues */
     proto->init_state = UIOX_NVME_INIT_CREATE_IO_Q;
     rc = uiox_nvme_proto_set_queues(proto, 1u, 1u);
     if (rc < 0) goto err;
 
     /* Create I/O CQ (qid=1, vec=1) */
     rc = uiox_nvme_proto_create_iocq(proto, 1u,
                                       (uint16_t)UIOX_NVME_IO_Q_DEPTH,
                                       1u);
     if (rc < 0) goto err;
 
     /* Create I/O SQ (qid=1, paired with CQ 1) */
     rc = uiox_nvme_proto_create_iosq(proto, 1u,
                                       (uint16_t)UIOX_NVME_IO_Q_DEPTH,
                                       1u);
     if (rc < 0) goto err;
 
     /* Identify active namespaces */
     proto->init_state = UIOX_NVME_INIT_IDENTIFY_NS;
     rc = uiox_nvme_proto_ns_list(proto);
     if (rc < 0) goto err;
 
     /* Set features: volatile write cache, APST */
     proto->init_state = UIOX_NVME_INIT_SET_FEATURES;
     if (proto->nif->hw->ctrl_id.volatile_wc)
         uiox_nvme_proto_set_volatile_wc(proto, true);
     if (proto->nif->hw->ctrl_id.apst_supported)
         uiox_nvme_proto_set_apst(proto, true);
 
     proto->init_state  = UIOX_NVME_INIT_DONE;
     proto->initialized = true;
     printf("  [proto] NVMe controller init DONE\n");
     return 0;
 
 err:
     proto->init_state = UIOX_NVME_INIT_ERROR;
     printf("  [proto] NVMe init ERROR  state=%d  rc=%d\n",
            (int)proto->init_state, rc);
     return rc;
 }
 
 int uiox_nvme_proto_read(uiox_nvme_proto_t *proto,
                           uint32_t nsid, uint64_t slba,
                           uint8_t *buf, uint32_t nlb)
 {
     if (!proto || !proto->initialized || !buf || !nlb) return -EINVAL;
     return uiox_nvme_if_read(proto->nif, nsid, slba, buf, nlb);
 }
 
 int uiox_nvme_proto_write(uiox_nvme_proto_t *proto,
                            uint32_t nsid, uint64_t slba,
                            const uint8_t *buf, uint32_t nlb)
 {
     if (!proto || !proto->initialized || !buf || !nlb) return -EINVAL;
     return uiox_nvme_if_write(proto->nif, nsid, slba, buf, nlb);
 }
 
 int uiox_nvme_proto_flush(uiox_nvme_proto_t *proto, uint32_t nsid)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     return uiox_nvme_if_flush(proto->nif, nsid);
 }
 
 int uiox_nvme_proto_trim(uiox_nvme_proto_t *proto,
                           uint32_t nsid, uint64_t slba, uint32_t nlb)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     if (!proto->nif->hw->ctrl_id.trim_supported) return -ENOTSUP;
     return uiox_nvme_if_trim(proto->nif, nsid, slba, nlb);
 }
 
 int uiox_nvme_proto_format_ns(uiox_nvme_proto_t *proto,
                                uint32_t nsid, uint8_t lbaf)
 {
     if (!proto || !proto->initialized) return -EINVAL;
     uiox_nvme_cmd_t *cmd = uiox_nvme_cmd_alloc();
     if (!cmd) return -ENOMEM;
     cmd->sqe.opc   = NVME_ADMIN_FORMAT_NVM;
     cmd->sqe.nsid  = nsid;
     cmd->sqe.cdw10 = (uint32_t)lbaf & 0x0Fu;
     int rc = uiox_nvme_if_admin(proto->nif, cmd, NULL, 0u);
     printf("  [proto] Format NS %u  LBAF=%u  rc=%d\n",
            nsid, lbaf, rc);
     uiox_nvme_cmd_free(cmd);
     return rc;
 }
 
 int uiox_nvme_proto_shutdown(uiox_nvme_proto_t *proto)
 {
     if (!proto) return -EINVAL;
     /* Delete I/O queues before shutdown */
     uiox_nvme_proto_delete_iosq(proto, 1u);
     uiox_nvme_proto_delete_iocq(proto, 1u);
     /* Set CC.SHN = 01b (normal shutdown) */
     uint32_t cc = uiox_nvme_hw_reg_read32(proto->nif->hw, NVME_REG_CC);
     cc = (cc & ~(3u << 14u)) | (1u << 14u);
     uiox_nvme_hw_reg_write32(proto->nif->hw, NVME_REG_CC, cc);
     printf("  [proto] Shutdown initiated\n");
     proto->nif->hw->ready = false;
     return 0;
 }
 