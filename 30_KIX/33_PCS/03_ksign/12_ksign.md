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


===========================
What I'd fix, in order
None of this is a rewrite; it's reconciliation in five places.

A. One verify_init signature. I'd take the header's four-parameter form and drop measure_ctx from the boot call — or, if the verify context does need the measure context (it extends PCR[6] on signature events per the diagram), add it as a fifth parameter in the header and the definition, in the same order. Decide which; don't leave three versions.

B. Assign ctx->image_base. In uiox_ks_verify_image, after the header parses: ctx->image_base = (uintptr_t)image;. That's the missing line that makes boot_arm_runtime work.

C. Name the hooks once. Either rename uiox_ks_boot_entry → uiox_ksign_boot_verify etc., or change the umbrella header to match the file. Given the file's names are more specific and its banner explains the two roles, I'd update the header.

D. Fix the PCR assignment. Add UIOX_KS_PCR_KEYLOAD to types.h, or move the signing-key extend to PCR[6] (SIG_VERIFY), which the diagram already assigns and which nothing currently uses.

E. Decide sim_mode's default explicitly. Right now it's implicitly false via static zeroing. Either set it from a compile flag in boot_init and say so, or leave it false and delete the sim branch from verify_image — a branch that can never be reached is worse than no branch.

What I need before writing anything
Two files I have only in preview: uiox_ksign_runtime.c (up-1m0x6ly, 308 lines) and uiox_ksign_key.c. The runtime file decides item B's downstream — whether rt_seed_from_image is consistent with the fix. And uiox_ksign.h I have from the paste, so item C is writable now.

Attach those two and I'll do A and C first, since they're pure interface reconciliation with no behaviour change, then B and D. I'm reading before writing this time — and where the code contradicts its own banner, I'll cite the line rather than reason about what it probably meant.




Then four real fixes, in dependency order:

1. uiox_ksign_verify_init — pick one signature. The header's four-parameter form is what the code should use; uiox_ksign.c's five-argument call with &g_measure_ctx in the min_ver slot has to change. If the verify context genuinely needs the measure context (per the diagram, signature events extend PCR[6]), add it to the header and both call sites together.

2. PCR assignment — one table, one owner. Right now types.h says PCR[5] is RUNTIME, uiox_ksign.c puts the signing key there, and runtime.c uses PCR[7] which nothing names. My recommendation: keep types.h's assignments, move the signing-key extend to a new UIOX_KS_PCR_KEYLOAD 6u, and give runtime checks their own UIOX_KS_PCR_RUNTIME 5u consistently — matching the measure.c banner's own list.

3. rt_seed_from_image must read hdr->payload_hash, not re-hash live memory. This is the one with a security consequence: today it adopts whatever is in .text at boot as the expected value, so a kernel tampered with before that point is blessed. The hdr parameter is unused — which -Werror will also flag as an unused parameter.

4. ctx->image_base never assigned. Add it in uiox_ks_verify_image after the header parses. Without it boot_arm_runtime reads NULL and the if (!hdr) guard fires every boot, so runtime monitoring never arms.


What I can write now, and what I'd need
I have read in full: types.h, crypto.h, key.h, image.h, verify.h, measure.h, runtime.h, uiox_ksign.h, verify.c, image.c, measure.c, key.c, runtime.c, and uiox_ksign.c. That's everything except crypto.c (I have its first 40 lines) and demo.c.

So items 1–4 are writable now. Item 1 needs one decision from you: does uiox_ks_verify_ctx_t carry a measure_ctx pointer or not?

Before I write, one question because it changes the shape of the fix:

Which side is authoritative — the headers or the sources? I've assumed headers, on the grounds that every other module #includes them. But if the sources are what you've been building against, the rename goes the other way, and there are four headers to touch instead of four sources.

And a second, smaller one: uiox_ksign_image.c appears three times identically in your paste. If that's three real files, two need deleting. If it's one file pasted thrice, ignore this.

Tell me the direction on naming and whether the verify ctx should hold the measure ctx, and I'll write the corrected set — starting with the two files where the mismatch is total (runtime.h/runtime.c) and the two lines that fix the security defect in rt_seed_from_image.