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
====================
Stage 0d (02_FwHal)
    └─▶ uiox_ks_boot_entry()
            ├─ boot_init()      ← keystore seeded from OTP
            ├─ boot_verify()    ← full sig + cert chain + anti-rollback
            ├─ boot_measure()   ← PCR[1/2/5] extended
            ├─ boot_arm_runtime() ← .text/.rodata hashes registered
            └─ boot_handoff()   ← PCRs locked → jump to kernel entry

Scheduler tick (33_ProcessControlSubsystem)
    └─▶ uiox_ks_scheduler_tick() → uiox_ks_rt_tick() → re-hash regions

Syscall table (40_SystemCallInterface)
    ├─ SYS_KERNEL_VERIFY (220)
    ├─ SYS_KSIGN_STATUS  (221)
    └─ SYS_KSIGN_QUOTE   (222)
-------------------------

12_ksign — Kernel Image Signing & Verification
Must be kernel — no question.

Called directly from uiox_kernel_main() as uiox_ks_boot_entry() before any userspace exists
Seeds keystore from OTP (hardware register access — only possible in kernel/privileged mode)
Performs SHA-256/SHA-384 + RSA-2048/ECDSA-256 signature verification of the kernel itself
Extends PCR measurements (TPM-style — requires privileged hardware access)
Has a scheduler tick hook uiox_ks_scheduler_tick() → re-hashes kernel text/rodata regions at runtime — this is a kernel integrity monitor, completely incompatible with userspace
Where it belongs: already correctly in 30_KIX build, linked into the kernel ELF.