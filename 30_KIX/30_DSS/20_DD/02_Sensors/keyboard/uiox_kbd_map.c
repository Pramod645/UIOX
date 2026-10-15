/**
 * @file    uiox_kbd_map.c
 * @brief   UIOX Keyboard keymap implementation.
 * @date    2026-05-27
 */

 #include "uiox_kbd_map.h"
 #include "uiox_kbd_buf.h"

 
 /* =========================================================================
  * QWERTY-US keymap table (partial — most common keys)
  * ====================================================================== */
 
 static const uiox_kbd_map_entry_t s_qwerty_us[] = {
  /* sc        keycode           base   shift   altgr  mod   mbit  lock */
  { 0x04, UIOX_KEY_A,        'a',   'A',   0,     false, 0,   false },
  { 0x05, UIOX_KEY_B,        'b',   'B',   0,     false, 0,   false },
  { 0x06, UIOX_KEY_C,        'c',   'C',   0,     false, 0,   false },
  { 0x07, UIOX_KEY_D,        'd',   'D',   0,     false, 0,   false },
  { 0x08, UIOX_KEY_E,        'e',   'E',   0,     false, 0,   false },
  { 0x09, UIOX_KEY_F,        'f',   'F',   0,     false, 0,   false },
  { 0x0A, UIOX_KEY_G,        'g',   'G',   0,     false, 0,   false },
  { 0x0B, UIOX_KEY_H,        'h',   'H',   0,     false, 0,   false },
  { 0x0C, UIOX_KEY_I,        'i',   'I',   0,     false, 0,   false },
  { 0x0D, UIOX_KEY_J,        'j',   'J',   0,     false, 0,   false },
  { 0x0E, UIOX_KEY_K,        'k',   'K',   0,     false, 0,   false },
  { 0x0F, UIOX_KEY_L,        'l',   'L',   0,     false, 0,   false },
  { 0x10, UIOX_KEY_M,        'm',   'M',   0,     false, 0,   false },
  { 0x11, UIOX_KEY_N,        'n',   'N',   0,     false, 0,   false },
  { 0x12, UIOX_KEY_O,        'o',   'O',   0,     false, 0,   false },
  { 0x13, UIOX_KEY_P,        'p',   'P',   0,     false, 0,   false },
  { 0x14, UIOX_KEY_Q,        'q',   'Q',   0,     false, 0,   false },
  { 0x15, UIOX_KEY_R,        'r',   'R',   0,     false, 0,   false },
  { 0x16, UIOX_KEY_S,        's',   'S',   0,     false, 0,   false },
  { 0x17, UIOX_KEY_T,        't',   'T',   0,     false, 0,   false },
  { 0x18, UIOX_KEY_U,        'u',   'U',   0,     false, 0,   false },
  { 0x19, UIOX_KEY_V,        'v',   'V',   0,     false, 0,   false },
  { 0x1A, UIOX_KEY_W,        'w',   'W',   0,     false, 0,   false },
  { 0x1B, UIOX_KEY_X,        'x',   'X',   0,     false, 0,   false },
  { 0x1C, UIOX_KEY_Y,        'y',   'Y',   0,     false, 0,   false },
  { 0x1D, UIOX_KEY_Z,        'z',   'Z',   0,     false, 0,   false },
  { 0x1E, UIOX_KEY_1,        '1',   '!',   0,     false, 0,   false },
  { 0x1F, UIOX_KEY_2,        '2',   '@',   0,     false, 0,   false },
  { 0x20, UIOX_KEY_3,        '3',   '#',   0,     false, 0,   false },
  { 0x21, UIOX_KEY_4,        '4',   '$',   0,     false, 0,   false },
  { 0x22, UIOX_KEY_5,        '5',   '%',   0,     false, 0,   false },
  { 0x23, UIOX_KEY_6,        '6',   '^',   0,     false, 0,   false },
  { 0x24, UIOX_KEY_7,        '7',   '&',   0,     false, 0,   false },
  { 0x25, UIOX_KEY_8,        '8',   '*',   0,     false, 0,   false },
  { 0x26, UIOX_KEY_9,        '9',   '(',   0,     false, 0,   false },
  { 0x27, UIOX_KEY_0,        '0',   ')',   0,     false, 0,   false },
  { 0x28, UIOX_KEY_ENTER,    '\n',  '\n',  0,     false, 0,   false },
  { 0x29, UIOX_KEY_ESCAPE,   0x1B,  0x1B,  0,     false, 0,   false },
  { 0x2A, UIOX_KEY_BACKSPACE,0x08,  0x08,  0,     false, 0,   false },
  { 0x2B, UIOX_KEY_TAB,      '\t',  '\t',  0,     false, 0,   false },
  { 0x2C, UIOX_KEY_SPACE,    ' ',   ' ',   0,     false, 0,   false },
  { 0x2D, UIOX_KEY_MINUS,    '-',   '_',   0,     false, 0,   false },
  { 0x2E, UIOX_KEY_EQUAL,    '=',   '+',   0,     false, 0,   false },
  { 0x2F, UIOX_KEY_LEFTBRACE,'[',   '{',   0,     false, 0,   false },
  { 0x30, UIOX_KEY_RIGHTBRACE,']',  '}',   0,     false, 0,   false },
  { 0x31, UIOX_KEY_BACKSLASH,'\\',  '|',   0,     false, 0,   false },
  { 0x33, UIOX_KEY_SEMICOLON,';',   ':',   0,     false, 0,   false },
  { 0x34, UIOX_KEY_APOSTROPHE,'\'', '"',   0,     false, 0,   false },
  { 0x35, UIOX_KEY_GRAVE,    '`',   '~',   0,     false, 0,   false },
  { 0x36, UIOX_KEY_COMMA,    ',',   '<',   0,     false, 0,   false },
  { 0x37, UIOX_KEY_DOT,      '.',   '>',   0,     false, 0,   false },
  { 0x38, UIOX_KEY_SLASH,    '/',   '?',   0,     false, 0,   false },
  /* Lock keys */
  { 0x39, UIOX_KEY_CAPSLOCK, 0,     0,     0,     false, 0,   true  },
  { 0x53, UIOX_KEY_NUMLOCK,  0,     0,     0,     false, 0,   true  },
  /* Modifiers */
  { 0xE0, UIOX_KEY_LCTRL,    0,     0,     0,     true,  UIOX_KBD_MOD_LCTRL,  false },
  { 0xE1, UIOX_KEY_LSHIFT,   0,     0,     0,     true,  UIOX_KBD_MOD_LSHIFT, false },
  { 0xE2, UIOX_KEY_LALT,     0,     0,     0,     true,  UIOX_KBD_MOD_LALT,   false },
  { 0xE3, UIOX_KEY_LGUI,     0,     0,     0,     true,  UIOX_KBD_MOD_LGUI,   false },
  { 0xE4, UIOX_KEY_RCTRL,    0,     0,     0,     true,  UIOX_KBD_MOD_RCTRL,  false },
  { 0xE5, UIOX_KEY_RSHIFT,   0,     0,     0,     true,  UIOX_KBD_MOD_RSHIFT, false },
  { 0xE6, UIOX_KEY_RALT,     0,     0,     0,     true,  UIOX_KBD_MOD_RALT,   false },
  { 0xE7, UIOX_KEY_RGUI,     0,     0,     0,     true,  UIOX_KBD_MOD_RGUI,   false },
  /* Function keys */
  { 0x3A, UIOX_KEY_F1,       0,     0,     0,     false, 0,   false },
  { 0x3B, UIOX_KEY_F2,       0,     0,     0,     false, 0,   false },
  { 0x3C, UIOX_KEY_F3,       0,     0,     0,     false, 0,   false },
  { 0x3D, UIOX_KEY_F4,       0,     0,     0,     false, 0,   false },
  { 0x3E, UIOX_KEY_F5,       0,     0,     0,     false, 0,   false },
  { 0x3F, UIOX_KEY_F6,       0,     0,     0,     false, 0,   false },
  { 0x40, UIOX_KEY_F7,       0,     0,     0,     false, 0,   false },
  { 0x41, UIOX_KEY_F8,       0,     0,     0,     false, 0,   false },
  { 0x42, UIOX_KEY_F9,       0,     0,     0,     false, 0,   false },
  { 0x43, UIOX_KEY_F10,      0,     0,     0,     false, 0,   false },
  { 0x44, UIOX_KEY_F11,      0,     0,     0,     false, 0,   false },
  { 0x45, UIOX_KEY_F12,      0,     0,     0,     false, 0,   false },
  /* Navigation */
  { 0x4F, UIOX_KEY_RIGHT,    0,     0,     0,     false, 0,   false },
  { 0x50, UIOX_KEY_LEFT,     0,     0,     0,     false, 0,   false },
  { 0x51, UIOX_KEY_DOWN,     0,     0,     0,     false, 0,   false },
  { 0x52, UIOX_KEY_UP,       0,     0,     0,     false, 0,   false },
  { 0x49, UIOX_KEY_INSERT,   0,     0,     0,     false, 0,   false },
  { 0x4A, UIOX_KEY_HOME,     0,     0,     0,     false, 0,   false },
  { 0x4B, UIOX_KEY_PAGEUP,   0,     0,     0,     false, 0,   false },
  { 0x4C, UIOX_KEY_DELETE,   0x7F,  0x7F,  0,     false, 0,   false },
  { 0x4D, UIOX_KEY_END,      0,     0,     0,     false, 0,   false },
  { 0x4E, UIOX_KEY_PAGEDOWN, 0,     0,     0,     false, 0,   false },
 };
 
