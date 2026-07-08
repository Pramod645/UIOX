This extends the existing uiox_fw_secboot.c/.h work (already started in 02_FwHal) into a full kernel signing, verification, and measurement chain that covers:

Build-time signing — uiox_ksign tool signs the kernel ELF
Boot-time verification — verifies signature before uiox_kernel_main()
Runtime integrity — periodically re-checks critical kernel text sections
Measurement log — TPM-style PCR chain for attestation
Key management — key revocation list (KRL) stored in NVRAM
Syscall interface — sys_kernel_verify() / sys_ksign_status()
===============================
50_UIX/12_ksign/
├── include/
│   ├── uiox_ksign_types.h     # Types: key, signature, cert chain, PCR
│   ├── uiox_ksign_crypto.h    # SHA-256/SHA-384, RSA-2048/ECDSA-256
│   ├── uiox_ksign_key.h       # Key store, KRL, key lifecycle
│   ├── uiox_ksign_image.h     # Signed image format, ELF annotation
│   ├── uiox_ksign_verify.h    # Signature verification engine
│   ├── uiox_ksign_measure.h   # PCR measurement log
│   ├── uiox_ksign_runtime.h   # Runtime integrity monitoring
│   └── uiox_ksign.h           # Master umbrella include
└── src/
    ├── uiox_ksign_crypto.c
    ├── uiox_ksign_key.c
    ├── uiox_ksign_image.c
    ├── uiox_ksign_verify.c
    ├── uiox_ksign_measure.c
    ├── uiox_ksign_runtime.c
    └── uiox_ksign_demo.c
