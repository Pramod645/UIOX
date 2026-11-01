/*
 * 02_FwHal/src/uiox_fw_printf.c
 *
 * The ONE definition of uiox_fw_printf.
 *
 * ── why this file exists ────────────────────────────────────────────
 * It used to live in uiox_fw.h as a static function.  A static function
 * in a header gives every translation unit that includes the header its
 * own private copy, with a LOCAL symbol -- `nm` showed six of them in
 * libuiox_fw_*.a, all lowercase `t`.
 *
 * A local symbol cannot satisfy a cross-object reference.  So when
 * 32_FS/02_jrnal/src/uiox_kix_jrnl_tx.c called it (lines 146 and 197),
 * the linker looked for a GLOBAL uiox_fw_printf, found only locals, and
 * reported it undefined -- on all four architectures.
 *
 * Moving the body here makes it one global definition in one object.
 *
 * ── what stays in the header ────────────────────────────────────────
 * uiox_fw.h keeps only the declaration:
 *
 *     void uiox_fw_printf(const char *fmt, ...);
 *
 * _fw_puts and _fw_putu stay static inline there; this file uses them
 * through the include, and nothing else calls them.
 *
 * ── the format set ──────────────────────────────────────────────────
 * %s %c %d %u %x %X %p %% -- plus a width skip ("%-3u" parses, the
 * width is ignored).  An unknown conversion prints its own letter
 * rather than failing, so a bad format cannot silence the log.
 *
 * __builtin_va_* rather than <stdarg.h>, matching the original: the
 * build is -nostdinc, so the builtins are what is available.
 *
 * @version 1.0.0  @date 2026-10-31
 */
#include "uiox_fw.h"

void uiox_fw_printf(const char *fmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            uiox_fw_putchar(*fmt);
            continue;
        }

        fmt++;

        /* width/flags are parsed and ignored -- "%-3u" must not print
         * the "3" as a conversion letter */
        if (*fmt == '-') fmt++;
        while (*fmt >= '0' && *fmt <= '9') fmt++;

        switch (*fmt) {
        case 's':
            _fw_puts(__builtin_va_arg(ap, const char *));
            break;

        case 'c':
            uiox_fw_putchar((char)__builtin_va_arg(ap, int));
            break;

        case 'd': {
            int v = __builtin_va_arg(ap, int);
            if (v < 0) {
                uiox_fw_putchar('-');
                v = -v;
            }
            _fw_putu((uint32_t)v, 10, 0);
            break;
        }

        case 'u':
            _fw_putu(__builtin_va_arg(ap, uint32_t), 10, 0);
            break;

        case 'x':
            _fw_putu(__builtin_va_arg(ap, uint32_t), 16, 0);
            break;

        case 'X':
            _fw_putu(__builtin_va_arg(ap, uint32_t), 16, 1);
            break;

        case 'p':
            _fw_puts("0x");
            _fw_putu((uint32_t)(uintptr_t)__builtin_va_arg(ap, void *), 16, 0);
            break;

        case '%':
            uiox_fw_putchar('%');
            break;

        default:
            uiox_fw_putchar('%');
            uiox_fw_putchar(*fmt);
            break;
        }
    }

    __builtin_va_end(ap);
}
