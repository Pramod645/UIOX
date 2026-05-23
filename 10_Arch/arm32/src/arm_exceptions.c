/*
 * arm_exceptions.c — ARM exception handler stubs
 */

 #include "../include/arm_exceptions.h"

 void arm_exc_reset     (arm_exception_t e,arm_word_t c,arm_word_t p){(void)e;(void)c;(void)p;}
 void arm_exc_undef     (arm_exception_t e,arm_word_t c,arm_word_t p){(void)e;(void)c;(void)p;}
 void arm_exc_swi       (arm_exception_t e,arm_word_t c,arm_word_t p){(void)e;(void)c;(void)p;}
 void arm_exc_prefetch  (arm_exception_t e,arm_word_t c,arm_word_t p){(void)e;(void)c;(void)p;}
 void arm_exc_data_abort(arm_exception_t e,arm_word_t c,arm_word_t p){(void)e;(void)c;(void)p;}
 void arm_exc_irq       (arm_exception_t e,arm_word_t c,arm_word_t p){(void)e;(void)c;(void)p;}
 void arm_exc_fiq       (arm_exception_t e,arm_word_t c,arm_word_t p){(void)e;(void)c;(void)p;}
 