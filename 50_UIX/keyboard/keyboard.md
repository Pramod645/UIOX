Application / Device Access API     (uiox_kbd_device)
  → Keyboard Subsystem: keymaps, repeat, shortcuts  (uiox_kbd_subsys)
    → Key Event Processing: debounce, scan, encode  (uiox_kbd_event)
    → Keymap abstraction: layout, modifier, unicode  (uiox_kbd_map)
    → Interface driver: matrix scan, GPIO, I2C, SPI  (uiox_kbd_if)
      → Hardware Abstraction: MMIO, GPIO, IRQ, timer  (uiox_kbd_hw)
    ↔ Buffer Manager: key event ring buffer           (uiox_kbd_buf)
/////////
uiox-keyboard/
├── include/
│   ├── uiox_kbd_hw.h         # Layer 1  — HAL: GPIO, timer, IRQ, MMIO
│   ├── uiox_kbd_buf.h        # Key event ring buffer
│   ├── uiox_kbd_if.h         # Layer 2  — Interface driver (matrix/GPIO/I2C)
│   ├── uiox_kbd_map.h        # Keymap: layout, modifiers, unicode
│   ├── uiox_kbd_event.h      # Layer 3  — Event: debounce, encode, repeat
│   ├── uiox_kbd_subsys.h     # Layer 4  — Subsystem: shortcuts, LED, locking
│   └── uiox_kbd_device.h     # Layer 5  — Application-facing API
└── src/
    ├── uiox_kbd_hw.c
    ├── uiox_kbd_buf.c
    ├── uiox_kbd_if.c
    ├── uiox_kbd_map.c
    ├── uiox_kbd_event.c
    ├── uiox_kbd_subsys.c
    ├── uiox_kbd_device.c
    └── uiox_kbd_demo.c
///////////
uiox-keyboard/
├── include/
│   ├── uiox_kbd_hw.h       # Layer 1   — HAL: GPIO row/col drive, timer,
│   │                       #              LED pins, PS/2, I2C/SPI ops vtable
│   │                       #              matrix cfg, direct GPIO, backlight
│   ├── uiox_kbd_buf.h      # Layer 1.5 — Lock-free SPSC ring buffer,
│   │                       #              key event struct, modifier bitmasks
│   ├── uiox_kbd_if.h       # Layer 2   — Interface driver: full matrix scan,
│   │                       #              ghost-key filter, direct GPIO poll,
│   │                       #              I2C (TCA8418) poll, PS/2 reception
│   ├── uiox_kbd_map.h      # Layer 2b  — Keymap: HID Usage IDs, QWERTY-US,
│   │                       #              QWERTZ-DE, layout registry,
│   │                       #              modifier + CapsLock translation,
│   │                       #              AltGr / unicode codepoints
│   ├── uiox_kbd_event.h    # Layer 3   — Event processing: debounce slots,
│   │                       #              modifier state, lock key toggles,
│   │                       #              N-key rollover, auto-repeat timer
│   ├── uiox_kbd_subsys.h   # Layer 4   — Subsystem: scan→debounce→keymap
│   │                       #              pipeline, shortcut dispatch,
│   │                       #              event callbacks, LED sync,
│   │                       #              backlight auto-dim, statistics
│   └── uiox_kbd_device.h   # Layer 5   — Application API: open/start/stop/
│                           #              close/tick/poll/add_shortcut/
│                           #              add_callback/set_layout/set_leds/
│                           #              set_backlight/modifiers/lock_state
└── src/
    ├── uiox_kbd_hw.c       # HAL lifecycle: init/deinit, scan_row,
    │                       #   read_direct, set_leds, set_backlight
    ├── uiox_kbd_buf.c      # SPSC ring buffer: init/push/pop/peek/
    │                       #   empty/count/flush — power-of-2 mask indexing
    ├── uiox_kbd_if.c       # Matrix scan (all rows + ghost filter),
    │                       #   direct GPIO poll, I2C TCA8418 event reg,
    │                       #   PS/2 set-2 break-code handling
    ├── uiox_kbd_map.c      # QWERTY-US + QWERTZ-DE keymap tables,
    │                       #   layout registry, translate() with
    │                       #   shift/AltGr/CapsLock logic
    ├── uiox_kbd_event.c    # Debounce state machine per scancode,
    │                       #   modifier/lock tracking, rollover array,
    │                       #   auto-repeat delay + interval timer,
    │                       #   emit() resolves keycode + unicode
    ├── uiox_kbd_subsys.c   # Full pipeline tick, shortcut dispatch,
    │                       #   callback dispatch, LED sync, backlight dim,
    │                       #   per-event statistics
    ├── uiox_kbd_device.c   # Device open/close, all API wrappers,
    │                       #   print_stats(), layout_name()
    └── uiox_kbd_demo.c     # End-to-end demo: 8×8 matrix with stub HAL,
                            #   simulated key sequence, Ctrl+C/Z shortcuts,
                            #   layout switch, CapsLock, LED sync, stats
