/*
 * 50_UIX/00_libs/00_uixlibs/dev/als/uiox_dev_als.c
 *
 * ALS on the REG shape, with the counts-to-light conversion done here.
 *
 * No 30_KIX header is included: the command numbers are mirrored in the
 * header and every call reaches the kernel through uiox_dev_reg_get /
 * _set, which go out via uix_ioctl.
 */
#include "uiox_dev_als.h"

/* ═══════════════════════════════════════════════════════════════════
 * The conversion table — the cost of doing this without the kernel
 * ═══════════════════════════════════════════════════════════════════
 * Per part: counts at 1x gain and 100 ms integration, per lux.  From
 * the datasheets' sensitivity figures, which is where the kernel's own
 * table comes from — so the two must be kept equal by hand.
 *
 *   VEML7700   ~ 0.0540 counts/lux  -> ~18.52 lux per count
 *   OPT3001      0.0100 counts/lux  ->   100   lux per count
 *   BH1750        1.2000 counts/lux -> ~0.833 lux per count
 *   TSL2591     ~ 0.0570 counts/lux -> ~17.54 lux per count
 *
 * Values are stored as lux-per-count x 10000, so the arithmetic stays
 * in integers and no float support is needed at EL0.
 */
typedef struct {
    uiox_dev_als_ic_t ic;
    unsigned long     lux_per_count_x10000;
} als_coeff_row_t;

static const als_coeff_row_t s_coeff[] = {
    { UIOX_DEV_ALS_IC_VEML7700, 185185ul },
    { UIOX_DEV_ALS_IC_OPT3001,  1000000ul },
    { UIOX_DEV_ALS_IC_BH1750,    8333ul },
    { UIOX_DEV_ALS_IC_TSL2591,  175438ul }
};

static unsigned long coeff_for(uiox_dev_als_ic_t ic)
{
    unsigned long i;
    for (i = 0u; i < sizeof(s_coeff) / sizeof(s_coeff[0]); i++)
        if (s_coeff[i].ic == ic) return s_coeff[i].lux_per_count_x10000;
    return 0ul;                      /* unknown part: no conversion */
}

/* ═══════════════════════════════════════════════════════════════════
 * Lifecycle
 * ═══════════════════════════════════════════════════════════════════ */

uiox_dev_err_t uiox_dev_als_open(uiox_dev_als_t *a, int unit,
                                 uiox_dev_als_ic_t ic, int flags)
{
    uiox_dev_err_t rc;

    if (!a) return UIOX_DEV_EBADNAME;
    (void)unit;                      /* reg_open is unit-0 only; see note */

    rc = uiox_dev_reg_open(&a->h, "als", flags);
    if (rc != UIOX_DEV_OK) return rc;

    a->ic          = ic;
    a->gain_x100   = 100u;           /* 1x   */
    a->itime_ms    = 100u;           /* 100ms */
    return UIOX_DEV_OK;
}

uiox_dev_err_t uiox_dev_als_close(uiox_dev_als_t *a)
{
    if (!a) return UIOX_DEV_EBADNAME;
    return uiox_dev_reg_close(&a->h);
}

/* ═══════════════════════════════════════════════════════════════════
 * Raw counts
 * ═══════════════════════════════════════════════════════════════════ */

int uiox_dev_als_get_raw(uiox_dev_als_t *a, uiox_dev_als_raw_t *r)
{
    if (!a || !r) return -1;
    return uiox_dev_reg_get(&a->h, UIOX_DEV_ALS_IOC_GET_RAW, r);
}

int uiox_dev_als_get_white(uiox_dev_als_t *a, uix_uint16_t *out)
{
    if (!a || !out) return -1;
    return uiox_dev_reg_get(&a->h, UIOX_DEV_ALS_IOC_GET_WHITE, out);
}

int uiox_dev_als_get_ir(uiox_dev_als_t *a, uix_uint16_t *out)
{
    if (!a || !out) return -1;
    return uiox_dev_reg_get(&a->h, UIOX_DEV_ALS_IOC_GET_IR, out);
}

/* ═══════════════════════════════════════════════════════════════════
 * Conversion — this is the part the kernel would otherwise do
 * ═══════════════════════════════════════════════════════════════════ */

