Application / Device Access API       (uiox_wifi_device)
  → WiFi Subsystem: scan, connect, DHCP, security     (uiox_wifi_subsys)
    → WiFi Protocol: 802.11 frames, auth, assoc, EAPOL (uiox_wifi_proto)
    → Security: WPA2/WPA3, CCMP, TKIP, PMKSA cache    (uiox_wifi_sec)
    → Interface driver: TX/RX queue, rate control       (uiox_wifi_if)
      → Hardware Abstraction: MMIO, DMA, IRQ, PLL       (uiox_wifi_hw)
    ↔ Buffer Manager: MPDU/MSDU frame pool              (uiox_wifi_buf)
/////////////////////////////
uiox-wifi/
├── include/
│   ├── uiox_wifi_hw.h         # Layer 1  — HAL: MMIO, DMA, IRQ, RF, PLL
│   ├── uiox_wifi_buf.h        # MPDU/MSDU frame buffer pool
│   ├── uiox_wifi_if.h         # Layer 2  — Interface driver (TX/RX FIFO)
│   ├── uiox_wifi_sec.h        # WPA2/WPA3 security: CCMP, EAPOL, PMKSA
│   ├── uiox_wifi_proto.h      # Layer 3  — 802.11 protocol: frames, auth
│   ├── uiox_wifi_subsys.h     # Layer 4  — Subsystem: scan, connect, AP
│   └── uiox_wifi_device.h     # Layer 5  — Application-facing API
└── src/
    ├── uiox_wifi_hw.c
    ├── uiox_wifi_buf.c
    ├── uiox_wifi_if.c
    ├── uiox_wifi_sec.c
    ├── uiox_wifi_proto.c
    ├── uiox_wifi_subsys.c
    ├── uiox_wifi_device.c
    └── uiox_wifi_demo.c
///////////////////////////////////
uiox-wifi/
├── include/
│   ├── uiox_wifi_hw.h      # Layer 1   — HAL: MMIO regs, RF/PLL,
│   │                       #              DMA TX/RX rings, IRQ top-half,
│   │                       #              channel/freq, TX power, MIMO,
│   │                       #              SDIO/SPI/PCIe bus ops vtable
│   ├── uiox_wifi_buf.h     # Layer 1.5 — MPDU/MSDU frame pool (TX+RX),
│   │                       #              zero-copy push/pull/put,
│   │                       #              headroom for MAC header prepend
│   ├── uiox_wifi_if.h      # Layer 2   — Interface driver: WMM AC queues
│   │                       #              (VO/VI/BE/BK), ARF rate control,
│   │                       #              A-MPDU scheduling, IF statistics
│   ├── uiox_wifi_sec.h     # Layer 2b  — Security: WPA2-PSK / WPA3-SAE,
│   │                       #              PBKDF2-SHA1 PMK, PRF-512 PTK,
│   │                       #              CCMP-128 enc/dec, 4-way HS,
│   │                       #              PMKSA cache, replay protection
│   ├── uiox_wifi_proto.h   # Layer 3   — 802.11 protocol: frame format,
│   │                       #              beacon/probe parse, auth/assoc,
│   │                       #              deauth, LLC/SNAP, QoS data,
│   │                       #              BSS cache, connection state machine
│   ├── uiox_wifi_subsys.h  # Layer 4   — Subsystem: scan, connect,
│   │                       #              auto-reconnect, background scan,
│   │                       #              RSSI quality, power save mode,
│   │                       #              WiFi event callbacks
│   └── uiox_wifi_device.h  # Layer 5   — Application API: open/start/stop/
│                           #              close/scan/connect/disconnect/
│                           #              tick/tx/connected/get_quality/
│                           #              get_mac/bss_list/print_stats
└── src/
    ├── uiox_wifi_hw.c      # HAL lifecycle: init/deinit/start/stop,
    │                       #   set_channel/set_mode/get_rssi
    ├── uiox_wifi_buf.c     # Static TX+RX frame pool: build_pool,
    │                       #   alloc/free/ref, push/pull/put (zero-copy)
    ├── uiox_wifi_if.c      # WMM AC TX queue push/pop, priority flush
    │                       #   (VO→VI→BE→BK), RX poll+copy, ARF rate ctrl
    ├── uiox_wifi_sec.c     # SHA-1 + HMAC-SHA1 + PRF-512 + PBKDF2-SHA1,
    │                       #   PMK/PTK derivation, CCMP enc/dec,
    │                       #   4-way HS state machine, PMKSA cache
    ├── uiox_wifi_proto.c   # MAC frame builder, beacon/probe parser,
    │                       #   auth/assoc/deauth builder, scan engine,
    │                       #   connect state machine, RX dispatcher,
    │                       #   LLC/SNAP encapsulation, QoS data TX
    ├── uiox_wifi_subsys.c  # Subsystem init/start/stop, scan, connect,
    │                       #   disconnect, tick (auto-reconnect + bg scan
    │                       #   + quality update + RSSI alert), event fire
    ├── uiox_wifi_device.c  # Device open/close, all API wrappers,
    │                       #   state_name/evt_name string helpers,
    │                       #   print_stats
    └── uiox_wifi_demo.c    # End-to-end demo: stub SDIO HAL + simulated
                            #   beacon/auth/assoc/EAPOL frames, scan,
                            #   WPA2 connect, ARP+IPv4 TX, tick loop,
                            #   quality metrics, disconnect
////////////////
Key Design Decisions
Decision,Rationale
Dual static TX/RX frame pools,"Prevents heap fragmentation; TX pool sized for burst (32 frames), RX pool deeper (64 frames) to absorb bursty beacon/data traffic"
Zero-copy push/pull/put,MAC header prepended by moving data pointer back into headroom — no memcpy on the TX path; critical for 802.11ac/ax throughput
WMM AC priority queues,Four hardware-mapped queues (VO/VI/BE/BK) serviced in strict priority order; prevents voice traffic starvation
ARF rate control,"Auto Rate Fallback: probe higher rate every N successes, fall back after 3 failures — simple, deterministic, no floating-point"
Self-contained SHA-1 + HMAC,Zero external crypto dependency for embedded targets; replace with mbedTLS/WolfSSL hardware-accelerated backend in production
PBKDF2-SHA1 PMK derivation,IEEE 802.11-2020 §J.4.1 compliant; 4096 iterations as specified for WPA2-Personal
PRF-512 PTK derivation,KCK/KEK/TK correctly split from 64-byte PRF output; canonical MAC/nonce ordering for cross-platform interoperability
PMKSA cache (4 entries),Enables fast roam/reconnect by caching PMK; evicts oldest on overflow; PMKID computed per IEEE 802.11-2020 §J.4.2
4-way handshake state machine,msg1→msg2→msg3→complete mapped to enum states; MIC verification via HMAC-SHA1(KCK); replay counter checked
CCMP-128 PN replay protection,Strictly monotonic PN enforced per-frame; -EBADMSG on replay — prevents replay attacks without kernel intervention
BSS cache (32 entries),Scan results retained between scan cycles; entries matched by BSSID and updated in-place to preserve RSSI history
Auto-reconnect + backoff,Configurable reconnect_delay_ms; reconnect count tracked in quality metrics; fires UIOX_WIFI_EVT_DISCONNECTED on each attempt
RSSI quality mapping,Linear −90 dBm=0% → −40 dBm=100%; fires UIOX_WIFI_EVT_RSSI_LOW below −80 dBm for application-level roaming trigger
Vtable ops pattern,"Real SDIO (CYW43xx), SPI (ESP8266), PCIe (Intel AX200) drivers plug in without modifying upper layers"
///////////////////
