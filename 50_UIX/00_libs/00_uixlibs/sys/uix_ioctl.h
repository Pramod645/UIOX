 /* BSD and Linux */
 #ifndef __SYS_UIX_IOCTL__H
#define __SYS_UIX_IOCTL__H

#include "sys/uix_types.h"

#define UIX_TCGETS     0x5401    // Gets terminal attributes via ioctl
#define UIX_TCSETS     0x5402    // Sets terminal attributes immediately
#define UIX_TCSETSW    0x5403
#define UIX_TCSETSF    0x5404
#define UIX_TIOCGWINSZ 0x5413       // Gets terminal window size into winsize
#define UIX_TIOCSWINSZ 0x5414       // Sets terminal window size
#define UIX_TIOCGPGRP  0x540F       // Gets foreground process group of terminal
#define UIX_TIOCSPGRP  0x5410        // Sets foreground process group
#define UIX_FIONREAD   0x541B     // Returns number of bytes available to read
#define UIX_FIONBIO    0x5421      // Sets/clears non-blocking mode
#define UIX_FIOCLEX    0x5451
#define UIX_FIONCLEX   0x5450
#define UIX_BLKGETSIZE 0x1260
#define UIX_BLKSSZGET  0x1268

typedef struct uix_winsize {
    unsigned short ws_row;          // Terminal rows
    unsigned short ws_col;           // Terminal columns
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
} uix_winsize_t;

int uix_ioctl(int fd, unsigned long request, ...);  // Device-specific control operation — POSIX

#endif /* __SYS_UIX_IOCTL__H */
/* ***This is End of file, there is no more line should be added after this line*** */
