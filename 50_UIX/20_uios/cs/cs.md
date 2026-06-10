UIOX Cybersecurity Architecture:
1. Full Stack Architecture Diagram
┌─────────────────────────────────────────────────────────────────────┐
│                        APPLICATION LAYER                            │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ │
│  │  Auth &  │ │  Crypto  │ │  Audit   │ │  Policy  │ │  Threat  │ │
│  │  IAM     │ │  API     │ │  Logger  │ │  Engine  │ │  Intel   │ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│                      MIDDLEWARE / RUNTIME LAYER                     │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ │
│  │  TLS/    │ │  Secure  │ │  RBAC /  │ │  DLP     │ │  IDS /   │ │
│  │  DTLS    │ │  Channel │ │  ABAC    │ │  Engine  │ │  IPS     │ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│                          KERNEL LAYER                               │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ │
│  │  LSM /   │ │  Syscall │ │  Memory  │ │  Secure  │ │  Kernel  │ │
│  │  SELinux │ │  Filter  │ │  Protect │ │  Boot    │ │  Crypto  │ │
│  │  AppArmor│ │  (seccomp│ │  (SMEP/  │ │  Verify  │ │  (kcapi) │ │
│  │          │ │  EBPF)   │ │  SMAP)   │ │          │ │          │ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│                       FIRMWARE / TEE LAYER                          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ │
│  │  UEFI    │ │  Secure  │ │  Trusted │ │  PCIe    │ │  ME / PSP│ │
│  │  Secure  │ │  Enclave │ │  Exec    │ │  DMA     │ │  Fused   │ │
│  │  Boot    │ │  (SGX/   │ │  Env     │ │  Protect │ │  Keys    │ │
│  │          │ │  TrustZone│ │  (TEE)  │ │  (IOMMU) │ │          │ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│                       HARDWARE LAYER                                │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ │
│  │  HSM /   │ │  TPM 2.0 │ │  Crypto  │ │  PUF /   │ │  Secure  │ │
│  │  Secure  │ │  (PCR,   │ │  Engine  │ │  RNG     │ │  Element │ │
│  │  Enclave │ │  Attest) │ │  (AES-NI │ │  (TRNG)  │ │  (eSE)   │ │
│  │          │ │          │ │  SHA-NI) │ │          │ │          │ │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘ │
└─────────────────────────────────────────────────────────────────────┘

2. Layer-by-Layer Architecture
Layer 1 — Hardware Security
Hardware Security Subsystems
├── HSM (Hardware Security Module)
│   ├── Key generation, storage, wrapping
│   ├── RSA-4096, ECC-P384, AES-256-GCM
│   └── FIPS 140-3 Level 3 boundary
│
├── TPM 2.0 (Trusted Platform Module)
│   ├── PCR (Platform Configuration Registers) — measurement chain
│   ├── Sealed storage — keys locked to system state
│   ├── Remote attestation — proof of platform integrity
│   └── NV storage — monotonic counter (anti-replay)
│
├── CPU Crypto Extensions
│   ├── AES-NI — hardware-accelerated AES
│   ├── SHA-NI — hardware-accelerated SHA-256/512
│   ├── RDRAND / RDSEED — hardware TRNG
│   ├── Intel SGX — isolated secure enclaves in memory
│   └── ARM TrustZone — split Normal/Secure world
│
├── PUF (Physically Unclonable Function)
│   ├── Device-unique fingerprint from silicon variation
│   ├── Root key derivation without persistent storage
│   └── Anti-cloning for device identity
│
├── IOMMU / VT-d / SMMU
│   ├── DMA remapping — prevents DMA attacks from peripherals
│   ├── Device isolation — each device sees only its memory
│   └── TBT4 / PCIe DMA protection
│
└── Secure Element (eSE / eSIM)
    ├── Credential storage for IoT / mobile
    ├── JavaCard OS with GlobalPlatform API
    └── Side-channel resistant execution


Layer 2 — Firmware / TEE Security
Firmware Security Subsystems
├── UEFI Secure Boot
│   ├── db / dbx signature databases
│   ├── PK → KEK → db chain of trust
│   ├── Shim → GRUB → Kernel signature verification
│   └── Measured boot → TPM PCR[0..7]
│
├── Trusted Execution Environment (TEE)
│   ├── OP-TEE (ARM TrustZone runtime)
│   │   ├── Normal World: Rich OS (Linux/Android)
│   │   ├── Secure World: OP-TEE OS + TAs (Trusted Apps)
│   │   └── SMC gateway: controlled crossing
│   ├── Intel TDX (Trust Domain Extensions)
│   │   ├── VM-level isolation in cloud
│   │   └── Guest firmware measurement
│   └── AMD SEV-SNP (Secure Encrypted Virtualisation)
│       ├── VCPU state encrypted + integrity-protected
│       └── Attestation report from PSP
│
├── PCIe / TBT4 DMA Protection
│   ├── IOMMU enabled before device initialisation
│   ├── DMA target validation per device
│   ├── TBT4 SL1/SL2 device authorisation
│   └── PCIe SR-IOV isolation domains
│
└── Fused Security Config (OTP)
    ├── Secure boot mandatory fuse
    ├── JTAG disable fuse
    ├── Debug port lockdown
    └── Device key fused at manufacturing


Layer 3 — Kernel Security
Kernel Security Subsystems
├── Linux Security Modules (LSM)
│   ├── SELinux
│   │   ├── Type Enforcement (TE) policy
│   │   ├── MLS / MCS labels
│   │   └── Process/file/network/IPC mandatory access control
│   ├── AppArmor
│   │   ├── Path-based MAC profiles
│   │   └── Container confinement
│   └── Landlock
│       ├── Unprivileged sandboxing (no root required)
│       └── Fine-grained filesystem access rules
│
├── Syscall Filtering
│   ├── seccomp-BPF
│   │   ├── Per-process syscall whitelist
│   │   ├── SECCOMP_MODE_STRICT / FILTER
│   │   └── Docker / container default profiles
│   └── eBPF security programs
│       ├── LSM hook programs (bpf_lsm)
│       ├── Network policy enforcement (XDP / tc)
│       └── Runtime tracing without kernel modification
│
├── Memory Protection
│   ├── KASLR — kernel address space randomisation
│   ├── SMEP — supervisor mode execution prevention
│   ├── SMAP — supervisor mode access prevention
│   ├── CET — control-flow enforcement (shadow stack)
│   ├── KPTI — page table isolation (Meltdown mitigation)
│   ├── CFI — indirect call target validation
│   └── Stack canaries + FORTIFY_SOURCE
│
├── Kernel Crypto API (kcapi)
│   ├── Cipher drivers: AES-GCM, ChaCha20-Poly1305
│   ├── Hash: SHA-3, BLAKE2, HMAC
│   ├── Asymmetric: RSA, ECDSA, EdDSA
│   ├── KDF: HKDF, SP800-108
│   └── FIPS 140-3 self-test at boot
│
├── Secure Boot Chain Verification
│   ├── IMA (Integrity Measurement Architecture)
│   │   ├── File hash measurement into TPM PCRs
│   │   ├── ima-appraise — block modified files
│   │   └── EVM — extended verification module
│   └── dm-verity
│       ├── Block-level integrity for read-only partitions
│       └── Android Verified Boot / ChromeOS
│
└── Network Stack Security
    ├── netfilter: nftables rules
    ├── Network namespaces (container isolation)
    ├── IPsec (xfrm) — kernel-native VPN
    └── WireGuard — in-kernel modern VPN


Layer 4 — Middleware / Runtime Security
Middleware Security Subsystems
├── TLS / DTLS Stack
│   ├── Protocol: TLS 1.3 only (1.2 deprecated)
│   ├── Cipher suites: TLS_AES_256_GCM_SHA384,
│   │                   TLS_CHACHA20_POLY1305_SHA256
│   ├── Key exchange: X25519, P-384, ML-KEM-768 (post-quantum)
│   ├── Auth: ECDSA P-256, Ed25519, ML-DSA (post-quantum)
│   ├── Certificate pinning + OCSP stapling
│   └── mTLS — mutual authentication for service mesh
│
├── Secure Channel Manager
│   ├── Session key derivation (HKDF)
│   ├── Perfect Forward Secrecy (Ephemeral DH)
│   ├── Key rotation policy (time + volume limits)
│   └── Anti-replay: sequence numbers + timestamps
│
├── RBAC / ABAC Engine
│   ├── RBAC: role → permission mapping
│   ├── ABAC: attribute-based (user + resource + env)
│   ├── PBAC: policy-based (OPA / Rego rules)
│   └── JWT + OIDC token validation
│
├── DLP (Data Loss Prevention)
│   ├── Content inspection: regex + ML classifier
│   ├── Data classification: Public/Internal/Confidential/Secret
│   ├── Egress filtering: email, USB, network
│   └── Watermarking: invisible tracking of exfiltrated docs
│
└── IDS / IPS Engine
    ├── Signature-based: Snort / Suricata rules
    ├── Anomaly-based: ML baseline deviation detection
    ├── Network IDS: packet capture + deep inspection
    └── Host IDS: file integrity + process monitoring


Layer 5 — Application Security
Application Security Subsystems
├── Identity & Access Management (IAM)
│   ├── AuthN: password, TOTP, FIDO2/WebAuthn, passkeys
│   ├── AuthZ: RBAC + ABAC + PBAC
│   ├── SSO: SAML 2.0, OIDC, OAuth 2.1
│   ├── MFA: TOTP, hardware token, biometric
│   └── Privileged Access Management (PAM)
│
├── Cryptographic API
│   ├── Key management: generate, store, rotate, revoke
│   ├── Encryption: AES-256-GCM, ChaCha20-Poly1305
│   ├── Signing: ECDSA P-384, Ed25519, RSA-PSS-4096
│   ├── Post-Quantum: ML-KEM-768, ML-DSA-65
│   └── PKI: X.509 issuance, OCSP, CRL, CT logs
│
├── Audit & Compliance Logger
│   ├── Tamper-evident log: append-only + Merkle tree
│   ├── SIEM integration: syslog, CEF, LEEF
│   ├── Compliance: SOC2, ISO27001, PCI-DSS, GDPR
│   └── Log forwarding: encrypted + authenticated
│
├── Policy Engine
│   ├── OPA (Open Policy Agent) with Rego
│   ├── Zero Trust Network Access (ZTNA) policies
│   ├── Data residency enforcement
│   └── Regulatory compliance automation
│
└── Threat Intelligence
    ├── STIX / TAXII feed ingestion
    ├── IOC matching: IP, domain, hash, URL
    ├── ATT&CK mapping (MITRE framework)
    └── Automated threat hunting


3. Cross-Cutting Security Mechanisms
Cross-Cutting Mechanisms (span all layers)
├── Chain of Trust
│   HW keys → TPM → UEFI Secure Boot → Bootloader
│   → Kernel IMA → Application signature verification
│
├── Cryptographic Agility
│   Classical now    → Post-Quantum ready (hybrid mode)
│   ECDH + ML-KEM   → both used simultaneously
│   ECDSA + ML-DSA  → dual-signed certificates
│
├── Zero Trust Architecture
│   "Never trust, always verify" at every layer:
│   HW attestation → network micro-segmentation
│   → per-request authentication → least privilege
│
└── Defense in Depth
    Each layer independently enforces:
    Confidentiality + Integrity + Availability + Non-repudiation


4. Cybersecurity Use Cases
Total: 12 Primary Use Cases

UC-01  Secure Boot and Platform Integrity
UC-02  Device Identity and Attestation
UC-03  Encrypted Storage
UC-04  Secure Communication
UC-05  Identity and Access Management
UC-06  Intrusion Detection and Prevention
UC-07  Data Loss Prevention
UC-08  Vulnerability Management
UC-09  Incident Response and Forensics
UC-10  Secure Software Supply Chain
UC-11  Post-Quantum Cryptography Readiness
UC-12  Privileged Access and Secrets Management


Use Case UC-01 — Secure Boot and Platform Integrity
UC-01: Secure Boot and Platform Integrity
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Guarantee that only trusted, unmodified software runs
      from power-on to running application.

Attack prevented:
  • Bootkit / rootkit injection
  • Firmware tampering (Evil Maid)
  • Rollback to known-vulnerable firmware

Module map:

  [HW]       TPM 2.0 PCR measurement
      ↓           Records each boot component hash into PCR[0..7]
  [Firmware]  UEFI Secure Boot
      ↓           Verifies EFI image signatures against db/dbx
  [Firmware]  Measured Boot
      ↓           Extends TPM PCRs with bootloader + kernel hashes
  [Kernel]    IMA (Integrity Measurement Architecture)
      ↓           Measures every executed file into TPM PCR[10]
  [Kernel]    dm-verity
      ↓           Block-level hash tree over read-only partitions
  [Kernel]    IMA-appraise
      ↓           Blocks execution of files failing hash check
  [App]       Remote Attestation
                  Relying party verifies TPM quote against
                  expected PCR values (Golden Values database)

Key data flows:
  Power-on
    → UEFI measures itself → PCR[0]
    → UEFI measures Option ROMs → PCR[2]
    → UEFI measures Boot Manager → PCR[4]
    → Shim measures GRUB → PCR[9]
    → GRUB measures kernel + initrd → PCR[8]
    → Kernel runs IMA on every open() → PCR[10]
    → TPM Quote generated
    → Remote Verifier checks Quote signature + PCR values
    → PASS / FAIL decision

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ HW Root of Trust    │ TPM 2.0 / fused keys        │
  │ FW Verification     │ UEFI Secure Boot            │
  │ Bootloader          │ Shim + GRUB2 signed         │
  │ Kernel integrity    │ IMA + EVM + dm-verity        │
  │ Attestation         │ TPM2-tools + Keylime        │
  │ Golden Values DB    │ Reference PCR store         │
  │ Policy Enforcement  │ Block boot on PCR mismatch  │
  └─────────────────────────────────────────────────┘


Use Case UC-02 — Device Identity and Attestation
UC-02: Device Identity and Attestation
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Every device has a unique, unforgeable cryptographic
      identity provable to a remote verifier.

Attack prevented:
  • Device impersonation / spoofing
  • Counterfeit hardware in supply chain
  • Man-in-the-middle on device onboarding

Module map:

  [HW]       PUF (Physically Unclonable Function)
      ↓           Derives unique root key from silicon
  [HW]       Secure Element / HSM
      ↓           Stores Device ID Certificate (DevID)
  [Firmware]  DevID key generation at manufacturing
      ↓           IDevID (Initial) → LDevID (Local operational)
  [Firmware]  TPM Endorsement Key (EK)
      ↓           Manufacturer-signed, burned at factory
  [Firmware]  Attestation Key (AK) derivation
      ↓           Derived from EK, signs TPM quotes
  [App]       FIDO Device Onboarding (FDO) protocol
      ↓           Zero-touch provisioning with ownership transfer
  [App]       SCEP / EST certificate enrollment
      ↓           Issues operational certificate from PKI
  [App]       NIST SP 800-193 Platform Firmware Resiliency
                  Detects + recovers from firmware corruption

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ Root Identity       │ PUF / fused HW key          │
  │ Key Storage         │ TPM EK / HSM / eSE          │
  │ Identity Cert       │ IEEE 802.1AR DevID (X.509)  │
  │ Onboarding          │ FIDO FDO / SCEP / EST       │
  │ Quote / Attest      │ TPM2_Quote + PCR log        │
  │ Verifier            │ Keylime / Parsec / RATS     │
  │ PKI                 │ EJBCA / CFSSL / HashiCorp   │
  └─────────────────────────────────────────────────┘


Use Case UC-03 — Encrypted Storage
UC-03: Encrypted Storage
━━━━━━━━━━━━━━━━━━━━━━━

Goal: All data at rest is encrypted; keys are bound
      to platform identity, not stored in plaintext.

Attack prevented:
  • Physical disk theft
  • Cold-boot attack
  • Cloud storage breach
  • Insider threat via direct disk access

Module map:

  [HW]       AES-NI hardware acceleration
      ↓           < 1% CPU overhead for full-disk encryption
  [HW]       TPM 2.0 sealed key storage
      ↓           DEK sealed to PCR[0,7] — only unseals if boot clean
  [Firmware]  TCG OPAL 2.0 (Self-Encrypting Drive)
      ↓           Drive firmware handles AES-256-XTS
  [Kernel]    dm-crypt / LUKS2
      ↓           Block-layer AES-256-XTS or ChaCha20-poly1305
  [Kernel]    eCryptfs / fscrypt
      ↓           File-level encryption (per-file DEK)
  [Kernel]    keyring subsystem
      ↓           In-kernel key cache; cleared on session end
  [Middleware] Key Management Service (KMS)
      ↓           Centralised key lifecycle management
  [App]       Application-level encryption
                  SQLCipher (database), age (files),
                  Encrypted protobuf fields

Key hierarchy:
  Master Key (HSM/TPM sealed)
    └── Volume Encryption Key (VEK)   [LUKS2 header]
          └── File Encryption Key (FEK)  [fscrypt per file]
                └── Data Encryption Key (DEK) [per-block AES-XTS]

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ HW acceleration     │ AES-NI / SHA-NI             │
  │ Key sealing         │ TPM 2.0 PCR policy          │
  │ Full-disk encrypt   │ LUKS2 + dm-crypt            │
  │ File-level encrypt  │ fscrypt (ext4/f2fs/ubifs)   │
  │ SED                 │ TCG OPAL 2.0 NVMe/SATA      │
  │ KMS                 │ HashiCorp Vault / AWS KMS   │
  │ Key rotation        │ Automated 90-day rotation   │
  └─────────────────────────────────────────────────┘


Use Case UC-04 — Secure Communication
UC-04: Secure Communication
━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: All data in transit is encrypted, authenticated,
      and integrity-protected. Zero-trust between services.

Attack prevented:
  • Man-in-the-Middle (MitM)
  • Protocol downgrade (SSL stripping)
  • Certificate spoofing
  • Replay attacks

Module map:

  [HW]       TRNG for nonce generation
      ↓           Prevents predictable IVs / nonces
  [Kernel]    WireGuard / IPsec (xfrm)
      ↓           Layer-3 VPN: site-to-site / remote access
  [Kernel]    TCP stack hardening
      ↓           SYN cookies, TCP MD5, RFC 5961 mitigation
  [Middleware] TLS 1.3 termination
      ↓           Cipher: AES-256-GCM / ChaCha20-Poly1305
      ↓           Key exchange: X25519 + ML-KEM-768 (hybrid PQ)
      ↓           Auth: ECDSA P-384 + ML-DSA-65 (hybrid PQ)
  [Middleware] mTLS (Mutual TLS)
      ↓           Both client + server present certificates
      ↓           Service mesh (Istio / Linkerd)
  [Middleware] Certificate Transparency (CT)
      ↓           All issued certs logged to public CT log
  [App]       HSTS / HPKP headers
      ↓           Force TLS; pin expected cert public key
  [App]       Certificate Pinning
      ↓           Mobile / embedded: reject unexpected certs
  [App]       QUIC + TLS 1.3
                  Zero-RTT + 0-RTT anti-replay protection

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ VPN                 │ WireGuard / IPsec / OpenVPN │
  │ TLS library         │ OpenSSL 3.x / mbedTLS / s2n │
  │ PQ hybrid KEM       │ X25519 + ML-KEM-768         │
  │ PQ hybrid Sign      │ ECDSA + ML-DSA-65           │
  │ Service mesh        │ Istio mTLS / Linkerd        │
  │ PKI                 │ EJBCA / Let's Encrypt / own │
  │ Certificate pinning │ HPKP / custom validator     │
  └─────────────────────────────────────────────────┘


Use Case UC-05 — Identity and Access Management
UC-05: Identity and Access Management (IAM)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Only authenticated, authorised principals access
      resources. Least-privilege enforced at every layer.

Attack prevented:
  • Credential stuffing / password spray
  • Privilege escalation
  • Lateral movement
  • Insider threat

Module map:

  [HW]       FIDO2 security key (YubiKey / TPM)
      ↓           Phishing-resistant MFA via WebAuthn
  [Firmware]  Measured boot → platform auth claim
      ↓           Device health included in auth decision
  [Kernel]    PAM (Pluggable Authentication Modules)
      ↓           pam_u2f, pam_tpm2, pam_fprintd
  [Middleware] OIDC / OAuth 2.1 token issuance
      ↓           Short-lived JWT (15 min) + refresh token
  [Middleware] SAML 2.0 federation
      ↓           Enterprise SSO across domains
  [Middleware] RBAC engine
      ↓           Role → Permission mapping (JSON/YAML)
  [Middleware] ABAC engine (OPA + Rego)
      ↓           Attribute-based: user + resource + context
  [App]       Passkeys (FIDO2 + sync)
      ↓           Platform authenticator (Face ID, Windows Hello)
  [App]       Just-In-Time (JIT) access provisioning
      ↓           Temporary elevated access with audit trail
  [App]       Continuous Access Evaluation (CAE)
                  Revoke tokens mid-session on risk signal

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ AuthN               │ FIDO2 / passkeys / TOTP     │
  │ SSO                 │ Keycloak / Okta / Entra ID  │
  │ Token format        │ JWT + PKCE / PASETO         │
  │ RBAC                │ Casbin / OPA / AWS IAM      │
  │ MFA                 │ TOTP / WebAuthn / SMS(avoid)│
  │ PAM                 │ pam_u2f / pam_tpm2          │
  │ Provisioning        │ SCIM 2.0                    │
  │ Directory           │ OpenLDAP / Active Directory │
  └─────────────────────────────────────────────────┘


Use Case UC-06 — Intrusion Detection and Prevention
UC-06: Intrusion Detection and Prevention (IDS/IPS)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Detect and block malicious activity across
      network, host, and application layers in real time.

Attack prevented:
  • Network intrusion / lateral movement
  • Zero-day exploit attempts
  • Malware execution
  • Exfiltration channels

Module map:

  [HW]       Network TAP / SmartNIC offload
      ↓           Line-rate packet capture without CPU overhead
  [Kernel]    eBPF-based network monitoring
      ↓           XDP: drop at driver level (< 1 μs latency)
      ↓           tc eBPF: inspect + classify all packets
  [Kernel]    HIDS: auditd + fanotify
      ↓           System call audit log
      ↓           File open/exec events (fanotify)
  [Kernel]    LSM hooks for behavioral detection
      ↓           bpf_lsm: block at kernel security decision points
  [Middleware] Network IDS (NIDS)
      ↓           Suricata: signature + protocol anomaly detection
      ↓           Zeek: protocol analysis + scripted detection
  [Middleware] Host IDS (HIDS)
      ↓           OSSEC / Wazuh: log analysis + FIM + rootkit scan
      ↓           Falco: container runtime security (eBPF-based)
  [Middleware] ML-based anomaly detection
      ↓           Baseline: CPU, memory, network, syscall rates
      ↓           Alert on statistical deviation > 3σ
  [App]       SIEM correlation engine
      ↓           Elastic SIEM / Splunk / Chronicle
      ↓           YARA rules for malware pattern matching
  [App]       SOAR (Security Orchestration Automation Response)
                  Automated playbook execution on alert

Detection methods:
  Signature-based   → Known CVE exploits, malware hashes
  Anomaly-based     → Deviation from normal baseline
  Behavioral        → Process tree, syscall sequence analysis
  Protocol          → RFC violation detection
  Threat Intel      → IOC matching (IP/domain/hash/URL)

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ NIDS                │ Suricata 7 / Zeek 7         │
  │ HIDS                │ Wazuh / OSSEC / Samhain     │
  │ Container IDS       │ Falco (eBPF)                │
  │ eBPF monitoring     │ Cilium / Tetragon           │
  │ SIEM                │ Elastic / Splunk / Wazuh    │
  │ SOAR                │ TheHive / Shuffle / Cortex  │
  │ Threat Intel        │ MISP / OpenCTI              │
  │ ML anomaly          │ sklearn / Isolation Forest  │
  └─────────────────────────────────────────────────┘


Use Case UC-07 — Data Loss Prevention
UC-07: Data Loss Prevention (DLP)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Prevent sensitive data from leaving the organisation
      via any channel (network, USB, email, cloud).

Attack prevented:
  • Accidental data leakage (employee error)
  • Intentional insider exfiltration
  • Phishing-driven data theft
  • Supply chain data exposure

Module map:

  [HW]       USB device control (udev rules / endpoint agent)
      ↓           Block unauthorised USB mass storage
  [Kernel]    fanotify — intercept file open/read/copy events
      ↓           Inspect content before allowing write to USB/net
  [Middleware] Data classification engine
      ↓           Regex + ML: detect PII, PCI, PHI, IP
      ↓           Labels: Public / Internal / Confidential / Secret
  [Middleware] Network DLP (inline proxy)
      ↓           HTTP/HTTPS inspection (TLS interception)
      ↓           Block upload of classified content
  [Middleware] Email DLP
      ↓           SMTP gateway: scan attachments + body
      ↓           Quarantine + notify + encrypt-in-transit
  [Middleware] Endpoint DLP agent
      ↓           Clipboard monitoring, screen capture block
      ↓           Application-level copy-paste control
  [App]       Digital watermarking
      ↓           Invisible marks in documents / images
      ↓           Forensic tracing of leaked document origin
  [App]       Rights Management (IRM/DRM)
                  Microsoft ADRMS / open MRMS
                  Document-level ACL: view-only, no-print, expiry

Data classification matrix:
  Public       → No restrictions
  Internal     → No external upload without approval
  Confidential → Encrypted + need-to-know + audit trail
  Secret       → Air-gapped systems only + hardware token

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ Classification      │ ML classifier + regex rules │
  │ Network DLP         │ Symantec / ForcePoint / own │
  │ Email DLP           │ Proofpoint / Mimecast       │
  │ Endpoint DLP        │ CrowdStrike / SentinelOne   │
  │ USB control         │ udev + usbguard             │
  │ Watermarking        │ Invisible ink / metadata    │
  │ IRM                 │ Azure ADRMS / MIP           │
  └─────────────────────────────────────────────────┘


Use Case UC-08 — Vulnerability Management
UC-08: Vulnerability Management
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Continuously discover, prioritise, and remediate
      vulnerabilities before attackers exploit them.

Attack prevented:
  • Known CVE exploitation
  • Misconfiguration attacks
  • Unpatched dependency exploitation (Log4Shell-style)
  • Supply chain compromise via vulnerable packages

Module map:

  [HW]       Hardware vulnerability scanning (UEFI/firmware)
      ↓           CHIPSEC: firmware security configuration scan
      ↓           FWTS: firmware test suite
  [Firmware]  Firmware update mechanism (FWUPD / LVFS)
      ↓           Signed capsule update with rollback protection
  [Kernel]    Kernel CVE tracking + live patching
      ↓           kpatch / livepatch: zero-downtime kernel patches
  [Middleware] SBOM (Software Bill of Materials) generation
      ↓           Syft / CycloneDX: inventory all dependencies
      ↓           Grype / Trivy: scan SBOM for known CVEs
  [Middleware] Container image scanning
      ↓           Trivy / Clair / Snyk: scan before deployment
      ↓           Admission controller: block vulnerable images
  [Middleware] DAST (Dynamic Application Security Testing)
      ↓           OWASP ZAP / Burp Suite: live attack simulation
  [Middleware] SAST (Static Application Security Testing)
      ↓           CodeQL / Semgrep / SonarQube
  [App]       CVE database integration
      ↓           NVD / OSV / GHSA: continuous feed matching
  [App]       Patch management
                  Automated SLA: Critical=24h, High=7d, Med=30d

CVSS v4 priority tiers:
  Critical (9.0–10.0) → Patch within 24 hours
  High     (7.0– 8.9) → Patch within 7 days
  Medium   (4.0– 6.9) → Patch within 30 days
  Low      (0.1– 3.9) → Patch within 90 days

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ Network scanner     │ OpenVAS / Nessus / Qualys   │
  │ Container scan      │ Trivy / Grype / Snyk        │
  │ SBOM                │ Syft / CycloneDX / SPDX     │
  │ SAST                │ CodeQL / Semgrep            │
  │ DAST                │ OWASP ZAP / Burp            │
  │ Patch mgmt          │ Ansible / Satellite / WSUS  │
  │ Live patch          │ kpatch / ksplice            │
  │ FW update           │ FWUPD + LVFS                │
  └─────────────────────────────────────────────────┘


Use Case UC-09 — Incident Response and Forensics
UC-09: Incident Response and Forensics
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Detect security incidents rapidly, contain damage,
      eradicate the threat, and preserve forensic evidence.

Incident lifecycle (NIST SP 800-61):
  Preparation → Detection → Containment
  → Eradication → Recovery → Post-Incident

Module map:

  [HW]       Tamper-evident hardware logging
      ↓           TPM event log: cryptographic record of boot
  [Firmware]  Firmware forensics snapshot
      ↓           Dump UEFI variables + flash state
  [Kernel]    System call audit trail (auditd)
      ↓           AUSEARCH: query all exec/open/network events
  [Kernel]    Memory forensics
      ↓           LiME: live RAM dump to network / file
      ↓           Volatility 3: kernel object extraction
  [Kernel]    Network forensics
      ↓           tcpdump / Zeek: packet capture + protocol log
      ↓           NetFlow / sFlow: connection metadata
  [Middleware] Centralised log collection
      ↓           Fluent Bit → OpenSearch / Elastic
      ↓           Chain-of-custody: signed, tamper-evident logs
  [Middleware] Malware analysis
      ↓           Cuckoo Sandbox: dynamic analysis
      ↓           YARA: static pattern matching
  [App]       IR playbook automation (SOAR)
      ↓           Auto-contain: isolate host from network
      ↓           Auto-collect: snapshot + ship evidence
  [App]       Digital forensics chain of custody
                  Evidence hash (SHA-256) + timestamp + sign

IR timeline targets (SLA):
  Detection      → < 1 hour (automated SIEM alert)
  Containment    → < 4 hours (network isolation)
  Eradication    → < 24 hours (clean + patch)
  Recovery       → < 72 hours (restore from verified backup)
  Post-incident  → 2 weeks (root cause + lessons learned)

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ SIEM                │ Elastic / Splunk            │
  │ SOAR                │ TheHive + Cortex + Shuffle  │
  │ Memory forensics    │ LiME + Volatility 3         │
  │ Network forensics   │ Zeek + Wireshark + Arkime   │
  │ Malware sandbox     │ Cuckoo / Any.run            │
  │ Log management      │ Fluent Bit + OpenSearch     │
  │ Audit               │ auditd + laureat            │
  │ Case management     │ TheHive / JIRA              │
  └─────────────────────────────────────────────────┘


Use Case UC-10 — Secure Software Supply Chain
UC-10: Secure Software Supply Chain
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Guarantee that all software components — from
      source code to production binary — are authentic,
      unmodified, and from trusted origins.

Attack prevented:
  • SolarWinds-style build system compromise
  • Dependency confusion / typosquatting
  • Malicious open-source package injection
  • CI/CD pipeline hijacking

Module map:

  [HW]       HSM-based code signing key storage
      ↓           Build server private key never leaves HSM
  [Kernel]    Verified kernel modules (DKMS with signing)
      ↓           kernel_lockdown: block unsigned modules
  [Middleware] Source integrity (VCS commit signing)
      ↓           Git commit signing: GPG / SSH / Sigstore Gitsign
      ↓           Branch protection: require signed commits
  [Middleware] Build provenance (SLSA framework)
      ↓           SLSA Level 3: hermetic, reproducible builds
      ↓           in-toto attestation for each build step
  [Middleware] Dependency pinning + verification
      ↓           Hash-pinned lockfiles (package-lock.json, Cargo.lock)
      ↓           Renovate: automated dependency updates
  [Middleware] Artifact signing (Sigstore / cosign)
      ↓           Container images signed: cosign + Rekor transparency log
      ↓           Binaries signed: Sigstore keyless signing
  [Middleware] SBOM generation + attestation
      ↓           CycloneDX SBOM attached to every release
      ↓           VEX (Vulnerability Exploitability eXchange)
  [App]       Policy Enforcement Point (OPA / Kyverno)
      ↓           Admit only signed images with valid attestation
  [App]       Dependency audit
                  npm audit / cargo audit / pip-audit
                  Block CI on new Critical/High CVEs

SLSA levels:
  Level 1 → Provenance generated (any build system)
  Level 2 → Hosted build, signed provenance
  Level 3 → Hardened build, non-forgeable provenance
  Level 4 → Two-party review, hermetic build

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ Signing             │ Sigstore / cosign / GPG     │
  │ Build attestation   │ in-toto / SLSA              │
  │ Transparency log    │ Rekor (Sigstore)            │
  │ SBOM                │ Syft / CycloneDX            │
  │ Policy enforce      │ OPA / Kyverno               │
  │ Dependency audit    │ Dependabot / Renovate       │
  │ Secret scanning     │ GitGuardian / Trufflehog    │
  │ Container signing   │ cosign + Notary v2          │
  └─────────────────────────────────────────────────┘


Use Case UC-11 — Post-Quantum Cryptography Readiness
UC-11: Post-Quantum Cryptography Readiness
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Ensure cryptographic systems remain secure against
      quantum computer attacks (CRQC threat horizon: 2030+).

Attack prevented:
  • "Harvest now, decrypt later" attacks on recorded traffic
  • Future quantum computer breaking RSA/ECC keys
  • Retroactive decryption of archived sensitive data

Timeline:
  Now (2026)       → Inventory + hybrid mode deployment
  2027–2028        → Migrate critical systems to PQC
  2029–2030        → Retire classical algorithms
  2030+            → CRQC threat becomes real

NIST PQC Standards (FIPS 203/204/205, 2024):
  ML-KEM-768       → Key encapsulation (replaces ECDH/RSA-KEM)
  ML-DSA-65        → Digital signatures (replaces ECDSA/RSA-PSS)
  SLH-DSA-SHA2-128 → Stateless hash-based signatures (backup)

Module map:

  [HW]       Crypto engine capability check
      ↓           Does HSM / TPM support ML-KEM / ML-DSA?
      ↓           Software fallback if HW not ready
  [Kernel]    Kernel crypto API — PQC cipher drivers
      ↓           liboqs kernel module
      ↓           kcapi-asym: ML-KEM + ML-DSA support
  [Middleware] Hybrid key exchange in TLS 1.3
      ↓           X25519MLKEM768: classical + PQ in parallel
      ↓           Both must be broken to compromise session
  [Middleware] Hybrid certificates
      ↓           Dual-signed: ECDSA P-384 + ML-DSA-65
      ↓           RFC 9162 / draft-ietf-lamps-dilithium-X509
  [Middleware] Crypto agility framework
      ↓           Algorithm IDs abstracted from application
      ↓           Hot-swap cipher suite without code change
  [App]       Cryptographic inventory (CBOM)
      ↓           Map every algorithm in use + exposure
      ↓           CBOM: Cryptography Bill of Materials
  [App]       PQC migration prioritisation
                  Priority 1: long-lived data (10+ year retention)
                  Priority 2: authentication credentials
                  Priority 3: TLS sessions

Migration phases per data type:
  Public keys / certs → Hybrid now → Pure PQC by 2028
  Stored encrypted data → Re-encrypt with PQC key wrapping
  Code signing → Add ML-DSA co-signature immediately
  VPN / TLS → Hybrid KEM now (X25519+ML-KEM-768)

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ PQC library         │ liboqs / BoringSSL+OQS     │
  │ Hybrid TLS          │ OQS-OpenSSL / Rustls+OQS   │
  │ PQC certs           │ ML-DSA X.509 (draft RFC)   │
  │ Key encap           │ ML-KEM-768 (FIPS 203)       │
  │ Signatures          │ ML-DSA-65 (FIPS 204)        │
  │ Hash-based sig      │ SLH-DSA (FIPS 205) backup  │
  │ Inventory           │ CBOM (CycloneDX crypto ext) │
  │ Agility layer       │ COSE / JOSE algorithm IDs   │
  └─────────────────────────────────────────────────┘


Use Case UC-12 — Privileged Access and Secrets Management
UC-12: Privileged Access and Secrets Management
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Goal: Ensure privileged credentials and application
      secrets are never stored in plaintext, are rotated
      automatically, and all usage is audited.

Attack prevented:
  • Credential theft from config files / environment vars
  • Lateral movement via stolen admin credentials
  • Persistent backdoors via static API keys
  • Insider abuse of shared privileged accounts

Module map:

  [HW]       HSM — master key storage for Vault unseal
      ↓           No software can extract the master key
  [Firmware]  TPM — bind Vault unseal key to platform state
      ↓           Auto-unseal only if boot chain is clean
  [Kernel]    Kernel keyring — in-memory secret cache
      ↓           Secrets never written to disk from kernel ring
  [Middleware] HashiCorp Vault (or equivalent)
      ↓           Dynamic secrets: DB credentials generated
      ↓           on-demand with automatic TTL + revocation
      ↓           Transit engine: encryption-as-a-service
      ↓           PKI engine: short-lived X.509 certificates
  [Middleware] Privileged Access Workstation (PAW)
      ↓           Dedicated hardened host for admin sessions
      ↓           No internet, no email, TPM-attested
  [Middleware] Session recording
      ↓           All privileged sessions recorded + audited
      ↓           CyberArk / Teleport / BeyondTrust
  [App]       Secret injection (no env vars)
      ↓           Vault Agent sidecar: mount secrets as files
      ↓           Kubernetes External Secrets / CSI driver
  [App]       Secret scanning in CI/CD
      ↓           Trufflehog / detect-secrets pre-commit hook
      ↓           Block push if secret detected in diff
  [App]       Break-glass emergency access
                  Dual-approval workflow + full audit
                  Time-limited (4h max) + auto-revoke

Secret types and rotation policy:
  Database passwords    → Vault dynamic, 1-hour TTL
  API keys             → Vault static, 30-day rotation
  TLS certificates     → 24-hour leaf certs (ACME/Vault PKI)
  SSH keys             → Certificate-based, 8-hour validity
  Cloud credentials    → AWS STS AssumeRole, 15-min TTL
  Encryption keys      → Annual rotation + key versioning

Components:
  ┌─────────────────────────────────────────────────┐
  │ Module              │ Technology                  │
  ├─────────────────────────────────────────────────┤
  │ Secrets manager     │ HashiCorp Vault / AWS SM   │
  │ PAM / PAW           │ CyberArk / Teleport        │
  │ Session recording   │ Teleport / CyberArk        │
  │ Secret scanning     │ Trufflehog / GitGuardian   │
  │ K8s integration     │ External Secrets / CSI     │
  │ Rotation automation │ Vault leases + renewers     │
  │ Break-glass         │ Dual-approval ITSM workflow │
  │ Audit               │ Vault audit log → SIEM     │
  └─────────────────────────────────────────────────┘


5. Summary Matrix — Use Cases × Layers
             HW  FW  Kernel  Middle  App
UC-01 Boot   ██  ██   ██      ░░     ██
UC-02 DevID  ██  ██   ░░      ░░     ██
UC-03 Crypt  ██  ██   ██      ██     ██
UC-04 Comms  ██  ░░   ██      ██     ██
UC-05 IAM    ██  ██   ██      ██     ██
UC-06 IDS    ██  ░░   ██      ██     ██
UC-07 DLP    ██  ░░   ██      ██     ██
UC-08 Vuln   ██  ██   ██      ██     ██
UC-09 IR     ░░  ██   ██      ██     ██
UC-10 SupChn ██  ░░   ██      ██     ██
UC-11 PQC    ██  ░░   ██      ██     ██
UC-12 Privil ██  ██   ██      ██     ██

██ = primary ownership   ░░ = supporting role


6. Key Standards and Frameworks
Domain	            Standard / Framework
Crypto primitives	FIPS 140-3, NIST SP 800-series
PQC algorithms	    FIPS 203 (ML-KEM), FIPS 204 (ML-DSA), FIPS 205 (SLH-DSA)
Secure boot	        UEFI spec, TCG PC Client Platform Firmware Profile
Device identity	    IEEE 802.1AR, FIDO FDO, NIST SP 800-193
Zero Trust	        NIST SP 800-207
Supply chain	    SLSA, in-toto, SSDF (NIST SP 800-218)
Incident response	NIST SP 800-61
Risk management	    ISO 27001, NIST CSF 2.0
Cloud security	    CSA CCM, CIS Benchmarks
Compliance	        PCI-DSS 4.0, GDPR, NIS2 (EU), SOC 2 Type II
IoT / embedded	    IEC 62443, ETSI EN 303 645
Vulnerability	    CVSSv4, EPSS, KEV (CISA Known Exploited)
