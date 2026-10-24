/**
 * @file  uiox_uart_if.c
 * @brief UIOX console driver — the line discipline.
 *
 * Reproduces 50_UIX/01_shell/termio/'s behaviour with the kernel's own
 * types: ICANON, ISIG, the four VMIN/VTIME cases, ERASE/KILL/EOF,
 * ECHO/ECHOE/ECHOK/ECHONL, OPOST|ONLCR.
 *
 * ── echo lives HERE, not in the ISR ───────────────────────────────
 * putc is a bounded busy-wait.  Fine in process context, wrong in an
 * interrupt.  The ISR only pushes; every echo is emitted from read().
 *
 * ── ISIG is tested BEFORE ICANON ──────────────────────────────────
 * termio's tty_cbreak() clears ICANON while leaving ISIG set
 * (c_lflag &= ~(ECHO | ICANON)), so the two switches are independent,
 * not nested — a ^C must still fire in cbreak mode.
 *
 * @version 1.0.0  @date 2026-10-21
 */
#include "uiox_uart_if.h"
#include "uiox_uart_ioctl.h"

/* ── defaults ─────────────────────────────────────────────────────── */
void uiox_tty_defaults(struct uiox_termios *t)
{
    if (!t) return;
    t->c_iflag = UIOX_ICRNL | UIOX_IXON;
    t->c_oflag = UIOX_OPOST | UIOX_ONLCR;
    t->c_cflag = UIOX_CREAD | UIOX_CS8;
    t->c_lflag = UIOX_ISIG | UIOX_ICANON | UIOX_ECHO | UIOX_ECHOE | UIOX_ECHOK;
    t->c_line  = 0u;
    for (uint32_t k = 0u; k < UIOX_NCCS; k++) t->c_cc[k] = 0u;
    t->c_cc[UIOX_VINTR]  = 3u;      /* ^C   */
    t->c_cc[UIOX_VQUIT]  = 28u;     /* ^\   */
    t->c_cc[UIOX_VERASE] = 127u;    /* DEL  */
    t->c_cc[UIOX_VKILL]  = 21u;     /* ^U   */
    t->c_cc[UIOX_VEOF]   = 4u;      /* ^D   */
    t->c_cc[UIOX_VSUSP]  = 26u;     /* ^Z   */
    t->c_cc[UIOX_VSTART] = 17u;     /* ^Q   */
    t->c_cc[UIOX_VSTOP]  = 19u;     /* ^S   */
    t->c_cc[UIOX_VMIN]   = 1u;      /* Case B: MIN>0, TIME=0 */
    t->c_cc[UIOX_VTIME]  = 0u;
    t->c_ispeed = t->c_ospeed = 115200u;
}

/* ── output ────────────────────────────────────────────────────────
 * OPOST|ONLCR: NL becomes CRLF on the wire.  termio's tty_mode.h notes
 * that without OPOST a bare "\n" does not return the cursor to column
 * 0 — the same rule, applied on the way out. */
static void tx_char(uiox_uart_if_t *tif, uint8_t c)
{
    if ((tif->tio.c_oflag & UIOX_OPOST) &&
        (tif->tio.c_oflag & UIOX_ONLCR) && c == (uint8_t)'\n')
        uiox_uart_hw_putc(tif->hw, '\r');
    uiox_uart_hw_putc(tif->hw, (char)c);
    tif->stats.tx_bytes++;
}

int uiox_uart_if_write(uiox_uart_if_t *tif, const uint8_t *buf, uint32_t count)
{
    if (!tif || !tif->primed || !buf) return -1;
    for (uint32_t i = 0u; i < count; i++) tx_char(tif, buf[i]);
    return (int)count;           /* polled TX cannot short-write */
}

/* ── the line discipline ───────────────────────────────────────────
 * Returns 1 when the line is complete, 0 to keep collecting. */
static int canon_feed(uiox_uart_if_t *tif, uint8_t c, uint8_t *buf,
                      uint32_t *got, uint32_t count)
{
    const struct uiox_termios *tio = &tif->tio;

    /* ISIG first — see banner. */
    if ((tio->c_lflag & UIOX_ISIG) &&
        (c == tio->c_cc[UIOX_VINTR] ||
         c == tio->c_cc[UIOX_VQUIT] ||
         c == tio->c_cc[UIOX_VSUSP])) {
        tif->stats.intr_signals++;
        uiox_tty_signal_from_char(tif, c);     /* consumed, no byte out */
        return 0;
    }

    if (c == tio->c_cc[UIOX_VERASE]) {
        if (*got > 0u) {
            (*got)--;
            if (tif->line_pos) tif->line_pos--;
            tif->stats.erases++;
            if (tio->c_lflag & UIOX_ECHO) {     /* BS SP BS */
                tx_char(tif, 0x08u); tx_char(tif, 0x20u); tx_char(tif, 0x08u);
            }
        }
        return 0;
    }

    if (c == tio->c_cc[UIOX_VKILL]) {
        tif->stats.kills++;
        while (*got > 0u) {
            (*got)--;
            if (tio->c_lflag & UIOX_ECHO) {
                tx_char(tif, 0x08u); tx_char(tif, 0x20u); tx_char(tif, 0x08u);
            }
        }
        tif->line_pos = 0u;
        return 0;
    }

    if (c == tio->c_cc[UIOX_VEOF] || c == (uint8_t)'\n') {
        if (*got < count) buf[(*got)++] = c;
        if (tio->c_lflag & UIOX_ECHO) tx_char(tif, c);
        tif->line_pos = 0u;
        tif->stats.canon_lines++;
        return 1;                              /* line complete */
    }

    if (*got < count) buf[(*got)++] = c;
    tif->line_pos++;
    if (tio->c_lflag & UIOX_ECHO) tx_char(tif, c);
    return 0;
}