///////////////////////
build/
└── X86_64/              ← or ARM64 / ARM32
    └── debug/           ← or release
        ├── obj/
        │   ├── uiox_kbd_hw.o      + .d
        │   ├── uiox_kbd_buf.o     + .d
        │   ├── uiox_kbd_if.o      + .d
        │   ├── uiox_kbd_map.o     + .d
        │   ├── uiox_kbd_event.o   + .d
        │   ├── uiox_kbd_subsys.o  + .d
        │   ├── uiox_kbd_device.o  + .d
        │   └── uiox_kbd_demo.o    + .d
        ├── lib/
        │   └── libuiox_kbd.a
        └── bin/
            └── uiox_kbd_demo
//////////////////
Makefile Usage Reference
Command,Effect
make,Native debug build (auto-detects platform)
make BUILD=release,"Optimised, stripped release build"
make PLATFORM=ARM64 CROSS=aarch64-linux-gnu-,Cross-compile for ARM64
make PLATFORM=ARM32 CROSS=arm-linux-gnueabihf-,Cross-compile for ARM32
make lib,Build static library only
make demo,Build demo binary (depends on lib)
make install PREFIX=/opt/uiox,Install headers + lib + binary
make uninstall PREFIX=/opt/uiox,Remove installed files
make size,Print text/data/bss breakdown
make dump,Generate disassembly → .asm file
make lint,Run cppcheck static analysis
make format,Auto-format all sources with clang-format
make docs,Generate Doxygen HTML documentation
make tags,Generate ctags index
make info,Print full build configuration summary
make clean,Remove current platform/mode build artefacts
make distclean,Remove entire build/ tree
make V=1,Verbose — print full compiler commands

//////////
Key Design Decisions
Decision,Rationale
SPSC lock-free ring buffer,Single-producer (ISR/scanner) / single-consumer (application) — no mutex needed; power-of-2 mask eliminates modulo division
Two-stage ring buffer pipeline,raw_rb holds unprocessed scan events; cooked_rb holds fully resolved events — decouples scan rate from application processing rate
Debounce per scancode slot,Each key has its own debounce timer rather than a global one; simultaneous multi-key presses all debounce correctly and independently
Scancode → HID Usage ID mapping,Direct HID Usage ID keycodes are OS-portable and match USB HID reports; no secondary translation needed for USB bridge use
AltGr / Shift / CapsLock in one table,"Each keymap entry carries unicode_base, unicode_shift, unicode_altgr; translation is a single lookup avoiding cascaded conditionals"
Two built-in layouts (QWERTY + QWERTZ),German embedded targets need QWERTZ; the registry pattern allows adding Dvorak/AZERTY/custom without modifying core code
N-key rollover array,held_scancodes[6] tracks all simultaneously pressed keys; repeat timer attaches to the most recently pressed key only
Auto-repeat delay + interval,"Matches standard keyboard behaviour (500 ms initial delay, 33 ms interval = ~30 Hz); fully configurable per-device"
Shortcut dispatch before application,Shortcuts fire before events reach the application ring buffer; no missed shortcuts even if application is slow to poll
LED sync on every tick,Lock key state changes are immediately reflected in hardware LEDs without a separate LED-update call from the application
Backlight auto-dim,Inactivity timer dims backlight to a configurable level; any key press immediately restores full brightness — saves power on embedded targets
Vtable ops pattern,"Real drivers (STM32 GPIO, RP2040 PIO, NXP GPIO, I2C TCA8418, PS/2) plug in without modifying upper layers"
Ghost-key detection ready,Matrix state is stored per-row as bitmasks; full KRO ghost detection can be added in scan_matrix() without API changes