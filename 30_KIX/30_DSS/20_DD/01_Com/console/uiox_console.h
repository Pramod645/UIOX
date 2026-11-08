/* =============================================================================
 * @file    uiox_console.h
 * @brief   The terminal — the abstraction a program talks to instead of a UART.
 *
 * PATH    30_KIX/30_DSS/20_DD/01_Com/console/include/uiox_console.h
 *
 * ── why this is a SIBLING of 01_Com/uart/, not a file inside it ───────────
 * 20_DD/01_Com/uart/ holds ONE driver: the PL011/16550 discipline.  A console
 * is not that.  A console is the thing a program talks to, and it is a
 * different kind of object: it holds no registers, it owns no baud rate, and
 * it outlives whichever byte-stream class is underneath it.
 *
 *   today      console  ←  uart    (uiox_uart_if_t, the only discipline)
 *   later      console  ←  uart
 *                      ←  usbser  (same console, a different discipline)
 *                      ←  vterm   (no hardware at all)
 *
 * Keeping it in uart/ would say the console IS a UART, and then a second
 * terminal class inherits the first class's directory.  As a sibling, the
 * include direction is one-way: console/ sees uart/, uart/ never sees
 * console/.
 *
 * ── what this layer does NOT duplicate ────────────────────────────────────
 * 20_DD/01_Com/uart/uiox_uart_if.c already owns, and this layer must never
 * re-implement:
 *
 *   termios state          uiox_uart_if_t.tio            (live, not copied)
 *   line assembly          canon_feed()                  ICANON, ERASE, KILL
 *   VEOF / NL completion   canon_feed()                  the line terminator
 *   echo + BS SP BS        canon_feed() via tx_char()
 *   OPOST|ONLCR on output  tx_char()                     \n becomes CRLF
 *   isatty()               uiox_uart_if_ioctl() GETATTR  "this IS isatty()"
 *   window size            uiox_uart_if_t.win            GWINSZ / SWINSZ
 *   VMIN/VTIME cases       read_common()                 noncanonical mode
 *   the byte counters      uiox_uart_if_t.stats          uiox_tty_stats_t
 *
 * A second copy of any of those is a second source of truth, and the two
 * would drift the first time one is changed.  So this header declares a
 * HANDLE and a small number of operations, and nothing that restates what
 * the discipline already knows.
 *
 * ── what the console genuinely owns ───────────────────────────────────────
 *   1. the binding: which discipline this console is, held as a pointer
 *   2. the WINCH hook: the discipline counts a SWINSZ change, but only the
 *      program above knows what to do about it — a CLI redraws, a GUI
 *      relayouts, a voice front-end ignores it.  The hook pointer lives
 *      here because it belongs to the consumer, not to the driver.
 *   3. the dispatch surface: one narrow set of entry points so a caller
 *      names a console operation and not a UART class op.
 *
 * ── the class-ops rule this preserves ─────────────────────────────────────
 * uiox_uart_if.h is explicit that a CMD class's byte-stream vtable
 * (uiox_uart_class_ops_t) is PRIVATE to its class and named nowhere above
 * it, because uiox_devops.h is frozen at six families and family 5 (CMD)
 * carries one ioctl slot.  This header does not break that rule: it does
 * not name uiox_uart_class_ops_t, and it does not take uiox_dev_dev_t.
 * It takes a uiox_uart_if_t *, which its callers already hold.
 *
 * @version 1.0.0  @date 2026-11-08
 * ========================================================================== */

 #ifndef UIOX_CONSOLE_H
 #define UIOX_CONSOLE_H
 
 #include "uiox_uart_if.h"        /* uiox_uart_if_t, uiox_uart_class_ops_t */
 #include "uiox_termios.h"        /* struct uiox_termios, uiox_winsize     */
 #include "uiox_klibc.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* ── return codes ────────────────────────────────────────────────────────
  * The tree's own, not errno: -ENODEV and friends are what uiox_uart_blk.c
  * already returns, and mixing the two in one call chain is how a caller
  * ends up testing the wrong thing.
  *
  * Success is 0 or a byte count.  Failure is negative.
  */
 #define UIOX_CONSOLE_OK        0
 #define UIOX_CONSOLE_EAGAIN   -2   /* nothing arrived — read()'s own -2    */
 #define UIOX_CONSOLE_EEOF     -6   /* VEOF at an empty line                */
 #define UIOX_CONSOLE_ENODEV   -19  /* no discipline bound                  */
 #define UIOX_CONSOLE_EBUSY    -16  /* already bound to a discipline        */
 
 /* ── the object ──────────────────────────────────────────────────────────
  * Opaque.  The storage is a file-static in uiox_console.c, for the same
  * reason uiox_uart_probe.c's descriptor is: uiox_dev_bind() stores the
  * POINTER it is handed, and the console has the same lifetime as the
  * console device — the whole boot.
  */
 typedef struct uiox_console uiox_console_t;
 
 /* ═════════════════════════════════════════════════════════════════════════
  * Binding
  *
  * There is ONE console.  Not because a second is impossible, but because
  * bind() takes the discipline and the tree has one uiox_uart_if_t today;
  * when a second discipline exists, this grows an index and nothing above
  * it changes, because every caller already goes through this header.
  * ═════════════════════════════════════════════════════════════════════════ */
 
 /* Attach a discipline.  Called by the probe that owns it, AFTER
  * uiox_uart_if_start() — a console reading from a discipline whose
  * interrupt line is not attached is a console that hangs.
  *
  * Bound twice is UIOX_CONSOLE_EBUSY, not a silent rebind: two disciplines
  * behind one console would mean two termios states and no way to tell
  * which one a read came from.
  */
 int  uiox_console_bind(uiox_uart_if_t *tif);
 void uiox_console_unbind(void);
 
 /* The handle, or NULL when nothing is bound.  For a caller that needs to
  * pass it on rather than use it. */
 uiox_console_t *uiox_console_get(void);
 int             uiox_console_ready(void);   /* 1 when a discipline is bound */
 
 /* The bound discipline, or NULL.  This is the escape hatch and it is
  * deliberate: a caller that needs a class op this header does not expose
  * (uiox_uart_if_pending(), say) reaches it here rather than having every
  * such op copied upward.  Use the named operations below where one exists.
  */
 uiox_uart_if_t *uiox_console_tif(void);
 
 /* ═════════════════════════════════════════════════════════════════════════
  * The operations
  *
  * Each is a thin pass-through.  None of them implements anything: the
  * discipline does the work, and the point of the function is that a caller
  * writes uiox_console_readline() rather than uiox_uart_if_read().
  * ═════════════════════════════════════════════════════════════════════════ */
 
 /* One line, assembled by the discipline.
  *
  * ICANON assembles it, ERASE emits BS-SP-BS, KILL drops it, VEOF ends it,
  * ECHO mirrors every accepted byte, and \n completes it — all of that is
  * canon_feed()'s, in uart/.  This call returns when the line is complete,
  * or UIOX_CONSOLE_EAGAIN when nothing had arrived by then.
  *
  * Returns bytes in `buf` excluding the NUL, or a negative UIOX_CONSOLE_*.
  * A caller that must block loops on EAGAIN — there is no scheduler yet, so
  * this function does NOT block on its own.
  */
 int uiox_console_readline(void *buf, uint32_t cap);
 
 /* Bytes out.  OPOST|ONLCR is applied by tx_char() in the discipline, so a
  * caller writes "\n" and the wire gets CRLF.  That translation is the
  * reason a program stops calling early_puts() once the console is bound:
  * output becomes the program's, not the kernel's. */
 int uiox_console_write(const void *buf, uint32_t count);
 int uiox_console_puts(const char *s);
 int uiox_console_putc(char ch);
 
 /* Is there a byte waiting, without consuming one?  Wraps the class's
  * pending op — the one the boot-context caller would otherwise reach
  * through uiox_console_tif(). */
 int uiox_console_pending(void);
 
 /* tty control.  `cmd` is a UIOX_TTY_IOC_* value from uiox_uart_ioctl.h,
  * passed through unchanged: the console is not a second numbering of the
  * same commands.  GETATTR is the isatty() test — if it answers, the
  * descriptor is a tty. */
 int uiox_console_ioctl(unsigned long cmd, void *arg);
 
 /* ── the WINCH hook — this layer's one piece of real state ───────────────
  * The discipline counts a SWINSZ change (stats.swinsz_changes) but has no
  * opinion about it.  A front-end does.  The pointer lives here so the
  * driver stays ignorant of its consumers.
  *
  * A front-end that ignores resizes never calls this, and the console then
  * has a NULL hook — which is a valid state, not a missing one.
  */
 typedef void (*uiox_console_winch_fn_t)(void *priv);
 void uiox_console_set_winch_hook(uiox_console_winch_fn_t fn, void *priv);
 void uiox_console_winch(void);   /* the discipline would call this */
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* UIOX_CONSOLE_H */
 