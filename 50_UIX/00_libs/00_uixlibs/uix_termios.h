
#ifndef __UIX_TERMIOS__H
#define __UIX_TERMIOS__H
/*
termios.h is a cornerstone for terminal (TTY) control in POSIX systems.  
The header <termios.h> defines the interface for configuring, reading, and writing terminal attributes — things 
like baud rate, input modes, echo, canonical (line-editing) mode, etc.

*/
/* This is for only POXIS */

#include "features.h"

#include <sys/types.h>

#if  (define __POSIX)

#ifdef _cplusplus
extern "C" {
#endif

// Control characters array indices /
#define VINTR    0   // Interrupt /
#define VQUIT    1   // Quit /
#define VERASE   2   // Erase /
#define VKILL    3   // Kill line /
#define VEOF     4   // End of file /
#define VTIME    5   // Timeout for noncanonical read (deciseconds) /
#define VMIN     6   // Minimum number of chars for noncanonical read /

// Input flag constants /
#define IGNBRK  0000001
#define BRKINT  0000002
#define IGNPAR  0000004
#define PARMRK  0000010
#define INPCK   0000020
#define ISTRIP  0000040
#define INLCR   0000100
#define IGNCR   0000200
#define ICRNL   0000400
#define IXON    0002000
#define IXANY   0004000
#define IXOFF   0010000

// Output flag constants /
#define OPOST   0000001

// Control flag constants /
#define CSIZE   0000060
#define CS8     0000060
#define CREAD   0000200
#define HUPCL   0002000
#define CLOCAL  0004000

// Local flag constants /
#define ISIG    0000001
#define ICANON  0000002
#define ECHO    0000010
#define ECHOE   0000020
#define ECHOK   0000040
#define ECHONL  0000100
#define NOFLSH  0000200

// Baud rate constants (simplified) /
#define B0      0000000
#define B9600   0000015
#define B115200 0000017

typedef unsigned char cct;
typedef unsigned int  speedt;
typedef unsigned int  tcflagt;

// Structure describing terminal attributes /
struct termios {
    tcflagt ciflag;   // Input modes /
    tcflagt coflag;   // Output modes /
    tcflagt ccflag;   // Control modes /
    tcflagt clflag;   // Local modes /
    cct     ccc[20];  // Control characters /
    speedt  cispeed;  // Input speed /
    speedt  cospeed;  // Output speed /
};

// Functions for getting/setting terminal attributes /
int tcgetattr(int fd, struct termios termiosp);
int tcsetattr(int fd, int optionalactions, const struct termios termiosp);
int cfsetispeed(struct termios termiosp, speedt speed);
int cfsetospeed(struct termios termiosp, speedt speed);

// Optional actions for tcsetattr() /
#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#ifdef cplusplus
}
#endif


#endif /* End  of POXIS */



#ifndef UIX_TERMIOS_H
#define UIX_TERMIOS_H

#include "uix_types.h"

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

#endif /* UIX_TERMIOS_H */


#endif /* End of __UIX_TERMIOS__H */
/* ***This is End of file, there is no more line should be added after this line*** */