#include "uix_termios.h"
#include "uix_ioctl.h"
#include "uix_errno.h"
#include "uix_string.h"

int uix_tcgetattr(int fd, uix_termios_t *termios_p)
{
    if (!termios_p) { uix_errno = UIX_EFAULT; return -1; }
    extern int sys_ioctl(int,unsigned long,void*)
        __attribute__((weak));
    if (sys_ioctl)
        return sys_ioctl(fd, UIX_TCGETS, termios_p);
    /* Provide sensible defaults */
    uix_memset(termios_p, 0, sizeof(*termios_p));
    termios_p->c_iflag = UIX_ICRNL | UIX_IXON;
    termios_p->c_oflag = UIX_OPOST | UIX_ONLCR;
    termios_p->c_cflag = UIX_CS8   | UIX_CREAD | UIX_CLOCAL;
    termios_p->c_lflag = UIX_ISIG  | UIX_ICANON | UIX_ECHO |
                         UIX_ECHOE | UIX_ECHOK;
    termios_p->c_cc[UIX_VEOF]   = 4;    /* Ctrl-D */
    termios_p->c_cc[UIX_VINTR]  = 3;    /* Ctrl-C */
    termios_p->c_cc[UIX_VKILL]  = 21;   /* Ctrl-U */
    termios_p->c_cc[UIX_VERASE] = 127;  /* DEL    */
    termios_p->c_ispeed = UIX_B9600;
    termios_p->c_ospeed = UIX_B9600;
    return 0;
}

int uix_tcsetattr(int fd, int optional_actions,
                  const uix_termios_t *termios_p)
{
    if (!termios_p) { uix_errno = UIX_EFAULT; return -1; }
    extern int sys_ioctl(int,unsigned long,void*)
        __attribute__((weak));
    unsigned long cmd;
    switch (optional_actions) {
    case UIX_TCSANOW:   cmd = UIX_TCSETS;  break;
    case UIX_TCSADRAIN: cmd = UIX_TCSETSW; break;
    case UIX_TCSAFLUSH: cmd = UIX_TCSETSF; break;
    default: uix_errno = UIX_EINVAL; return -1;
    }
    if (sys_ioctl)
        return sys_ioctl(fd, cmd, (void *)termios_p);
    return 0;
}

int uix_tcsendbreak(int fd, int duration)
    { (void)fd; (void)duration; return 0; }
int uix_tcdrain(int fd)
    { (void)fd; return 0; }
int uix_tcflush(int fd, int queue_selector)
    { (void)fd; (void)queue_selector; return 0; }
int uix_tcflow(int fd, int action)
    { (void)fd; (void)action; return 0; }

uix_speed_t uix_cfgetispeed(const uix_termios_t *t)
    { return t ? t->c_ispeed : UIX_B0; }
uix_speed_t uix_cfgetospeed(const uix_termios_t *t)
    { return t ? t->c_ospeed : UIX_B0; }
int uix_cfsetispeed(uix_termios_t *t, uix_speed_t s)
    { if(!t){uix_errno=UIX_EFAULT;return -1;} t->c_ispeed=s; return 0; }
int uix_cfsetospeed(uix_termios_t *t, uix_speed_t s)
    { if(!t){uix_errno=UIX_EFAULT;return -1;} t->c_ospeed=s; return 0; }

void uix_cfmakeraw(uix_termios_t *t)
{
    if (!t) return;
    t->c_iflag &= ~(UIX_IGNBRK|UIX_BRKINT|UIX_PARMRK|UIX_ISTRIP|
                    UIX_INLCR|UIX_IGNCR|UIX_ICRNL|UIX_IXON);
    t->c_oflag &= ~UIX_OPOST;
    t->c_lflag &= ~(UIX_ECHO|UIX_ECHONL|UIX_ICANON|UIX_ISIG);
    t->c_cflag &= ~(UIX_CSIZE|UIX_PARENB);
    t->c_cflag |=  UIX_CS8;
    t->c_cc[UIX_VMIN]  = 1;
    t->c_cc[UIX_VTIME] = 0;
}

uix_pid_t uix_tcgetpgrp(int fd)
{
    uix_pid_t pgrp = -1;
    uix_ioctl(fd, UIX_TIOCGPGRP, &pgrp);
    return pgrp;
}

int uix_tcsetpgrp(int fd, uix_pid_t pgrp)
{
    return uix_ioctl(fd, UIX_TIOCSPGRP, &pgrp);
}
