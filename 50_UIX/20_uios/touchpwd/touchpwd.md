Application / Device Access API        (uiox_tpwd_device)
  → Password Subsystem: entry, verify, lockout, audit  (uiox_tpwd_subsys)
    → Gesture Engine: pattern, PIN, swipe recognition   (uiox_tpwd_gesture)
    → Security Engine: hash, salt, timing-safe compare  (uiox_tpwd_sec)
    → Interface driver: touch scan, coordinate map      (uiox_tpwd_if)
      → Hardware Abstraction: MMIO, I2C, IRQ, GPIO      (uiox_tpwd_hw)
    ↔ Buffer Manager: touch event + credential pool     (uiox_tpwd_buf)
================================================================
uiox-touchpwd/
├── include/
│   ├── uiox_tpwd_hw.h         # Layer 1  — HAL: I2C, GPIO, IRQ, power
│   ├── uiox_tpwd_buf.h        # Touch event + credential buffer pool
│   ├── uiox_tpwd_if.h         # Layer 2  — Interface driver (scan, map)
│   ├── uiox_tpwd_sec.h        # Security: hash, HMAC, timing-safe
│   ├── uiox_tpwd_gesture.h    # Layer 3  — Gesture: PIN, pattern, swipe
│   ├── uiox_tpwd_subsys.h     # Layer 4  — Subsystem: lockout, audit
│   └── uiox_tpwd_device.h     # Layer 5  — Application-facing API
└── src/
    ├── uiox_tpwd_hw.c
    ├── uiox_tpwd_buf.c
    ├── uiox_tpwd_if.c
    ├── uiox_tpwd_sec.c
    ├── uiox_tpwd_gesture.c
    ├── uiox_tpwd_subsys.c
    ├── uiox_tpwd_device.c
    └── uiox_tpwd_demo.c
==================================
uiox-touchpwd/
├── include/
│   ├── uiox_tpwd_hw.h      # Layer 1   — HAL: I2C reg access, GPIO RST/INT,
│   │                       #              touch read, sensitivity, backlight,
│   │                       #              panel geometry/rotation, ops vtable
│   ├── uiox_tpwd_buf.h     # Layer 1.5 — SPSC event ring buffer (64-deep),
│   │                       #              credential pool (zeroed on free)
│   ├── uiox_tpwd_if.h      # Layer 2   — Interface: scan, debounce, hold,
│   │                       #              lift-off timeout, coordinate norm,
│   │                       #              3×4 grid cell mapping, stats
│   ├── uiox_tpwd_sec.h     # Layer 2b  — Security: SHA-256, HMAC-SHA-256,
│   │                       #              PBKDF2 (4096 iter), random salt,
│   │                       #              timing-safe compare, enrol/verify,
│   │                       #              per-record + global lockout,
│   │                       #              session token, secure zero
│   ├── uiox_tpwd_gesture.h # Layer 3   — Gesture engine: PIN (0-9,*,#),
│   │                       #              pattern (3×3 connect-the-dots),
│   │                       #              swipe (4-direction), serialise
│   ├── uiox_tpwd_subsys.h  # Layer 4   — Subsystem: full pipeline tick,
│   │                       #              enrol/verify workflows, lockout,
│   │                       #              session management, backlight,
│   │                       #              audit log (16 entries), events
│   └── uiox_tpwd_device.h  # Layer 5   — Application API: open/start/stop/
│                           #              close/tick/enrol/verify/delete/
│                           #              logout/authenticated/state/
│                           #              attempts_left/set_backlight/
│                           #              print_stats/print_audit
└── src/
    ├── uiox_tpwd_hw.c      # HAL lifecycle, power, reset, read_touch,
    │                       #   set_backlight — all via ops vtable
    ├── uiox_tpwd_buf.c     # SPSC ring buffer push/pop/empty/count;
    │                       #   credential pool alloc/free with secure zero
    ├── uiox_tpwd_if.c      # Grid rotation/flip, cell mapping, debounce
    │                       #   FSM, hold timer, timeout sentinel
    ├── uiox_tpwd_sec.c     # SHA-256 compress, HMAC-SHA-256, PBKDF2-SHA-256,
    │                       #   LFSR PRNG, timing-safe compare, enrol/verify
    │                       #   with lockout, session token, secure zero
    ├── uiox_tpwd_gesture.c # PIN (append/backspace/'#'=submit), pattern
    │                       #   (dedup node visit, lift=complete), swipe
    │                       #   (dx/dy threshold), serialise to raw bytes
    ├── uiox_tpwd_subsys.c  # Tick pipeline: scan→gesture→serialise→
    │                       #   hash→verify/enrol, lockout check, session,
    │                       #   event fire, audit push, backlight control
    ├── uiox_tpwd_device.c  # All API wrappers, secure zero on close,
    │                       #   print_stats, print_audit, name helpers
    └── uiox_tpwd_demo.c    # FT6336 stub HAL, simulated touch sequences,
                            #   PIN enrol/correct/wrong/lockout,
                            #   pattern enrol/verify, session, audit log
