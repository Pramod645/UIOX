
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

#endif /* End of __UIX_TERMIOS__H */
/* ***This is End of file, there is no more line should be added after this line*** */