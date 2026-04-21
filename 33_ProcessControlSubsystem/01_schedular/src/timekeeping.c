#include "timekeeping.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ─────────────────────────────────────────────────────────────
 * Module-private state
 * ───────────────────────────────────────────────────────────── */
static uint64_t   tsc_frequency    = 1000000000ULL; /* 1 GHz default    */
static uint64_t   tsc_at_boot      = 0;
static uint64_t   monotonic_base   = 0;             /* ns since init     */
static uint64_t   loops_per_usec   = 1000;          /* calibrated delay  */
static TimerOpts *active_timer     = NULL;

/* ─────────────────────────────────────────────────────────────
 * Timer source implementations (simulated)
 * ───────────────────────────────────────────────────────────── */
static void     pit_mark_offset(void)   { /* PIT: record offset   */ }
static uint64_t pit_get_offset(void)    { return TICK_NSEC / 2;   }
static uint64_t pit_monotonic(void)     { return jiffies * TICK_NSEC; }
static void     pit_delay(unsigned long loops)
{
    volatile unsigned long i = loops;
    while (i--);
}

static void     tsc_mark_offset(void)   { /* TSC: rdtsc snapshot  */ }
static uint64_t tsc_get_offset(void)    { return 0; /* simplified */ }
static uint64_t tsc_monotonic(void)     { return jiffies * TICK_NSEC; }
static void     tsc_delay(unsigned long loops) { pit_delay(loops); }

static void     hpet_mark_offset(void)  { }
static uint64_t hpet_get_offset(void)   { return 0; }
static uint64_t hpet_monotonic(void)    { return jiffies * TICK_NSEC; }
static void     hpet_delay(unsigned long l) { pit_delay(l); }

/* ─────────────────────────────────────────────────────────────
 * Timer source table (Table 6-2 in source)
 * Order: HPET > ACPI PMT > TSC > PIT > none
 * ───────────────────────────────────────────────────────────── */
static TimerOpts timer_table[] = {
    {
        "timer_hpet", TIMER_SRC_HPET,
        hpet_mark_offset, hpet_get_offset, hpet_monotonic, hpet_delay
    },
    {
        "timer_tsc",  TIMER_SRC_TSC,
        tsc_mark_offset,  tsc_get_offset,  tsc_monotonic,  tsc_delay
    },
    {
        "timer_pit",  TIMER_SRC_PIT,
        pit_mark_offset,  pit_get_offset,  pit_monotonic,  pit_delay
    },
    {
        "timer_none", TIMER_SRC_NONE,
        NULL, NULL, NULL, pit_delay
    }
};
#define TIMER_TABLE_COUNT 4

/* ─────────────────────────────────────────────────────────────
 * select_timer
 * In a real kernel, probes hardware; here we pick PIT by default.
 * ───────────────────────────────────────────────────────────── */
TimerOpts *select_timer(void)
{
    /* In simulation: prefer PIT (always available) */
    active_timer = &timer_table[2]; /* timer_pit */
    printf("[timekeeping] selected timer source: %s\n", active_timer->name);
    return active_timer;
}

/* ─────────────────────────────────────────────────────────────
 * get_cmos_time  — reads wall-clock from simulated RTC
 * ───────────────────────────────────────────────────────────── */
int64_t get_cmos_time(void)
{
    /* In a real kernel: read I/O ports 0x70/0x71 (CMOS) */
    time_t t = time(NULL);
    printf("[timekeeping] CMOS time: %ld\n", (long)t);
    return (int64_t)t;
}

/* ─────────────────────────────────────────────────────────────
 * calibrate_tsc
 * ───────────────────────────────────────────────────────────── */
uint64_t calibrate_tsc(void)
{
    /*
     * Real implementation: measure TSC ticks over a known PIT
     * interval, then compute frequency.
     * Here we simply return a simulated 1 GHz.
     */
    tsc_frequency = 1000000000ULL;
    printf("[timekeeping] TSC frequency calibrated: %llu Hz\n",
           (unsigned long long)tsc_frequency);
    return tsc_frequency;
}

/* ─────────────────────────────────────────────────────────────
 * timekeeping_init  (time_init equivalent)
 * ───────────────────────────────────────────────────────────── */
void timekeeping_init(void)
{
    select_timer();
    calibrate_tsc();
    calibrate_delay();

    int64_t cmos = get_cmos_time();
    xtime.tv_sec  = cmos;
    xtime.tv_nsec = 0;
    monotonic_base = 0;
    tsc_at_boot    = 0;

    printf("[timekeeping] init: xtime.tv_sec=%ld\n", (long)xtime.tv_sec);
}

/* ─────────────────────────────────────────────────────────────
 * update_times — advance xtime by one tick
 * ───────────────────────────────────────────────────────────── */
void update_times(void)
{
    xtime.tv_nsec += TICK_NSEC;
    if (xtime.tv_nsec >= 1000000000UL) {
        xtime.tv_nsec -= 1000000000UL;
        xtime.tv_sec++;
    }
    monotonic_base += TICK_NSEC;
}

int64_t  timekeeping_get_seconds(void)   { return xtime.tv_sec;    }
uint64_t timekeeping_monotonic_ns(void)  { return monotonic_base;  }

/* ─────────────────────────────────────────────────────────────
 * Delay functions
 * ───────────────────────────────────────────────────────────── */
void calibrate_delay(void)
{
    /*
     * Real kernel: measure how many empty loops fit in a known
     * time interval (BogoMIPS calibration).
     */
    loops_per_usec = 1000; /* simulated */
    printf("[timekeeping] delay calibration: %lu loops/usec\n",
           (unsigned long)loops_per_usec);
}

void udelay(unsigned long usecs)
{
    volatile unsigned long loops = usecs * loops_per_usec;
    while (loops--);
}

void ndelay(unsigned long nsecs)
{
    volatile unsigned long loops = (nsecs * loops_per_usec) / 1000;
    while (loops--);
}
