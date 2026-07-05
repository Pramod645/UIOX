/**
 * @file  uiox_uart_proto.h
 * @brief UIOX UART Protocol — ANSI escape, line discipline, break handling.
 * @version 1.0.0
 * @date    2026-07-05
 */

 #ifndef UIOX_UART_PROTO_H
 #define UIOX_UART_PROTO_H
 
 #include "uiox_uart_if.h"
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /* =========================================================================
  * Line discipline mode
  * ====================================================================== */
 
 typedef enum {
     UIOX_UART_LDISC_RAW    = 0,  /**< Raw: no processing               */
     UIOX_UART_LDISC_COOKED = 1,  /**< Cooked: echo, line editing, sigs */
     UIOX_UART_LDISC_CBREAK = 2,  /**< cbreak: per-char, no echo        */
 } uiox_uart_ldisc_t;
 
 /* =========================================================================
  * ANSI escape sequence parser state
  * ====================================================================== */
 
 typedef enum {
     UIOX_ANSI_GROUND   = 0,
     UIOX_ANSI_ESC      = 1,  /**< Saw ESC                              */
     UIOX_ANSI_CSI      = 2,  /**< Saw ESC[                             */
     UIOX_ANSI_OSC      = 3,  /**< Saw ESC]                             */
     UIOX_ANSI_COMPLETE = 4,  /**< Sequence complete                    */
 } uiox_ansi_state_t;
 
 #define UIOX_ANSI_PARAM_MAX  8u
 #define UIOX_ANSI_BUF_MAX    32u
 
 typedef struct {
     uiox_ansi_state_t state;
     char              buf[UIOX_ANSI_BUF_MAX];
     uint8_t           buf_len;
     int32_t           params[UIOX_ANSI_PARAM_MAX];
     uint8_t           param_count;
     char              final_byte;
 } uiox_ansi_parser_t;
 
 /* =========================================================================
  * Protocol context
  * ====================================================================== */
 
 #define UIOX_UART_LINE_BUF_SIZE  256u
 
 typedef struct {
     uiox_uart_if_t    *uif;
     uiox_uart_ldisc_t  ldisc;
     uiox_ansi_parser_t ansi;
 
     /* Cooked mode line buffer */
     char               line_buf[UIOX_UART_LINE_BUF_SIZE];
     uint32_t           line_len;
     bool               line_ready;
 
     /* Special characters (termios-style) */
     char               c_erase;   /**< Backspace/delete char (DEL/BS)  */
     char               c_kill;    /**< Kill line (^U)                  */
     char               c_intr;    /**< Interrupt (^C → SIGINT)         */
     char               c_eof;     /**< EOF (^D)                        */
     char               c_eol;     /**< End-of-line (newline)           */
 
     bool               echo;      /**< Echo received chars to TX       */
     bool               break_pending;
 
     /* Stats */
     uint32_t           ansi_sequences;
     uint32_t           lines_received;
 } uiox_uart_proto_t;
 
 /* =========================================================================
  * Protocol API
  * ====================================================================== */
 
 int  uiox_uart_proto_init       (uiox_uart_proto_t *proto,
                                   uiox_uart_if_t *uif);
 int  uiox_uart_proto_set_ldisc  (uiox_uart_proto_t *proto,
                                   uiox_uart_ldisc_t mode);
 void uiox_uart_proto_set_echo   (uiox_uart_proto_t *proto, bool echo);
 
 /* Transmit: applies ANSI cursor / colour sequences */
 int  uiox_uart_proto_putc       (uiox_uart_proto_t *proto, char c);
 int  uiox_uart_proto_puts       (uiox_uart_proto_t *proto, const char *s);
 int  uiox_uart_proto_printf     (uiox_uart_proto_t *proto,
                                   const char *fmt, ...);
 
 /* ANSI helpers */
 void uiox_uart_proto_cursor_up  (uiox_uart_proto_t *proto, uint32_t n);
 void uiox_uart_proto_cursor_dn  (uiox_uart_proto_t *proto, uint32_t n);
 void uiox_uart_proto_cursor_col (uiox_uart_proto_t *proto, uint32_t col);
 void uiox_uart_proto_clear_line (uiox_uart_proto_t *proto);
 void uiox_uart_proto_clear_scr  (uiox_uart_proto_t *proto);
 void uiox_uart_proto_set_colour (uiox_uart_proto_t *proto,
                                   uint8_t fg, uint8_t bg);
 void uiox_uart_proto_reset_attr (uiox_uart_proto_t *proto);
 
 /* Receive: processes through line discipline */
 int  uiox_uart_proto_process_rx (uiox_uart_proto_t *proto);
 bool uiox_uart_proto_line_ready (const uiox_uart_proto_t *proto);
 int  uiox_uart_proto_read_line  (uiox_uart_proto_t *proto,
                                   char *buf, uint32_t max);
 
 /* Break */
 void uiox_uart_proto_send_break (uiox_uart_proto_t *proto, uint32_t ms);
 
 #ifdef __cplusplus
 }
 #endif
 #endif /* UIOX_UART_PROTO_H */
 