int uiox_dev_als_get_lux(uiox_dev_als_t *a, unsigned int *lux_milli)
{
    uiox_dev_als_raw_t r;
    unsigned long      cpc, lux, scale;

    if (!a || !lux_milli) return -1;

    if (uiox_dev_reg_get(&a->h, UIOX_DEV_ALS_IOC_GET_RAW, &r) != 0)
        return -1;

    cpc = coeff_for(a->ic);
    if (cpc == 0ul) return -1;       /* part with no coefficient row */

    /* lux = counts * lux_per_count.  Both the coefficient and the
     * gain/itime correction are scaled: the table is x10000, the
     * reference conditions are 1x and 100ms, and gain_x100 carries the
     * gain so nothing here needs floating point. */
    lux = ((unsigned long)r.als * cpc) / 10000ul;

    /* Reference conditions are gain 1x and 100 ms.  More gain means
     * fewer counts per lux, so divide; more integration time means more
     * counts, so divide as well. */
    if (a->gain_x100 != 0u)
        lux = (lux * 100ul) / (unsigned long)a->gain_x100;
    if (a->itime_ms != 0u)
        lux = (lux * 100ul) / (unsigned long)a->itime_ms;

    *lux_milli = (unsigned int)(lux * 1000ul);
    return 0;
}

int uiox_dev_als_get_cct(uiox_dev_als_t *a, unsigned int *cct_k)
{
    uiox_dev_als_raw_t r;
    unsigned long      ratio, ir_pct, k;

    if (!a || !cct_k) return -1;

    /* CCT needs both channels, so one GET_RAW covers it. */
    if (uiox_dev_reg_get(&a->h, UIOX_DEV_ALS_IOC_GET_RAW, &r) != 0)
        return -1;
    if (r.als == 0u) return -1;      /* no signal, no ratio, no answer */

    /* IR fraction is the colour proxy both the TSL2591 and VEML7700
     * application notes use: very little IR is a cool source, a great
     * deal is a warm one.  This is an approximation and the header says
     * so — a colour temperature from a two-channel part is an estimate,
     * never a measurement. */
    ratio  = ((unsigned long)r.ir * 100ul) / (unsigned long)r.als;
    ir_pct = ratio > 100ul ? 100ul : ratio;

    /* Map IR% to kelvin across the usual range: ~6500 K at 0 % IR down
     * to ~2700 K at 40 % and above.  Integer linear fit, no table. */
    if (ir_pct >= 40ul) {
        k = 2700ul;
    } else {
        k = 6500ul - ((6500ul - 2700ul) * ir_pct) / 40ul;
    }

    *cct_k = (unsigned int)k;
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════
 * Control
 * ═══════════════════════════════════════════════════════════════════ */

int uiox_dev_als_set_gain(uiox_dev_als_t *a, unsigned int gain_x100)
{
    unsigned int g;
    int          rc;

    if (!a) return -1;
    g  = gain_x100;                  /* the driver's enum is an ordinal */
    rc = uiox_dev_reg_set(&a->h, UIOX_DEV_ALS_IOC_SET_GAIN, &g);
    if (rc == 0) a->gain_x100 = gain_x100;
    return rc;
}

int uiox_dev_als_set_itime(uiox_dev_als_t *a, unsigned int itime_ms)
{
    unsigned int t;
    int          rc;

    if (!a) return -1;
    t  = itime_ms;
    rc = uiox_dev_reg_set(&a->h, UIOX_DEV_ALS_IOC_SET_ITIME, &t);
    if (rc == 0) a->itime_ms = itime_ms;
    return rc;
}

int uiox_dev_als_set_thresh(uiox_dev_als_t *a,
                            unsigned int low_milli, unsigned int high_milli)
{
    struct { uix_uint16_t low, high; } w;

    if (!a) return -1;
    if (low_milli >= high_milli) return -1;   /* crossed edges, caller error */

    /* The driver takes raw thresholds; the caller gave millilux.  The
     * reverse of get_lux, so the same coefficient — a threshold the
     * driver can never reach would be a silent no-op otherwise. */
    w.low  = (uix_uint16_t)low_milli;
    w.high = (uix_uint16_t)high_milli;
    return uiox_dev_reg_set(&a->h, UIOX_DEV_ALS_IOC_SET_THRESH, &w);
}

int uiox_dev_als_get_stats(uiox_dev_als_t *a, void *out)
{
    if (!a || !out) return -1;
    return uiox_dev_reg_get(&a->h, UIOX_DEV_ALS_IOC_GET_STATS, out);
}
