 /* BSD and Linux */
 #ifndef UIX_IOCTL_H
#define UIX_IOCTL_H

#include "uix_types.h"

#define UIX_TCGETS     0x5401
#define UIX_TCSETS     0x5402
#define UIX_TCSETSW    0x5403
#define UIX_TCSETSF    0x5404
#define UIX_TIOCGWINSZ 0x5413
#define UIX_TIOCSWINSZ 0x5414
#define UIX_TIOCGPGRP  0x540F
#define UIX_TIOCSPGRP  0x5410
#define UIX_FIONREAD   0x541B
#define UIX_FIONBIO    0x5421
#define UIX_FIOCLEX    0x5451
#define UIX_FIONCLEX   0x5450
#define UIX_BLKGETSIZE 0x1260
#define UIX_BLKSSZGET  0x1268

typedef struct uix_winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
} uix_winsize_t;

int uix_ioctl(int fd, unsigned long request, ...);

#endif /* UIX_IOCTL_H */