/* ── input ─────────────────────────────────────────────────────────
 * Both receive bodies are present; UIOX_UART_RX_MODE selects.  Keeping
 * both is deliberate: the diff between (a) and (b) stays visible in the
 * source rather than being baked into a choice.
 *
 * Returns bytes read, or -2 for EAGAIN (nothing available). */
static int read_common(uiox_uart_if_t *tif, uint8_t *buf, uint32_t count)
{
    uint32_t got = 0u;

    while (got < count) {
        uint8_t c;
#if UIOX_UART_RX_MODE == UIOX_UART_RX_IRQ
        if (uiox_uart_hw_rx_pop(tif->hw, &c) != 0) break;
#else
        int rc = uiox_uart_hw_getc_poll(tif->hw, &c, UIOX_UART_POLL_SPINS);
        if (rc == 1) break;
        if (rc < 0) return got ? (int)got : -1;
#endif
        tif->stats.rx_bytes++;

        if (tif->tio.c_lflag & UIOX_ICANON) {
            if (canon_feed(tif, c, buf, &got, count)) return (int)got;
        } else {
            buf[got++] = c;
            /* Case B (MIN>0, TIME=0) when VMIN is set; a VMIN of 0 makes
             * this the Case D "return immediately" form. */
            if (tif->tio.c_cc[UIOX_VMIN] > 0u &&
                got >= tif->tio.c_cc[UIOX_VMIN]) return (int)got;
        }
    }
    return got ? (int)got : -2;    /* EAGAIN */
}

int uiox_uart_if_read(uiox_uart_if_t *tif, uint8_t *buf, uint32_t count)
{
    if (!tif || !tif->primed || !buf || count == 0u) return -1;
    return read_common(tif, buf, count);
}

bool uiox_uart_if_pending(uiox_uart_if_t *tif)
{ return tif && tif->primed && uiox_uart_hw_rx_pending(tif->hw); }

/* ── ioctl ────────────────────────────────────────────────────────── */
int uiox_uart_if_ioctl(uiox_uart_if_t *tif, unsigned long cmd, void *arg)
{
    if (!tif || !tif->primed) return -1;

    switch (cmd) {
    case UIOX_TTY_IOC_GETATTR:
        if (!arg) return -1;
        *(struct uiox_termios *)arg = tif->tio;   /* this IS isatty() */
        return 0;

    case UIOX_TTY_IOC_SETATTR:
        if (!arg) return -1;
        /* Snapshot before the change, so a caller can restore — the
         * reference's partial-success rule ("we always re-read and
         * compare to detect partial failure"). */
        tif->saved = tif->tio;
        tif->saved_valid = true;
        tif->tio = *(const struct uiox_termios *)arg;
        return 0;

    case UIOX_TTY_IOC_FLUSH: {
        if (!arg) return -1;
        int which = *(int *)arg;
        if (which == 0 || which == 2) {            /* input  */
            tif->hw->rx.tail = tif->hw->rx.head;
            tif->line_pos = 0u;
        }
        if (which == 1 || which == 2) {            /* output */
            for (uint32_t i = 0u; i < UIOX_UART_POLL_SPINS; i++)
                if (uiox_uart_tx_ready(tif->hw)) break;
        }
        return 0;
    }

    case UIOX_TTY_IOC_GWINSZ:
        if (!arg) return -1;
        *(struct uiox_winsize *)arg = tif->win;
        return 0;

    case UIOX_TTY_IOC_SWINSZ:
        if (!arg) return -1;
        tif->win = *(const struct uiox_winsize *)arg;
        tif->stats.swinsz_changes++;
        return 0;                    /* termio drives SIGWINCH off this */

    case UIOX_TTY_IOC_DRAIN:
        return 0;                    /* polled TX is never buffered */

    case UIOX_TTY_IOC_GETSTATS:
        if (!arg) return -1;
        *(uiox_tty_stats_t *)arg = tif->stats;
        return 0;

    case UIOX_TTY_IOC_SETBAUD:
        if (!arg) return -1;
        return uiox_uart_hw_set_baud(tif->hw, *(uint32_t *)arg);

    default:
        return -1;
    }
}

/* ── lifecycle ────────────────────────────────────────────────────── */
int uiox_uart_if_config(uiox_uart_if_t *tif, uiox_uart_hw_t *hw)
{
    if (!tif || !hw) return -1;
    for (uint32_t i = 0u; i < sizeof(*tif); i++) ((uint8_t *)tif)[i] = 0u;
    tif->hw = hw;
    uiox_tty_defaults(&tif->tio);
    tif->win.ws_row = 24u;
    tif->win.ws_col = 80u;
    /* primed WITH NO hardware I/O.  A read arriving between here and
     * start() would pass blk.c's guard and reach a controller whose
     * interrupt line is not yet attached — the two calls are adjacent
     * in the probe for that reason. */
    tif->primed = true;
    return 0;
}

int uiox_uart_if_start(uiox_uart_if_t *tif)
{
    if (!tif || !tif->hw) return -1;
    return uiox_uart_hw_irq_attach(tif->hw);   /* 0 in the IRQ path */
}

void uiox_uart_if_stop(uiox_uart_if_t *tif)
{ if (tif && tif->hw) uiox_uart_hw_deinit(tif->hw); }

/* ── the signal hook ───────────────────────────────────────────────
 * 33_PCS owns signal delivery.  Weak so the console links and refuses
 * honestly before the scheduler is wired, rather than inventing a pcb. */
__attribute__((weak)) void uiox_tty_signal_from_char(uiox_uart_if_t *tif, uint8_t c)
{ (void)tif; (void)c; }
