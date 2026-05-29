/**
 * @file    uiox_kbd_map.h
 * @brief   UIOX Keyboard keymap abstraction.
 *
 * Translates raw hardware scancodes to:
 *   - HID Usage ID keycodes
 *   - Unicode codepoints (with modifier handling)
 *   - Dead-key composition (acute, grave, umlaut, etc.)
 *
 * Supports multiple layouts: QWERTY, QWERTZ (German), AZERTY,
 * Dvorak, Colemak, custom embedded layouts.
 *
 * @date    2026-05-27
 */
//Layer 2b — Keymap
 #ifndef UIOX_KBD_MAP_H
 #define UIOX_KBD_MAP_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * HID Usage IDs (USB HID Keyboard/Keypad Usage Page 0x07)
  * ====================================================================== */
 
 #define UIOX_KEY_NONE           0x00u
 #define UIOX_KEY_A              0x04u
 #define UIOX_KEY_B              0x05u
 #define UIOX_KEY_C              0x06u
 #define UIOX_KEY_D              0x07u
 #define UIOX_KEY_E              0x08u
 #define UIOX_KEY_F              0x09u
 #define UIOX_KEY_G              0x0Au
 #define UIOX_KEY_H              0x0Bu
 #define UIOX_KEY_I              0x0Cu
 #define UIOX_KEY_J              0x0Du
 #define UIOX_KEY_K              0x0Eu
 #define UIOX_KEY_L              0x0Fu
 #define UIOX_KEY_M              0x10u
 #define UIOX_KEY_N              0x11u
 #define UIOX_KEY_O              0x12u
 #define UIOX_KEY_P              0x13u
 #define UIOX_KEY_Q              0x14u
 #define UIOX_KEY_R              0x15u
 #define UIOX_KEY_S              0x16u
 #define UIOX_KEY_T              0x17u
 #define UIOX_KEY_U              0x18u
 #define UIOX_KEY_V              0x19u
 #define UIOX_KEY_W              0x1Au
 #define UIOX_KEY_X              0x1Bu
 #define UIOX_KEY_Y              0x1Cu
 #define UIOX_KEY_Z              0x1Du
 #define UIOX_KEY_1              0x1Eu
 #define UIOX_KEY_2              0x1Fu
 #define UIOX_KEY_3              0x20u
 #define UIOX_KEY_4              0x21u
 #define UIOX_KEY_5              0x22u
 #define UIOX_KEY_6              0x23u
 #define UIOX_KEY_7              0x24u
 #define UIOX_KEY_8              0x25u
 #define UIOX_KEY_9              0x26u
 #define UIOX_KEY_0              0x27u
 #define UIOX_KEY_ENTER          0x28u
 #define UIOX_KEY_ESCAPE         0x29u
 #define UIOX_KEY_BACKSPACE      0x2Au
 #define UIOX_KEY_TAB            0x2Bu
 #define UIOX_KEY_SPACE          0x2Cu
 #define UIOX_KEY_MINUS          0x2Du
 #define UIOX_KEY_EQUAL          0x2Eu
 #define UIOX_KEY_LEFTBRACE      0x2Fu
 #define UIOX_KEY_RIGHTBRACE     0x30u
 #define UIOX_KEY_BACKSLASH      0x31u
 #define UIOX_KEY_SEMICOLON      0x33u
 #define UIOX_KEY_APOSTROPHE     0x34u
 #define UIOX_KEY_GRAVE          0x35u
 #define UIOX_KEY_COMMA          0x36u
 #define UIOX_KEY_DOT            0x37u
 #define UIOX_KEY_SLASH          0x38u
 #define UIOX_KEY_CAPSLOCK       0x39u
 #define UIOX_KEY_F1             0x3Au
 #define UIOX_KEY_F2             0x3Bu
 #define UIOX_KEY_F3             0x3Cu
 #define UIOX_KEY_F4             0x3Du
 #define UIOX_KEY_F5             0x3Eu
 #define UIOX_KEY_F6             0x3Fu
 #define UIOX_KEY_F7             0x40u
 #define UIOX_KEY_F8             0x41u
 #define UIOX_KEY_F9             0x42u
 #define UIOX_KEY_F10            0x43u
 #define UIOX_KEY_F11            0x44u
 #define UIOX_KEY_F12            0x45u
 #define UIOX_KEY_INSERT         0x49u
 #define UIOX_KEY_HOME           0x4Au
 #define UIOX_KEY_PAGEUP         0x4Bu
 #define UIOX_KEY_DELETE         0x4Cu
 #define UIOX_KEY_END            0x4Du
 #define UIOX_KEY_PAGEDOWN       0x4Eu
 #define UIOX_KEY_RIGHT          0x4Fu
 #define UIOX_KEY_LEFT           0x50u
 #define UIOX_KEY_DOWN           0x51u
 #define UIOX_KEY_UP             0x52u
 #define UIOX_KEY_NUMLOCK        0x53u
 #define UIOX_KEY_LCTRL          0xE0u
 #define UIOX_KEY_LSHIFT         0xE1u
 #define UIOX_KEY_LALT           0xE2u
 #define UIOX_KEY_LGUI           0xE3u
 #define UIOX_KEY_RCTRL          0xE4u
 #define UIOX_KEY_RSHIFT         0xE5u
 #define UIOX_KEY_RALT           0xE6u
 #define UIOX_KEY_RGUI           0xE7u
 
 /* =========================================================================
  * Layout identifiers
  * ====================================================================== */
 
 typedef enum {
     UIOX_KBD_LAYOUT_QWERTY = 0,
     UIOX_KBD_LAYOUT_QWERTZ,      /**< German / Central European           */
     UIOX_KBD_LAYOUT_AZERTY,      /**< French                              */
     UIOX_KBD_LAYOUT_DVORAK,
     UIOX_KBD_LAYOUT_COLEMAK,
     UIOX_KBD_LAYOUT_CUSTOM,
 } uiox_kbd_layout_t;
 
 /* =========================================================================
  * Keymap entry
  * ====================================================================== */
 
 typedef struct {
     uint8_t   scancode;      /**< Raw hardware scancode                   */
     uint16_t  keycode;       /**< HID Usage ID                            */
     uint32_t  unicode_base;  /**< Unicode without modifiers               */
     uint32_t  unicode_shift; /**< Unicode with Shift held                 */
     uint32_t  unicode_altgr; /**< Unicode with AltGr (Right Alt)         */
     bool      is_modifier;   /**< true if this key is a modifier          */
     uint8_t   mod_bit;       /**< UIOX_KBD_MOD_* if is_modifier          */
     bool      is_lock;       /**< true if this is a lock key (Caps etc.)  */
 } uiox_kbd_map_entry_t;
 
 /* =========================================================================
  * Keymap table
  * ====================================================================== */
 
 typedef struct {
     uiox_kbd_layout_t      layout;
     const uiox_kbd_map_entry_t *entries;
     uint16_t               num_entries;
     const char            *name;       /**< e.g. "QWERTY-US", "QWERTZ-DE" */
 } uiox_kbd_map_t;
 
 /* =========================================================================
  * Keymap API
  * ====================================================================== */
 
 /** Initialise the keymap subsystem with the given layout. */
 int  uiox_kbd_map_init   (uiox_kbd_layout_t layout);
 
 /** Look up an entry by scancode. Returns NULL if not found. */
 const uiox_kbd_map_entry_t *uiox_kbd_map_lookup(uint8_t scancode);
 
 /**
  * @brief  Translate scancode + modifier state → keycode + unicode.
  * @param  scancode    Raw hardware scancode.
  * @param  modifiers   Active modifier bitmask (UIOX_KBD_MOD_*).
  * @param  caps_lock   CapsLock state.
  * @param  keycode_out HID Usage ID output.
  * @param  unicode_out Unicode codepoint output (0 if none).
  * @return 0 on success, -ENOENT if scancode not in table.
  */
 int  uiox_kbd_map_translate(uint8_t   scancode,
                              uint8_t   modifiers,
                              bool      caps_lock,
                              uint16_t *keycode_out,
                              uint32_t *unicode_out);
 
 /** Return name of current layout. */
 const char *uiox_kbd_map_name(void);
 
 /** Switch to a different layout at runtime. */
 int  uiox_kbd_map_set_layout(uiox_kbd_layout_t layout);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_KBD_MAP_H */
 