/* =========================================================================
 * QWERTZ-DE keymap (key differences from QWERTY-US)
 * Y ↔ Z swapped, ä/ö/ü added, § on key 1 position, etc.
 * ====================================================================== */

 static const uiox_kbd_map_entry_t s_qwertz_de[] = {
    /* sc        keycode           base    shift   altgr   mod   mbit  lock */
    { 0x04, UIOX_KEY_A,        'a',    'A',    0,      false, 0,   false },
    { 0x05, UIOX_KEY_B,        'b',    'B',    0,      false, 0,   false },
    { 0x06, UIOX_KEY_C,        'c',    'C',    0,      false, 0,   false },
    { 0x07, UIOX_KEY_D,        'd',    'D',    0,      false, 0,   false },
    { 0x08, UIOX_KEY_E,        'e',    'E',    0x20ACu,false, 0,   false }, /* € on AltGr+E */
    { 0x09, UIOX_KEY_F,        'f',    'F',    0,      false, 0,   false },
    { 0x0A, UIOX_KEY_G,        'g',    'G',    0,      false, 0,   false },
    { 0x0B, UIOX_KEY_H,        'h',    'H',    0,      false, 0,   false },
    { 0x0C, UIOX_KEY_I,        'i',    'I',    0,      false, 0,   false },
    { 0x0D, UIOX_KEY_J,        'j',    'J',    0,      false, 0,   false },
    { 0x0E, UIOX_KEY_K,        'k',    'K',    0,      false, 0,   false },
    { 0x0F, UIOX_KEY_L,        'l',    'L',    0,      false, 0,   false },
    { 0x10, UIOX_KEY_M,        'm',    'M',    0x00B5u,false, 0,   false }, /* µ on AltGr+M */
    { 0x11, UIOX_KEY_N,        'n',    'N',    0,      false, 0,   false },
    { 0x12, UIOX_KEY_O,        'o',    'O',    0,      false, 0,   false },
    { 0x13, UIOX_KEY_P,        'p',    'P',    0,      false, 0,   false },
    { 0x14, UIOX_KEY_Q,        'q',    'Q',    '@',    false, 0,   false }, /* @ on AltGr+Q */
    { 0x15, UIOX_KEY_R,        'r',    'R',    0,      false, 0,   false },
    { 0x16, UIOX_KEY_S,        's',    'S',    0,      false, 0,   false },
    { 0x17, UIOX_KEY_T,        't',    'T',    0,      false, 0,   false },
    { 0x18, UIOX_KEY_U,        'u',    'U',    0,      false, 0,   false },
    { 0x19, UIOX_KEY_V,        'v',    'V',    0,      false, 0,   false },
    { 0x1A, UIOX_KEY_W,        'w',    'W',    0,      false, 0,   false },
    { 0x1B, UIOX_KEY_X,        'x',    'X',    0,      false, 0,   false },
    { 0x1C, UIOX_KEY_Z,        'y',    'Y',    0,      false, 0,   false }, /* Z key = y */
    { 0x1D, UIOX_KEY_Y,        'z',    'Z',    0,      false, 0,   false }, /* Y key = z */
    { 0x1E, UIOX_KEY_1,        '1',    '!',    0,      false, 0,   false },
    { 0x1F, UIOX_KEY_2,        '2',    '"',    0x00B2u,false, 0,   false }, /* ² on AltGr+2 */
    { 0x20, UIOX_KEY_3,        '3',    0x00A7u,'3',    false, 0,   false }, /* § on shift+3 */
    { 0x21, UIOX_KEY_4,        '4',    '$',    0,      false, 0,   false },
    { 0x22, UIOX_KEY_5,        '5',    '%',    0,      false, 0,   false },
    { 0x23, UIOX_KEY_6,        '6',    '&',    0,      false, 0,   false },
    { 0x24, UIOX_KEY_7,        '7',    '/',    '{',    false, 0,   false },
    { 0x25, UIOX_KEY_8,        '8',    '(',    '[',    false, 0,   false },
    { 0x26, UIOX_KEY_9,        '9',    ')',    ']',    false, 0,   false },
    { 0x27, UIOX_KEY_0,        '0',    '=',    '}',    false, 0,   false },
    { 0x2C, UIOX_KEY_SPACE,    ' ',    ' ',    0,      false, 0,   false },
    { 0x28, UIOX_KEY_ENTER,    '\n',   '\n',   0,      false, 0,   false },
    { 0x29, UIOX_KEY_ESCAPE,   0x1B,   0x1B,   0,      false, 0,   false },
    { 0x2A, UIOX_KEY_BACKSPACE,0x08,   0x08,   0,      false, 0,   false },
    { 0x2B, UIOX_KEY_TAB,      '\t',   '\t',   0,      false, 0,   false },
    /* ä, ö, ü — mapped to SEMICOLON/APOSTROPHE/LEFTBRACE positions */
    { 0x33, UIOX_KEY_SEMICOLON,0x00F6u,0x00D6u,0,      false, 0,   false }, /* ö Ö */
    { 0x34, UIOX_KEY_APOSTROPHE,0x00E4u,0x00C4u,0,     false, 0,   false }, /* ä Ä */
    { 0x2F, UIOX_KEY_LEFTBRACE,0x00FCu,0x00DCu,0,      false, 0,   false }, /* ü Ü */
    { 0x2D, UIOX_KEY_MINUS,    0x00DFu,'?',    '\\',   false, 0,   false }, /* ß ? \ */
    { 0x39, UIOX_KEY_CAPSLOCK, 0,      0,      0,      false, 0,   true  },
    { 0xE0, UIOX_KEY_LCTRL,    0,      0,      0,      true,  UIOX_KBD_MOD_LCTRL,  false },
    { 0xE1, UIOX_KEY_LSHIFT,   0,      0,      0,      true,  UIOX_KBD_MOD_LSHIFT, false },
    { 0xE2, UIOX_KEY_LALT,     0,      0,      0,      true,  UIOX_KBD_MOD_LALT,   false },
    { 0xE4, UIOX_KEY_RCTRL,    0,      0,      0,      true,  UIOX_KBD_MOD_RCTRL,  false },
    { 0xE5, UIOX_KEY_RSHIFT,   0,      0,      0,      true,  UIOX_KBD_MOD_RSHIFT, false },
    { 0xE6, UIOX_KEY_RALT,     0,      0,      0,      true,  UIOX_KBD_MOD_RALT,   false },
    { 0x3A, UIOX_KEY_F1,       0,0,0,  false,0,false },
    { 0x3B, UIOX_KEY_F2,       0,0,0,  false,0,false },
    { 0x3C, UIOX_KEY_F3,       0,0,0,  false,0,false },
    { 0x3D, UIOX_KEY_F4,       0,0,0,  false,0,false },
    { 0x3E, UIOX_KEY_F5,       0,0,0,  false,0,false },
    { 0x3F, UIOX_KEY_F6,       0,0,0,  false,0,false },
    { 0x40, UIOX_KEY_F7,       0,0,0,  false,0,false },
    { 0x41, UIOX_KEY_F8,       0,0,0,  false,0,false },
    { 0x42, UIOX_KEY_F9,       0,0,0,  false,0,false },
    { 0x43, UIOX_KEY_F10,      0,0,0,  false,0,false },
    { 0x44, UIOX_KEY_F11,      0,0,0,  false,0,false },
    { 0x45, UIOX_KEY_F12,      0,0,0,  false,0,false },
    { 0x4F, UIOX_KEY_RIGHT,    0,0,0,  false,0,false },
    { 0x50, UIOX_KEY_LEFT,     0,0,0,  false,0,false },
    { 0x51, UIOX_KEY_DOWN,     0,0,0,  false,0,false },
    { 0x52, UIOX_KEY_UP,       0,0,0,  false,0,false },
   };
   
   /* =========================================================================
    * Layout registry
    * ====================================================================== */
   
   static const uiox_kbd_map_t s_maps[] = {
       {
           UIOX_KBD_LAYOUT_QWERTY,
           s_qwerty_us,
           (uint16_t)(sizeof(s_qwerty_us) / sizeof(s_qwerty_us[0])),
           "QWERTY-US"
       },
       {
           UIOX_KBD_LAYOUT_QWERTZ,
           s_qwertz_de,
           (uint16_t)(sizeof(s_qwertz_de) / sizeof(s_qwertz_de[0])),
           "QWERTZ-DE"
       },
   };
   
   #define NUM_MAPS  (sizeof(s_maps) / sizeof(s_maps[0]))
   
   static const uiox_kbd_map_t *s_active = &s_maps[0];
   
   /* =========================================================================
    * Public API
    * ====================================================================== */
   
   int uiox_kbd_map_init(uiox_kbd_layout_t layout)
   {
       return uiox_kbd_map_set_layout(layout);
   }
   
   int uiox_kbd_map_set_layout(uiox_kbd_layout_t layout)
   {
       for (size_t i = 0; i < NUM_MAPS; i++) {
           if (s_maps[i].layout == layout) {
               s_active = &s_maps[i];
               return 0;
           }
       }
       return -ENOENT;
   }
   
   const uiox_kbd_map_entry_t *uiox_kbd_map_lookup(uint8_t scancode)
   {
       if (!s_active) return NULL;
       for (uint16_t i = 0; i < s_active->num_entries; i++)
           if (s_active->entries[i].scancode == scancode)
               return &s_active->entries[i];
       return NULL;
   }
   
   int uiox_kbd_map_translate(uint8_t   scancode,
                               uint8_t   modifiers,
                               bool      caps_lock,
                               uint16_t *keycode_out,
                               uint32_t *unicode_out)
   {
       const uiox_kbd_map_entry_t *e = uiox_kbd_map_lookup(scancode);
       if (!e) return -ENOENT;
   
       if (keycode_out) *keycode_out = e->keycode;
       if (!unicode_out) return 0;
   
       bool shift  = (modifiers & UIOX_KBD_MOD_SHIFT)  != 0u;
       bool altgr  = (modifiers & UIOX_KBD_MOD_RALT)   != 0u;
   
       /* CapsLock inverts shift for alpha keys */
       if (caps_lock && e->unicode_base >= 'a' && e->unicode_base <= 'z')
           shift = !shift;
   
       if (altgr && e->unicode_altgr)
           *unicode_out = e->unicode_altgr;
       else if (shift && e->unicode_shift)
           *unicode_out = e->unicode_shift;
       else
           *unicode_out = e->unicode_base;
   
       return 0;
   }
   
   const char *uiox_kbd_map_name(void)
   {
       return s_active ? s_active->name : "none";
   }
   