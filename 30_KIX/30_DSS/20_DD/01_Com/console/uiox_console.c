/* =============================================================================
 * @file    uiox_console.c
 * @brief   The terminal — a binding and three pass-throughs, and nothing else.
 *
 * PATH    30_KIX/30_DSS/20_DD/01_Com/console/src/uiox_console.c
 *
 * ── the whole file, in one sentence ───────────────────────────────────────
 * It holds a uiox_uart_if_t * and forwards.  Every piece of behaviour a
 * terminal is expected to have — line assembly, echo, ERASE, KILL, VEOF,
 * OPOST|ONLCR, the VMIN/VTIME cases, the counter set — already lives in
 * 20_DD/01_Com/uart/ and is NOT restated here.
 *
 * Read this file expecting to be disappointed by how little it does.  That
 * is the design: a console that knew how to assemble a line would be a
 * second line discipline, and the first time the two disagreed, neither
 * would be obviously wrong.
 *
 * ── what was deliberately deleted from an earlier draft ───────────────────
 * An earlier version of this file carried, and all of it was duplicate:
 *
 *     struct uiox_termios t;      → uiox_uart_if_t.tio is live and owned
 *                                    by the discipline; a copy would drift
 *     char lbuf[]; size_t llen;   → canon_feed() assembles into the
 *                                    CALLER's buffer; line_pos is the cursor
 *     int  cr_pending;            → ICRNL is the discipline's business
 *     struct uiox_winsize ws;     → uiox_uart_if_t.win, served by GWINSZ
 *     uiox_console_stats_t stats; → uiox_uart_if_t.stats, uiox_tty_stats_t
 *     ldisc_feed()                → canon_feed()
 *     ldisc_reset()               → canon_feed()'s state, on FLUSH
 *     ldisc_take_line()           → canon_feed() returns the line itself
 *     uiox_console_tx_byte()      → tx_char(), which also does OPOST|ONLCR
 *     isatty() implementation     → GETATTR, which the ioctl says IS isatty
 *
 * Each of those was a second place for one fact.  None is coming back.
 *
 * ── threading, stated honestly ────────────────────────────────────────────
 * There is no lock, and there is no lock for the same reason
 * uiox_devtable.c has none: bind() runs during probe, before anything can
 * read from the console, and nothing unbinds at runtime.  If a second
 * discipline ever makes bind() runtime-callable, this comment is the one
 * that will be wrong first.
 *
 * @version 1.0.0  @date 2026-11-08
 * ========================================================================== */

 #include "uiox_console.h"

 /* ═════════════════════════════════════════════════════════════════════════
  * The one console
  *
  * File-static, not heap and not stack.  Same reasoning as
  * uiox_uart_probe.c's s_uart_dev: the console's lifetime is the boot, and
  * a static gives it a stable address with no allocator involved — which
  * matters because this is reachable from the probe sequence, where no
  * allocator is guaranteed up yet.
  * ═════════════════════════════════════════════════════════════════════════ */
 
 struct uiox_console {
     uiox_uart_if_t          *tif;        /* the discipline. NOT a copy.    */
     uiox_console_winch_fn_t  winch_fn;   /* NULL is a valid state          */
     void                    *winch_priv;
 };
 
 static struct uiox_console s_console;
 
 /* ═════════════════════════════════════════════════════════════════════════
  * Binding
  * ═════════════════════════════════════════════════════════════════════════ */
 
 int uiox_console_bind(uiox_uart_if_t *tif)
 {
     /* A NULL discipline is not a console.  The tree's convention is to
      * refuse rather than to accept and no-op later — uiox_dev_bind()
      * refuses a device with no class for exactly this reason. */
     if (!tif) return UIOX_CONSOLE_ENODEV;
     if (s_console.tif) return UIOX_CONSOLE_EBUSY;
 
     /* `primed` is set by uiox_uart_if_config() and cleared by nothing, so
      * it is the honest test of "this discipline is usable".  Binding a
      * discipline that was never configured would give a console whose
      * reads trip read_common()'s own guard and return -1 forever — a
      * failure that would surface far from this call. */
     if (!tif->primed) return UIOX_CONSOLE_ENODEV;
 
     s_console.tif        = tif;
     s_console.winch_fn   = (uiox_console_winch_fn_t)0;
     s_console.winch_priv = (void *)0;
     return UIOX_CONSOLE_OK;
 }
 
 void uiox_console_unbind(void)
 {
     s_console.tif        = (uiox_uart_if_t *)0;
     s_console.winch_fn   = (uiox_console_winch_fn_t)0;
     s_console.winch_priv = (void *)0;
 }
 
 uiox_console_t *uiox_console_get(void)
 {
     return s_console.tif ? &s_console : (uiox_console_t *)0;
 }
 
 int uiox_console_ready(void)
 {
     return s_console.tif ? 1 : 0;
 }
 
 uiox_uart_if_t *uiox_console_tif(void)
 {
     /* The escape hatch, and it is here on purpose.  A caller needing
      * uiox_uart_if_pending() — or any future class op this header has not
      * grown a wrapper for — reaches it through this rather than having
      * every op copied upward one at a time.  A wrapper earns its place
      * when the NAME differs; not when it is a straight pass-through. */
     return s_console.tif;
 }
 
 /* ═════════════════════════════════════════════════════════════════════════
  * Input
  * ═════════════════════════════════════════════════════════════════════════ */
 
 int uiox_console_readline(void *buf, uint32_t cap)
 {
     if (!s_console.tif) return UIOX_CONSOLE_ENODEV;
     if (!buf || cap == 0u) return UIOX_CONSOLE_ENODEV;
 
     /* Straight pass-through, including the -2.
      *
      * uiox_uart_if_read()'s read_common() already:
      *   - pops the ring (or polls, per UIOX_UART_RX_MODE),
      *   - runs canon_feed() when ICANON is set, which assembles the line,
      *     runs ERASE / KILL / VEOF, and echoes every accepted byte,
      *   - applies the VMIN/VTIME rule when ICANON is clear,
      *   - returns the byte count on a complete line, or -2 for EAGAIN.
      *
      * So this function adds no behaviour — it adds a NAME.  A caller
      * writes uiox_console_readline() and does not have to know that the
      * line discipline is a UART class's private business.
      *
      * It does NOT block.  Option A says read() blocks; there is no
      * scheduler to block ON yet, so the loop belongs to the caller and the
      * EAGAIN is passed through rather than hidden behind a spin.  A
      * blocking version becomes correct — and this comment gets rewritten —
      * when 33_PCS can put a process to sleep.
      */
     return uiox_uart_if_read(s_console.tif, (uint8_t *)buf, cap);
 }
 
 int uiox_console_pending(void)
 {
     if (!s_console.tif) return 0;
     return uiox_uart_if_pending(s_console.tif) ? 1 : 0;
 }
 
 /* ═════════════════════════════════════════════════════════════════════════
  * Output
  *
  * OPOST|ONLCR is applied inside the discipline's tx_char(), which is the
  * one place it should be: a caller writes '\n' and the wire receives CRLF,
  * with no front-end having to remember the pair.
  * ═════════════════════════════════════════════════════════════════════════ */
 
 int uiox_console_write(const void *buf, uint32_t count)
 {
     if (!s_console.tif) return UIOX_CONSOLE_ENODEV;
     if (!buf || count == 0u) return UIOX_CONSOLE_OK;
     return uiox_uart_if_write(s_console.tif, (const uint8_t *)buf, count);
 }
 
 int uiox_console_puts(const char *s)
 {
     uint32_t n = 0u;
     if (!s) return UIOX_CONSOLE_ENODEV;
 
     /* The length walk is the one thing this file does itself, and only
      * because uiox_uart_if_write() takes a count.  Same shape as
      * uiox_sh.c's own sh_puts(). */
     while (s[n]) n++;
     if (n == 0u) return UIOX_CONSOLE_OK;
     return uiox_console_write(s, n);
 }
 
 int uiox_console_putc(char ch)
 {
     return uiox_console_write(&ch, 1u);
 }
 
 /* ═════════════════════════════════════════════════════════════════════════
  * tty control
  * ═════════════════════════════════════════════════════════════════════════ */
 
 int uiox_console_ioctl(unsigned long cmd, void *arg)
 {
     if (!s_console.tif) return UIOX_CONSOLE_ENODEV;
 
     /* Passed through unchanged.  The console is NOT a second numbering of
      * the same commands: UIOX_TTY_IOC_GETATTR means one thing in this
      * tree, and it is defined in uiox_uart_ioctl.h under 10_DI.  A caller
      * includes that header for the constant and this one for the call.
      *
      * GETATTR is also the isatty() test — it answers when the descriptor
      * behind it is a tty, which is a capability check rather than a
      * file-type test.  uiox_uart_if.c's own comment says as much at the
      * case: "this IS isatty()".  So there is no isatty() here to write. */
     return uiox_uart_if_ioctl(s_console.tif, cmd, arg);
 }
 
 /* ═════════════════════════════════════════════════════════════════════════
  * The WINCH hook — the one piece of state this layer owns outright
  *
  * The discipline counts a window-size change and stops there: SWINSZ
  * increments stats.swinsz_changes.  What a resize MEANS is a consumer's
  * question — a CLI redraws its prompt, a GUI relays out, a voice
  * front-end ignores it entirely.  So the callback pointer lives here, in
  * the layer that sits between them, and the driver never learns that
  * consumers exist.
  * ═════════════════════════════════════════════════════════════════════════ */
 
 void uiox_console_set_winch_hook(uiox_console_winch_fn_t fn, void *priv)
 {
     s_console.winch_fn   = fn;
     s_console.winch_priv = priv;
 }
 
 void uiox_console_winch(void)
 {
     /* NULL is a valid state, not a missing one: a front-end that ignores
      * resizes never registers, and the console must not treat that as an
      * error.  Nothing to do is not a failure. */
     if (s_console.winch_fn) s_console.winch_fn(s_console.winch_priv);
 }
 
 /* ═════════════════════════════════════════════════════════════════════════
  * NOT HERE, and where it would go instead
  *
  *   ICRNL / INLCR / IGNCR    canon_feed() and read_common(), in uart/
  *   ISIG → a signal          uiox_tty_signal_from_char(), weak, owned by
  *                            33_PCS.  Still a stub today: it counts the
  *                            interrupt and delivers nothing, which is why
  *                            ^C at a prompt does nothing yet.  Wiring it
  *                            is a 33_PCS job, not a console one.
  *   flush of the RX ring     currently tif->hw->rx.tail = tif->hw->rx.head
  *                            INSIDE uiox_uart_if_ioctl() — a 20_DD file
  *                            writing 10_DI's struct internals.  Should
  *                            become uiox_uart_hw_rx_flush(hw) in 10_DI,
  *                            beside rx_pop and rx_pending.
  *   baud                     forwarded by the discipline to
  *                            uiox_uart_hw_set_baud(); the console never
  *                            caches it, because it does not own the clock.
  * ═════════════════════════════════════════════════════════════════════════ */
 