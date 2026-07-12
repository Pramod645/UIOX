## 8_Syscalls

| Syscall Name | Number (decimal) | Number (hex) | Arg 0 (a0/rdi/a0) | Arg 1 (a1/rsi/a1) | Arg 2 (a2/rdx/a2) | Arg 3 (a3/rcx/a3) | Return Value | Source File | Description |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| sys_kernel_verify | 220 | 0xDC | image_addr (uintptr_t) | image_size (size_t) | flags (long) | — (unused) | 0=OK, <0=uiox_ks_err_t | ksign_rt | Re-runs full ksign boot-time verify on a mapped kernel image from user-space. Parses header, checks rollback floor, SHA-256/384 hash, signature chain against locked keystore. |
| sys_ksign_status | 221 | 0xDD | buf (char* user) | buf_size (size_t) | — (unused) | — (unused) | 0=OK, <0=err | ksign_rt | Copies compact status string into user buffer: region count, total violations. Format: 'UIOX_KSIGN_STATUS regions=N violations=M'. |
| sys_ksign_quote | 222 | 0xDE | buf (uint8_t* user) | buf_size (size_t) | — (unused) | — (unused) | >0=bytes written, <0=err | ksign_rt | Serialises the live PCR measurement log into user buffer via uiox_ks_log_serialise(). Output is a flat binary blob (magic+version+count+PCR+entries) for remote attestation. Caller should sign the blob with a platform key for a full TPM-style quote. |
| — (constant) | — | — | UIOX_KS_IMG_MAGIC = 0x554B5349 | 'UKSI' | Signed image magic | — | — | ksign_types | uiox_ks_img_hdr_t.magic |
| — (constant) | — | — | UIOX_KS_KEY_MAGIC = 0x554B4B45 | 'UKKE' | Key entry magic | — | — | ksign_types | uiox_ks_key_entry_t.magic |
| — (constant) | — | — | UIOX_KS_KRL_MAGIC = 0x554B4B52 | 'UKKR' | KRL magic | — | — | ksign_types | uiox_ks_krl_t.magic |
| — (constant) | — | — | UIOX_KS_LOG_MAGIC = 0x554B4C47 | 'UKLG' | Meas. log magic | — | — | ksign_types | uiox_ks_log_t.magic |
| — (constant) | — | — | UIOX_KS_FORMAT_VERSION = 1 | — | Format version | — | — | ksign_types | All ksign on-disk structures |
| — (alg IDs) | — | — | UIOX_KS_ALG_NONE=0 | RSA2048_SHA256=1 | RSA4096_SHA256=2 | ECDSA_P256=3 | ED25519=4 | ksign_types | uiox_ks_alg_t enum |
| — (sizes) | — | — | SHA256_LEN=32 B | SHA384_LEN=48 B | SHA512_LEN=64 B | KEY_ID_LEN=32 B | IMG_HDR_SIZE=512 B | ksign_types | Fixed-size constants in uiox_ksign_types.h |
| — (img hdr) | — | — | kernel load_addr=0x40080000 | entry_addr=0x40080040 | payload_offset=512 | payload_size=var | build_time=unix ts | ksign_img | uiox_ks_img_hdr_t field layout |
