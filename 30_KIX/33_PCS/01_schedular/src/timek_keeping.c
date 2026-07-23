/*
 * 30_KIX/33_PCS/01_schedular/src/timekeeping.c
 *
 * System header fixes (v2.0)
 * ──────────────────────────
 *   REMOVED: #include <stdio.h>   → uiox_printf  via uiox_klibc.h
 *   REMOVED: #include <string.h>  → uiox_memset  via uiox_klibc.h
 *   REMOVED: #include <time.h>    → time_t/clock_t via uiox_klibc.h
 *   All three flow in: timekeeping.h → sched_types.h → uiox_klibc.h
 *
 *   FIXED:  get_cmos_time() — removed time(NULL) (libc call).
 *           In a real kernel this reads CMOS I/O ports 0x70/0x71.
 *           In simulation we use xtime.tv_sec which is already
 *           maintained by timekeeping_init() / update_times().
 *           time_t replaced with int64_t (same underlying type
 *           from uiox_klibc.h §4).
 *
 * No other logic changed.
 */

 #include "../include/time_keeping.h"
 /* stdio.h / string.h / time.h removed — provided transitively via  */
 /* timekeeping.h → sched_types.h → uiox_klibc.h                     */
 
 /* ── Module-private state ───────────────────────────────────────────── */
 static uint64_t   tsc_frequency  = 1000000000ULL; /* 1 GHz default    */
 static uint64_t   tsc_at_boot    = 0;
 static uint64_t   monotonic_base = 0;             /* ns since init     */
 static uint64_t   loops_per_usec = 1000;          /* calibrated delay  */
 static TimerOpts *active_timer   = NULL;
 
 /* ── Timer source implementations (simulated) ───────────────────────── */
 static void     pit_mark_offset(void)                 { }
 static uint64_t pit_get_offset(void)                  { return TICK_NSEC / 2; }
 static uint64_t pit_monotonic(void)                   { return jiffies * TICK_NSEC; }
 static void     pit_delay(unsigned long loops)        { volatile unsigned long l = loops; while (l--); }
 
 static void     tsc_mark_offset(void)                 { }
 static uint64_t tsc_get_offset(void)                  { return 0; }
 static uint64_t tsc_monotonic(void)                   { return jiffies * TICK_NSEC; }
 static void     tsc_delay(unsigned long loops)        { pit_delay(loops); }
 
 static void     hpet_mark_offset(void)                { }
 static uint64_t hpet_get_offset(void)                 { return 0; }
 static uint64_t hpet_monotonic(void)                  { return jiffies * TICK_NSEC; }
 static void     hpet_delay(unsigned long l)           { pit_delay(l); }
 
 /* ── Timer source table — HPET > TSC > PIT > none ───────────────────── */
 static TimerOpts timer_table[] = {
     { "timer_hpet", TIMER_SRC_HPET,
       hpet_mark_offset, hpet_get_offset, hpet_monotonic, hpet_delay },
     { "timer_tsc",  TIMER_SRC_TSC,
       tsc_mark_offset,  tsc_get_offset,  tsc_monotonic,  tsc_delay  },
     { "timer_pit",  TIMER_SRC_PIT,
       pit_mark_offset,  pit_get_offset,  pit_monotonic,  pit_delay  },
     { "timer_none", TIMER_SRC_NONE,
       NULL, NULL, NULL, pit_delay }
 };
 #define TIMER_TABLE_COUNT 4
 
 /* ── select_timer — pick best available timer source ────────────────── */
 TimerOpts *select_timer(void)
 {
     int i;
     /* In simulation every source is "available"; pick HPET first */
     for (i = 0; i < TIMER_TABLE_COUNT; i++) {
         if (timer_table[i].source != TIMER_SRC_NONE) {
             active_timer = &timer_table[i];
             printf("[timekeeping] selected timer source: %s\n",
                    active_timer->name);
             return active_timer;
         }
     }
     active_timer = &timer_table[TIMER_TABLE_COUNT - 1]; /* none */
     return active_timer;
 }
 
 /* ── get_cmos_time — read wall-clock from simulated RTC ─────────────── */
 int64_t get_cmos_time(void)
 {
     /*
      * Real kernel: read CMOS I/O ports 0x70 / 0x71.
      * Freestanding simulation: return xtime.tv_sec which is already
      * set by timekeeping_init() from the BSP-provided epoch value.
      *
      * REMOVED: time_t t = time(NULL);  ← libc call, not available
      *          with -nostdinc / -nostdlib.
      */
     int64_t t = xtime.tv_sec;
     printf("[timekeeping] CMOS time: %ld\n", (long)t);
     return t;
 }
 
 /* ── calibrate_tsc ───────────────────────────────────────────────────── */
 uint64_t calibrate_tsc(void)
 {
     /* Real: measure TSC ticks over a known PIT interval.
        Simulation: return a fixed 1 GHz.                  */
     tsc_frequency = 1000000000ULL;
     printf("[timekeeping] TSC frequency calibrated: %llu Hz\n",
            (unsigned long long)tsc_frequency);
     return tsc_frequency;
 }
 
 /* ── timekeeping_init (time_init equivalent) ─────────────────────────── */
 void timekeeping_init(void)
 {
     int64_t cmos;
     select_timer();
     calibrate_tsc();
     calibrate_delay();
     cmos          = get_cmos_time();
     xtime.tv_sec  = cmos;
     xtime.tv_nsec = 0;
     monotonic_base = 0;
     tsc_at_boot    = 0;
     printf("[timekeeping] init complete: xtime.sec=%ld\n", (long)xtime.tv_sec);
 }
 
 /* ── update_times — called every tick ───────────────────────────────── */
 void update_times(void)
 {
     xtime.tv_nsec += (uint32_t)TICK_NSEC;
     if (xtime.tv_nsec >= 1000000000UL) {
         xtime.tv_nsec -= 1000000000UL;
         xtime.tv_sec++;
     }
     monotonic_base += TICK_NSEC;
 }
 
 /* ── timekeeping_get_seconds ─────────────────────────────────────────── */
 int64_t timekeeping_get_seconds(void)
 {
     return xtime.tv_sec;
 }
 
 /* ── timekeeping_monotonic_ns ────────────────────────────────────────── */
 uint64_t timekeeping_monotonic_ns(void)
 {
     if (active_timer && active_timer->monotonic_clock)
         return active_timer->monotonic_clock();
     return monotonic_base;
 }
 
 /* ── calibrate_delay ─────────────────────────────────────────────────── */
 void calibrate_delay(void)
 {
     loops_per_usec = 1000; /* simulated BogoMIPS calibration */
     printf("[timekeeping] delay calibration: %lu loops/usec\n",
            (unsigned long)loops_per_usec);
 }
 
 /* ── udelay / ndelay ─────────────────────────────────────────────────── */
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
 