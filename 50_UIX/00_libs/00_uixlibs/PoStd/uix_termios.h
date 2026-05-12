
#ifndef __UIX_TERMIOS__H
#define __UIX_TERMIOS__H
/*
termios.h is a cornerstone for terminal (TTY) control in POSIX systems.  
The header <termios.h> defines the interface for configuring, reading, and writing terminal attributes — things 
like baud rate, input modes, echo, canonical (line-editing) mode, etc.

*/
/* This is for only POXIS */

//#include "features.h"


#include "sys/uix_types.h"

typedef uix_uint32_t uix_tcflag_t;
typedef uix_uint8_t  uix_cc_t;
typedef uix_uint32_t uix_speed_t;

#define UIX_NCCS 32

#define UIX_IGNBRK 0x0001
#define UIX_BRKINT 0x0002
#define UIX_IGNPAR 0x0004
#define UIX_PARMRK 0x0008
#define UIX_INPCK  0x0010
#define UIX_ISTRIP 0x0020
#define UIX_INLCR  0x0040
#define UIX_IGNCR  0x0080
#define UIX_ICRNL  0x0100
#define UIX_IXON   0x0400
#define UIX_IXOFF  0x1000

#define UIX_OPOST  0x0001
#define UIX_ONLCR  0x0004
#define UIX_OCRNL  0x0008

#define UIX_CSIZE  0x0030
#define UIX_CS5    0x0000
#define UIX_CS6    0x0010
#define UIX_CS7    0x0020
#define UIX_CS8    0x0030       // 8-bit characters
#define UIX_CSTOPB 0x0040
#define UIX_CREAD  0x0080
#define UIX_PARENB 0x0100
#define UIX_PARODD 0x0200
#define UIX_HUPCL  0x0400
#define UIX_CLOCAL 0x0800

#define UIX_ISIG   0x0001     // Enable signal generation (INTR, QUIT, SUSP)
#define UIX_ICANON 0x0002     // Canonical mode — line-based input
#define UIX_ECHO   0x0008     // Echo input characters
#define UIX_ECHOE  0x0010
#define UIX_ECHOK  0x0020
#define UIX_ECHONL 0x0040
#define UIX_NOFLSH 0x0080
#define UIX_TOSTOP 0x0100

#define UIX_VEOF   4     // Index of EOF character (Ctrl-D)
#define UIX_VEOL   11
#define UIX_VERASE 2
#define UIX_VINTR  0      // Index of interrupt character (Ctrl-C)
#define UIX_VKILL  3
#define UIX_VMIN   6      // Minimum chars for non-canonical read
#define UIX_VQUIT  1
#define UIX_VSTART 8
#define UIX_VSTOP  9
#define UIX_VSUSP  10
#define UIX_VTIME  5     // Timeout for non-canonical read

#define UIX_B0      0
#define UIX_B9600   13    // 9600 baud rate constant
#define UIX_B19200  14
#define UIX_B38400  15
#define UIX_B57600  4097
#define UIX_B115200 4098

#define UIX_TCSANOW   0      // Apply changes immediately
#define UIX_TCSADRAIN 1
#define UIX_TCSAFLUSH 2       // Apply after flushing output, discard pending input

typedef struct uix_termios {
    uix_tcflag_t c_iflag;         // Input mode flags: parity, CR/NL translation
    uix_tcflag_t c_oflag;        // Output mode flags: post-processing
    uix_tcflag_t c_cflag;        // Control flags: baud rate, character size
    uix_tcflag_t c_lflag;         // Local flags: echo, canonical mode, signals
    uix_cc_t     c_line;
    uix_cc_t     c_cc[UIX_NCCS];  // Special characters array
    uix_speed_t  c_ispeed;
    uix_speed_t  c_ospeed;
} uix_termios_t;

int         uix_tcgetattr  (int fd, uix_termios_t *t);   // Reads terminal attributes
int         uix_tcsetattr  (int fd, int action, const uix_termios_t *t);// Writes terminal attributes
int         uix_tcsendbreak(int fd, int duration);
int         uix_tcdrain    (int fd);
int         uix_tcflush    (int fd, int queue);
int         uix_tcflow     (int fd, int action);
uix_speed_t uix_cfgetispeed(const uix_termios_t *t);
uix_speed_t uix_cfgetospeed(const uix_termios_t *t);
int         uix_cfsetispeed(uix_termios_t *t, uix_speed_t speed);
int         uix_cfsetospeed(uix_termios_t *t, uix_speed_t speed);
void        uix_cfmakeraw  (uix_termios_t *t);                   //Sets terminal to raw mode — disables all processing
uix_pid_t   uix_tcgetpgrp  (int fd);
int         uix_tcsetpgrp  (int fd, uix_pid_t pgrp);


#endif /* End of __UIX_TERMIOS__H */
/* ***This is End of file, there is no more line should be added after this line*** */
