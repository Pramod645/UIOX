#1. 
Console:
stdin,stdout,stderr



---------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------
#2. Segments:File descriptor, 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

a.STDIN_FILENO, STDOUT_FILENO, and STDERR_FILENO

b.
#include <unistd.h> 
 ssize_t pread(int fd, void *buf, size_t num, off_t offset); 
 ssize_t pwrite(int fd, void *buf, size_t num, off_t offset); 
Returns: number of bytes read/written, -1 on error

#include <unistd.h> 
 int dup(int oldfd); 
 int dup2(int oldfd, int newfd); 
Returns: newfd, -1 on error

 #include <fcntl.h> 
 int fcntl(int fd, int cmd, ...); 
Returns: depends on cmd, -1 on error

#include <sys/ioctl.h> 
 int ioctl(int fd, unsigned long request, ...); 
Returns: depends on request, -1 on error

c.
#include <fcntl.h> 
 int creat(const char *pathname, mode_t mode); 
Returns: file descriptor if OK, -1 on error


#include <fcntl.h> 
 int creat(const char *pathname, mode_t mode); 
Returns: file descriptor if OK, -1 on error

#include <fcntl.h> 
 int open(const char *pathname, int oflag, ... /* mode_t mode */); 
 int openat(int dirfd, const char *pathname, int oflag, ... /* mode_t mode); 
Returns: file descriptor if OK, -1 on error

#include <unistd.h> 
 int close(int fd); 
Returns: 0 if OK, -1 on error

#include <unistd.h> 
 int close(int fd); 
Returns: 0 if OK, -1 on error

d.
#include <unistd.h> 
 ssize_t read(int fd, void *buf, size_t num); 
Returns: number of bytes read; 0 on EOF, -1 on error

 #include <unistd.h> 
 ssize_t write(int fd, void *buf, size_t num); 
Returns: number of bytes written if OK; -1 on error

 #include <sys/types.h> 
 #include <fcntl.h> 
 off_t lseek(int fd, off_t offset, int whence); 
Returns: new offset if OK; -1 on error

--------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------
#3.files and directory
a.
#include <sys/stat.h>
int stat(const char *restrict pathname, s truct stat *restrict buf );
int fstat(int fd, s truct stat *buf );
int lstat(const char *restrict pathname, s truct stat *restrict buf );
int fstatat(int fd, c onst char *restrict pathname,
struct stat *restrict buf, i nt flag);
All four return: 0 if OK,−1 on error

b.
#include <unistd.h>
int access(const char *pathname, int mode);
int faccessat(int fd, const char *pathname, int mode, int flag);
Both return: 0 if OK,−1 on error

c.
#include <sys/stat.h>
mode_t umask(mode_t cmask);
Returns: previous file mode creation mask

d.
#include <sys/stat.h>
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag);
All three return: 0 if OK,−1 on error

e.
Sticky Bit

f.
#include <unistd.h>
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int fd, const char *pathname, uid_t owner, gid_t group,
int flag);
int lchown(const char *pathname, uid_t owner, gid_t group);
All four return: 0 if OK,−1 on error

g.
#include <unistd.h>
int truncate(const char *pathname, off_t length);
int ftruncate(int fd, off_t length);
Both return: 0 if OK,−1 on error

h.
File Systems

i.
#include <unistd.h>
int link(const char *existingpath, const char *newpath);
int linkat(int efd, const char *existingpath, int nfd, const char *newpath,
int flag);
Both return: 0 if OK,−1 on error

#include <unistd.h>
int unlink(const char *pathname);
int unlinkat(int fd, const char *pathname, int flag);
Both return: 0 if OK,−1 on error

#include <stdio.h>
int remove(const char *pathname);
Returns: 0 if OK,−1 on error

j.
#include <stdio.h>
int rename(const char *oldname, const char *newname);
int renameat(int oldfd, const char *oldname, int newfd,
const char *newname);
Both return: 0 if OK,−1 on error

k.
#include <unistd.h>
int symlink(const char *actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);
Both return: 0 if OK,−1 on error

#include <unistd.h>
ssize_t readlink(const char* restrict pathname, char *restrict buf,
size_t bufsize);
ssize_t readlinkat(int fd, const char* restrict pathname,
char *restrict buf, size_t bufsize);
Both return: number of bytes read if OK,−1 on error

l.
#include <sys/stat.h>
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2],
int flag);
Both return: 0 if OK,−1 on error
==================================================================================================
m.4
#include <sys/stat.h>
int mkdir(const char *pathname, mode_t mode);
int mkdirat(int fd, const char *pathname, mode_t mode);
Both return: 0 if OK,−1 on error

#include <unistd.h>
int rmdir(const char *pathname);
Returns: 0 if OK,−1 on error

n.
#include <dirent.h>
DIR *opendir(const char *pathname);
DIR *fdopendir(int fd);
Both return: pointer if OK, NULLon error
struct dirent *readdir(DIR *dp);
Returns: pointer if OK, NULLat end of directory or error
void rewinddir(DIR *dp);
int closedir(DIR *dp);
Returns: 0 if OK,−1 on error
long telldir(DIR *dp);
Returns: current location in directory associated with dp
void seekdir(DIR *dp, long loc);

o.
#include <unistd.h>
int chdir(const char *pathname);
int fchdir(int fd);
Both return: 0 if OK,−1 on error

#include <unistd.h>
char *getcwd(char *buf, size_t size);
Returns: buf if OK, NULLon error

==================================================================================================
==================================================================================================
#5. standerd I/O Library
a.
#include <stdio.h>
#include <wchar.h>
int fwide(FILE *fp, i nt mode);
Returns:Positive if stream is wide oriented,
negative if stream is byte oriented,
or 0 if stream has no orientation

b.
#include <stdio.h>
void setbuf(FILE *restrict fp, c har *restrict buf );
int setvbuf(FILE *restrict fp, c har *restrict buf, i nt mode,
size_t size);
Returns: 0 if OK, non zero on error

c.
#include <stdio.h>
int fflush(FILE *fp);
Returns: 0 if OK, EOFon error

d.
#include <stdio.h>
FILE *fopen(const char *restrict pathname, const char *restrict type);
FILE *freopen(const char *restrict pathname, const char *restrict type,
FILE *restrict fp);
FILE *fdopen(int fd, const char *type);
All three return: file pointer if OK, NULLon erro

#include <stdio.h>
int fclose(FILE *fp);
Returns: 0 if OK, EOFon error

e.
#include <stdio.h>
int getc(FILE *fp);
int fgetc(FILE *fp);
int getchar(void);
All three return: next character if OK, EOFon end of file or error

#include <stdio.h>
int ferror(FILE *fp);
int feof(FILE *fp);
Both return: nonzero (true) if condition is true, 0 (false) otherwise
void clearerr(FILE *fp);

#include <stdio.h>
int ungetc(int c, FILE *fp);
Returns: c if OK, EOFon error

f.
include <stdio.h>
int putc(int c, FILE *fp);
int fputc(int c, FILE *fp);
int putchar(int c);
All three return: c if OK, EOFon error

g.
#include <stdio.h>
char *fgets(char *restrict buf, int n, FILE *restrict fp);
char *gets(char *buf );
Both return: bufif OK, NULLon end of file or error

#include <stdio.h>
int fputs(const char *restrict str, FILE *restrict fp);
int puts(const char *str);
Both return: non-negative value if OK, EOFon error

h.Binary I/O 
#include <stdio.h>
size_t fread(void *restrict ptr, size_t size, size_t nobj,
FILE *restrict fp);
size_t fwrite(const void *restrict ptr, size_t size, size_t nobj,
FILE *restrict fp);
Both return: number of objects read or written

i.
#include <stdio.h>
long ftell(FILE *fp);
Returns: current file position indicator if OK,−1L on error
int fseek(FILE *fp, long offset, int whence);
Returns: 0 if OK,−1 on error
void rewind(FILE *fp);

#include <stdio.h>
off_t ftello(FILE *fp);
Returns: current file position indicator if OK, (off_t)−1on error
int fseeko(FILE *fp, off_t offset, int whence);
Returns: 0 if OK,−1 on error

#include <stdio.h>
int fgetpos(FILE *restrict fp, fpos_t *restrict pos);
int fsetpos(FILE *fp, const fpos_t *pos);
Both return: 0 if OK, nonzero on error

j.
#include <stdio.h>
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict fp, const char *restrict format, ...);
int dprintf(int fd, const char *restrict format, ...);
All three return: number of characters output if OK, negative value if output error
int sprintf(char *restrict buf, const char *restrict format, ...);
Returns: number of characters stored in array if OK, negative value if encoding error
int snprintf(char *restrict buf, size_t n,
const char *restrict format, ...);
Returns: number of characters that would have been stored in array
if buffer was large enough, negative value if encoding error

k.
#include <stdarg.h>
#include <stdio.h>
int vprintf(const char *restrict format, va_list arg);
int vfprintf(FILE *restrict fp, const char *restrict format,
va_list arg);
int vdprintf(int fd, const char *restrict format, va_list arg);
All three return: number of characters output if OK, negative value if output error
int vsprintf(char *restrict buf, const char *restrict format,
va_list arg);
Returns: number of characters stored in array if OK, negative value if encoding error
int vsnprintf(char *restrict buf, size_t n,
const char *restrict format, va_list arg);
Returns: number of characters that would have been stored in array
if buffer was large enough, negative value if encoding error

l. Formatted Input
#include <stdio.h>
int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict fp, const char *restrict format, ...);
int sscanf(const char *restrict buf, const char *restrict format, ...);
All three return: number of input items assigned,
EOFif input error or end of file before any conversion

#include <stdarg.h>
#include <stdio.h>
int vscanf(const char *restrict format, va_list arg);
int vfscanf(FILE *restrict fp, const char *restrict format,
va_list arg);
int vsscanf(const char *restrict buf, const char *restrict format,
va_list arg);
All three return: number of input items assigned,
EOFif input error or end of file before any conversion

m.
#include <stdio.h>
int fileno(FILE *fp);
Returns: the file descriptor associated with the stream

n.
#include <stdio.h>
char *tmpnam(char *ptr);
Returns: pointer to unique pathname
FILE *tmpfile(void);
Returns: file pointer if OK, NULLon error

#include <stdlib.h>
char *mkdtemp(char *template);
Returns: pointer to directory name if OK, NULLon error
int mkstemp(char *template);
Returns: file descriptor if OK,−1 on error

O. Memory stream
include <stdio.h>
FILE *fmemopen(void *restrict buf, size_t size,
const char *restrict type);
Returns: stream pointer if OK, NULLon error

#include <stdio.h>
FILE *open_memstream(char **bufp, size_t *sizep);
#include <wchar.h>
FILE *open_wmemstream(wchar_t **bufp, size_t *sizep);
Both return: stream pointer if OK, NULLon error



==================================================================================================
==================================================================================================
#6.<pwd.h>
a.
#include <pwd.h>
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *name);
Both return:pointer if, NULL on error

b.
#include <pwd.h>
struct passwd *getpwent(void);
Returns: pointer if OK, NULLon error or end of file
void setpwent(void);
void endpwent(void);

c.
#include <shadow.h>
struct spwd *getspnam(const char *name);
struct spwd *getspent(void);
Both return: pointer if OK, NULLon error

void setspent(void);
void endspent(void);

d.
#include <grp.h>
struct group *getgrgid(gid_t gid);
struct group *getgrnam(const char *name);
Both return: pointer if OK, NULLon error

e.
#include <grp.h>
struct group *getgrent(void);
Returns: pointer if OK, NULLon error or end of file
void setgrent(void);
void endgrent(void);

f.
#include <sys/utsname.h>
int uname(struct utsname *name);
Returns: non-negative value if OK,−1 on error

g.
#include <unistd.h>
int gethostname(char *name, int namelen);
Returns: 0 if OK,−1 on error

h.The basic time service provided by the kernel counts the number of seconds that
have passed since the Epoch: 00:00:00 January 1, 1970, Coordinated Universal Time
(UTC).
#include <time.h>
time_t time(time_t *calptr);
Returns: value of time if OK,−1 on error

#include <time.h>
struct tm *gmtime(const time_t *calptr);
struct tm *localtime(const time_t *calptr);
Both return: pointer to broken-down time, NULLon error

#include <time.h>
time_t mktime(struct tm *tmptr);
Returns: calendar time if OK,−1 on error

#include <time.h>
size_t strftime(char *restrict buf, size_t maxsize,
const char *restrict format,
const struct tm *restrict tmptr);
size_t strftime_l(char *restrict buf, size_t maxsize,
const char *restrict format,
const struct tm *restrict tmptr, locale_t locale);
Both return: number of characters stored in array if room, 0 otherwise

i.
#include <sys/time.h>
int clock_gettime(clockid_t clock_id, struct timespec *tsp);
Returns: 0 if OK,−1 on error

#include <sys/time.h>
int clock_getres(clockid_t clock_id, struct timespec *tsp);
Returns: 0 if OK,−1 on error

#include <sys/time.h>
int clock_settime(clockid_t clock_id, const struct timespec *tsp);
Returns: 0 if OK,−1 on error

#include <sys/time.h>
int gettimeofday(struct timeval *restrict tp, void *restrict tzp);
Returns: 0 always

Format Description Example
%a abbreviated weekday name Thu
%A full weekday name Thursday
%b abbreviated month name Jan
%B full month name January
%c date and time Thu Jan 19 21:24:52 2012
%C year/100: [00–99] 20
%d day of the month: [01–31] 19
%D date [MM/DD/YY] 01/19/12
%e day of month (single digit preceded by space) [1–31] 19
%F ISO 8601 date format [YYYY–MM–DD] 2012-01-19
%g last two digits of ISO 8601 week-based year [00–99] 12
%G ISO 8601 week-based year 2012
%h same as %b Jan
%H hour of the day (24-hour format): [00–23] 21
%I hour of the day (12-hour format): [01–12] 09
%j day of the year: [001–366] 019
%m month: [01–12] 01
%M minute: [00–59] 24
%n newline character
%p AM/PM PM
%r locale’s time (12-hour format) 09:24:52 PM
%R same as %H:%M 21:24
%S second: [00–60] 52
%t horizontal tab character
%T same as %H:%M:%S 21:24:52
%u ISO 8601 weekday [Monday = 1, 1–7] 4
%U Sunday week number: [00–53] 03
%V ISO 8601 week number: [01–53] 03
%w weekday: [0 = Sunday, 0–6] 4
%W Monday week number: [00–53] 03
%x locale’s date 01/19/12
%X locale’s time 21:24:52
%y last two digits of year: [00–99] 12
%Y year 2012
%z offset from UTC in ISO 8601 format -0500
%Z time zone name EST
%% translates to a percent sign %
==================================================================================================
==================================================================================================
#7.Process Envirenment
a.how the main function is called. C program is executed by the kernel—by one of the execfunctions.
a special start-up routine is called before the main function is called.
int main(int argc, char *argv[]);

b.Process Termination
1. Return from main
2. Calling exit
3. Calling _exit or _Exit
4. Return of the last thread from its start routine
5. Calling pthread_exit from the last thread
Abnormal termination occurs in three ways:
6. Calling abort
7. Receipt of a signal 
8. Response of the last thread to a cancellation request 

#include <stdlib.h>
void exit(int status);
void _Exit(int status);
#include <unistd.h>
void _exit(int status);

c.
#include <stdlib.h>
int atexit(void (*func)(void));

d.
                 FreeBSD Linux Mac OS X Solaris
Variable POSIX.1 8.0 3.2.0 10.6.8 10                 Description
COLUMNS • • • • • terminal width
DATEMSK XSI • • • getdate(3) template file pathname
HOME • • • • • home directory
LANG • • • • • name of locale
LC_ALL • • • • • name of locale
LC_COLLATE • • • • • name of locale for collation
LC_CTYPE • • • • • name of locale for character classification
LC_MESSAGES • • • • • name of locale for messages
LC_MONETARY • • • • • name of locale for monetary editing
LC_NUMERIC • • • • • name of locale for numeric editing
LC_TIME • • • • • name of locale for date/time formatting
LINES • • • • • terminal height
LOGNAME • • • • • login name
MSGVERB XSI • • • • fmtmsg(3) message components to process
NLSPATH • • • • • sequence of templates for message catalogs
PATH • • • • • list of path prefixes to search for executable file
PWD • • • • • absolute pathname of current working directorSHELL • • • • • name of user’s preferred shell
TERM • • • • • terminal type
TMPDIR • • • • • pathname of directory for creating temporary fiTZ • • • • • time zone information


extern char **environ;

e.Memory Allocation
#include <stdlib.h>
void *malloc(size_t size);
void *calloc(size_t nobj, size_t size);
void *realloc(void *ptr, size_t newsize);
All three return: non-null pointer if OK, NULLon error
void free(void *ptr);


e.Environment Variables
#include <stdlib.h>
char *getenv(const char *name);
Returns: pointer to value associated with name, NULLif not found

f.
#include <stdlib.h>
int putenv(char *str);
Returns: 0 if OK, nonzero on error
int setenv(const char *name, const char *value, int rewrite);
int unsetenv(const char *name);
Both return: 0 if OK,−1 on error

g.
#include <setjmp.h>
int setjmp(jmp_buf env);
Returns: 0 if called directly, nonzero if returning from a call to longjmp
void longjmp(jmp_buf env, int val);

i.
#include <sys/resource.h>
int getrlimit(int resource, struct rlimit *rlptr);
int setrlimit(int resource, const struct rlimit *rlptr);
Both return: 0 if OK,−1 on error
==================================================================================================
==================================================================================================
#8.Process Control
a. Process Identifiers
#include <unistd.h>
pid_t getpid(void);
Returns: process ID of calling process
pid_t getppid(void);
Returns: parent process ID of calling process
uid_t getuid(void);
Returns: real user ID of calling process
uid_t geteuid(void);
Returns: effective user ID of calling process
gid_t getgid(void);
Returns: real group ID of calling process
gid_t getegid(void);
Returns: effective group ID of calling process

b.
#include <unistd.h>
pid_t fork(void);
Returns: 0 in child, process ID of child in parent,−1 on error

c.
#include <sys/wait.h>
pid_t wait(int *statloc);
pid_t waitpid(pid_t pid, int *statloc, int options);
Both return: process ID if OK, 0 (see later), or−1 on error

#include <sys/wait.h>
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
Returns: 0 if OK,−1 on error

d.
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
pid_t wait3(int *statloc, int options, struct rusage *rusage);
pid_t wait4(pid_t pid, int *statloc, int options, struct rusage *rusage);
Both return: process ID if OK, 0, or−1 on error

e. exec Functions
#include <unistd.h>
int execl(const char *pathname, const char *arg0, ... /* (char *)0 */ );
int execv(const char *pathname, char *const argv[]);
int execle(const char *pathname, const char *arg0, ...
/* (char *)0, char *const envp[] */ );
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execlp(const char *filename, const char *arg0, ... /* (char *)0 */ );
int execvp(const char *filename, char *const argv[]);
int fexecve(int fd, char *const argv[], char *const envp[]);
All seven return:−1 on error, no return on success

f.
#include <unistd.h>
int setuid(uid_t uid);
int setgid(gid_t gid);
Both return: 0 if OK,−1 on error

g.
#include <unistd.h>
int setreuid(uid_t ruid, uid_t euid);
int setregid(gid_t rgid, gid_t egid);
Both return: 0 if OK,−1 on error

h.system Function
#include <stdlib.h>
int system(const char *cmdstring);
Returns: (see below)

i.
#include <unistd.h>
int nice(int incr);
Returns: new nice value− NZEROif OK,−1 on error

#include <sys/resource.h>
int getpriority(int which, id_t who);
Returns: nice value between−NZEROand NZERO−1if OK,−1 on error

#include <sys/resource.h>
int setpriority(int which, id_t who, int value);
Returns: 0 if OK,−1 on error

j. process time
#include <sys/times.h>
clock_t times(struct tms *buf );
Returns: elapsed wall clock time in clock ticks if OK,−1 on error
==================================================================================================
==================================================================================================
#9.Process relationship


==================================================================================================
==================================================================================================
#10.signals
Name Description ISO C FreeBSD Linux MacOSX Solar   Default action
                        8.0   3.2.0 1 0.6.8  10
SIGABRT abnormal termination (abort) • • • • • • terminate+core
SIGALRM timer expired (alarm) • • • • • terminate
SIGBUS hardware fault • • • • • terminate+core
SIGCANCEL threads library internal use • ignore
SIGCHLD change in status of child • • • • • ignore
SIGCONT continue stopped process • • • • • continue/ignore
SIGEMT hardware fault • • • • terminate+core
SIGFPE arithmetic exception • • • • • • terminate+core
SIGFREEZE checkpoint freeze • ignore
SIGHUP hangup • • • • • terminate
SIGILL illegal instruction • • • • • • terminate+core
SIGINFO status request from keyboard • • ignore
SIGINT terminal interrupt character • • • • • • terminate
SIGIO asynchronous I/O • • • • terminate/ignore
SIGIOT hardware fault • • • • terminate+core
SIGJVM1 Java virtual machine internal use • ignore
SIGJVM2 Java virtual machine internal use • ignore
SIGKILL termination • • • • • terminate
SIGLOST resource lost • terminate
SIGLWP threads library internal use • • terminate/ignore
SIGPIPE write to pipe with no readers • • • • • terminate
SIGPOLL pollable event (poll) • • terminate
SIGPROF profiling time alarm (setitimer) • • • • terminate
SIGPWR power fail/restart • • terminate/ignore
SIGQUIT terminal quit character • • • • • terminate+core
SIGSEGV invalid memory reference • • • • • • terminate+core
SIGSTKFLT coprocessor stack fault • terminate
SIGSTOP stop • • • • • stop process
SIGSYS invalid system call XSI • • • • terminate+core
SIGTERM termination • • • • • • terminate
SIGTHAW checkpoint thaw • ignore
SIGTHR threads library internal use • terminate
SIGTRAP hardware fault XSI • • • • terminate+core
SIGTSTP terminal stop character • • • • • stop process
SIGTTIN background read from control tty • • • • • stop process
SIGTTOU background write to control tty • • • • • stop process
SIGURG urgent condition (sockets) • • • • • ignore
SIGUSR1 user-defined signal • • • • • terminate
SIGUSR2 user-defined signal • • • • • terminate
SIGVTALRM virtual time alarm (setitimer) XSI • • • • terminate
SIGWAITING threads library internal use • ignore
SIGWINCH terminal window size change • • • • ignore
SIGXCPU CPU limit exceeded (setrlimit) XSI • • • • terminate or
terminate+core
SIGXFSZ file size limit exceeded (setrlimit) XSI • • • • terminate or
terminate+core
SIGXRES resource control exceeded • ignore


a.
#include <signal.h>
void (*signal(int signo, void (*func)(int)))(int);
Returns: previous disposition of signal (see following) if OK, SIG_ERRon error

b.
#include <signal.h>
int kill(pid_t pid, int signo);
int raise(int signo);
Both return: 0 if OK,−1 on error

c.
#include <unistd.h>
unsigned int alarm(unsigned int seconds);
Returns: 0 or number of seconds until previously set alarm

#include <unistd.h>
int pause(void);
Returns:−1 with errnoset to EINTR

d.
#include <signal.h>
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
All four return: 0 if OK,−1 on error
int sigismember(const sigset_t *set, int signo);
Returns: 1 if true, 0 if false,−1 on error

e.
#include <signal.h>
int sigprocmask(int how, const sigset_t *restrict set,
sigset_t *restrict oset);
Returns: 0 if OK,−1 on error

f.
#include <signal.h>
int sigpending(sigset_t *set);
Returns: 0 if OK,−1 on error

g.
#include <signal.h>
int sigaction(int signo, const struct sigaction *restrict act,
struct sigaction *restrict oact);
Returns: 0 if OK,−1 on error

h.
#include <setjmp.h>
int sigsetjmp(sigjmp_buf env, int savemask);
Returns: 0 if called directly, nonzero if returning from a call to siglongjmp
void siglongjmp(sigjmp_buf env, int val);

i.
#include <signal.h>
int sigsuspend(const sigset_t *sigmask);
Returns:−1 with errnoset to EINTR

j.
#include <stdlib.h>
void abort(void);
This function never returns

k.
#include <time.h>
int nanosleep(const struct timespec *reqtp, struct timespec *remtp);
Returns: 0 if slept for requested time or−1 on error

#include <time.h>
int clock_nanosleep(clockid_t clock_id, int flags,
const struct timespec *reqtp, struct timespec *remtp);
Returns: 0 if slept for requested time or error number on failure

l.
#include <signal.h>
int sigqueue(pid_t pid, int signo, const union sigval value)
Returns: 0 if OK,−1 on error

m.
#include <signal.h>
void psignal(int signo, const char *msg);

#include <signal.h>
void psiginfo(const siginfo_t *info, const char *msg);

#include <string.h>
char *strsignal(int signo);
Returns: a pointer to a string describing the signal

#include <signal.h>
int sig2str(int signo, char *str);
int str2sig(const char *str, int *signop);
Both return: 0 if OK,−1 on error
==================================================================================================
==================================================================================================
#11.pthread
a.
#include <pthread.h>
int pthread_equal(pthread_t tid1, pthread_t tid2);
Returns: nonzero if equal, 0 otherwise

#include <pthread.h>
pthread_t pthread_self(void);
Returns: the thread ID of the calling thread

b.
#include <pthread.h>
int pthread_create(pthread_t *restrict tidp,
const pthread_attr_t *restrict attr,
void *(*start_rtn)(void *), void *restrict arg);
Returns: 0 if OK, error number on failure

c.
#include <pthread.h>
void pthread_exit(void *rval_ptr);

#include <pthread.h>
int pthread_join(pthread_t thread, void **rval_ptr);
Returns: 0 if OK, error number on failure

#include <pthread.h>
int pthread_cancel(pthread_t tid);
Returns: 0 if OK, error number on failure

#include <pthread.h>
void pthread_cleanup_push(void (*rtn)(void *), void *arg);
void pthread_cleanup_pop(int execute);

#include <pthread.h>
int pthread_detach(pthread_t tid);
Returns: 0 if OK, error number on failure

d.
#include <pthread.h>
int pthread_mutex_init(pthread_mutex_t *restrict mutex,
const pthread_mutexattr_t *restrict attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
All return: 0 if OK, error number on failure

e.
#include <pthread.h>
#include <time.h>
int pthread_mutex_timedlock(pthread_mutex_t *restrict mutex,
const struct timespec *restrict tsptr);
Returns: 0 if OK, error number on failure

f.
#include <pthread.h>
int pthread_rwlock_init(pthread_rwlock_t *restrict rwlock,
const pthread_rwlockattr_t *restrict attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
All return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
Both return: 0 if OK, error number on failure

g.
#include <pthread.h>
#include <time.h>
int pthread_rwlock_timedrdlock(pthread_rwlock_t *restrict rwlock,
const struct timespec *restrict tsptr);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *restrict rwlock,
const struct timespec *restrict tsptr);
Both return: 0 if OK, error number on failure

h.
#include <pthread.h>
int pthread_cond_init(pthread_cond_t *restrict cond,
const pthread_condattr_t *restrict attr);
int pthread_cond_destroy(pthread_cond_t *cond);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_cond_wait(pthread_cond_t *restrict cond,
pthread_mutex_t *restrict mutex);
int pthread_cond_timedwait(pthread_cond_t *restrict cond,
pthread_mutex_t *restrict mutex,
const struct timespec *restrict tsptr);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
Both return: 0 if OK, error number on failure

i.
#include <pthread.h>
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_destroy(pthread_spinlock_t *lock);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);
All return: 0 if OK, error number on failure

j.
#include <pthread.h>
int pthread_barrier_init(pthread_barrier_t *restrict barrier,
const pthread_barrierattr_t *restrict attr,
unsigned int count);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_barrier_wait(pthread_barrier_t *barrier);
Returns: 0 or PTHREAD_BARRIER_SERIAL_THREADif OK, error number on failure


==================================================================================================
==================================================================================================
#12.thread control
a.
#include <pthread.h>
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_attr_getdetachstate(const pthread_attr_t *restrict attr,
int *detachstate);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_attr_getstack(const pthread_attr_t *restrict attr,
void **restrict stackaddr,
size_t *restrict stacksize);
int pthread_attr_setstack(pthread_attr_t *attr,
void *stackaddr, size_t stacksize);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_attr_getstacksize(const pthread_attr_t *restrict attr,
size_t *restrict stacksize);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_attr_getguardsize(const pthread_attr_t *restrict attr,
size_t *restrict guardsize);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
Both return: 0 if OK, error number on failure

b.
#include <pthread.h>
int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_mutexattr_getpshared(const pthread_mutexattr_t *
restrict attr,
int *restrict pshared);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr,
int pshared);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_mutexattr_getrobust(const pthread_mutexattr_t *
restrict attr,
int *restrict robust);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr,
int robust);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_mutex_consistent(pthread_mutex_t * mutex);
Returns: 0 if OK, error number on failure

#include <pthread.h>
int pthread_mutexattr_gettype(const pthread_mutexattr_t *
restrict attr, int *restrict type);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
Both return: 0 if OK, error number on failure

c.
#include <pthread.h>
int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *
restrict attr,
int *restrict pshared);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr,
int pshared);
Both return: 0 if OK, error number on failure

d.
include <pthread.h>
int pthread_condattr_init(pthread_condattr_t *attr);
int pthread_condattr_destroy(pthread_condattr_t *attr);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_condattr_getpshared(const pthread_condattr_t *
restrict attr,
int *restrict pshared);
int pthread_condattr_setpshared(pthread_condattr_t *attr,
int pshared);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_condattr_getclock(const pthread_condattr_t *
restrict attr,
clockid_t *restrict clock_id);
int pthread_condattr_setclock(pthread_condattr_t *attr,
clockid_t clock_id);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_barrierattr_init(pthread_barrierattr_t *attr);
int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
Both return: 0 if OK, error number on failure

#include <pthread.h>
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *
restrict attr,
int *restrict pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr,
int pshared);
Both return: 0 if OK, error number on failure

#include <stdio.h>
int ftrylockfile(FILE *fp);
Returns: 0 if OK, nonzero if lock can’t be acquired
void flockfile(FILE *fp);
void funlockfile(FILE *fp);

#include <stdio.h>
int getchar_unlocked(void);
int getc_unlocked(FILE *fp);
int putchar_unlocked(int c);
int putc_unlocked(int c, FILE *fp);
Both return: the next character if OK, EOFon end of file or error
Both return: c if OK, EOFon error

#include <pthread.h>
int pthread_key_create(pthread_key_t *keyp, void (*destructor)(void *));
Returns: 0 if OK, error number on failure

#include <pthread.h>
int pthread_key_delete(pthread_key_t key);
Returns: 0 if OK, error number on failure

#include <pthread.h>
pthread_once_t initflag = PTHREAD_ONCE_INIT;
int pthread_once(pthread_once_t *initflag, void (*initfn)(void));
Returns: 0 if OK, error number on failure

#include <pthread.h>
void *pthread_getspecific(pthread_key_t key);
Returns: thread-specific data value or NULLif no value
has been associated with the key
int pthread_setspecific(pthread_key_t key, const void *value);
Returns: 0 if OK, error number on failure

#include <pthread.h>
int pthread_setcancelstate(int state, int *oldstate);
Returns: 0 if OK, error number on failure

#include <pthread.h>
void pthread_testcancel(void);

#include <pthread.h>
int pthread_setcanceltype(int type, int *oldtype);
Returns: 0 if OK, error number on failure

#include <signal.h>
int pthread_sigmask(int how, const sigset_t *restrict set,
sigset_t *restrict oset);
Returns: 0 if OK, error number on failure

#include <signal.h>
int sigwait(const sigset_t *restrict set, int *restrict signop);
Returns: 0 if OK, error number on failure

#include <signal.h>
int pthread_kill(pthread_t thread, int signo);
Returns: 0 if OK, error number on failure

#include <pthread.h>
int pthread_atfork(void (*prepare)(void), void (*parent)(void),
void (*child)(void));
Returns: 0 if OK, error number on failure



==================================================================================================
==================================================================================================
#13.Daemmon processes


==================================================================================================
==================================================================================================
#14.advanced I/O : nonblocking I/O, record locking, I/O multiplexing (the select and
poll functions), asynchronous I/O, the readv and writev functions, and memory-mapped I/O (mmap).
a.
#include <fcntl.h>
int fcntl(int fd, int cmd, ... /* struct flock *flockptr */ );
Returns: depends on cmd if OK (see following),−1 on error

==================================================================================================
==================================================================================================
#15.Inter process Communication
IPC type SUS FreeBSD Linux  Mac OSX   Solaris
               8.0   3.2.0  10.6.8    10
half-duplex pipes • (full) • • (full)
FIFOs • • • • •
full-duplex pipes allowed •, UDS UDS UDS •, UDS
named full-duplex pipes obsolescent UDS UDS UDS •, UDS
XSI message queues XSI • • • •
XSI semaphores XSI • • • •
XSI shared memory XSI • • • •
message queues (real-time) MSG option • • •
semaphores • • • • •
shared memory (real-time) SHM option • • • •
sockets • • • • •
STREAMS obsolescent •
a.
#include <unistd.h>
int pipe(int fd[2]);
Returns: 0 if OK,−1 on error

b.
#include <stdio.h>
FILE *popen(const char *cmdstring, const char *type);
Returns: file pointer if OK, NULLon error
int pclose(FILE *fp);
Returns: termination status of cmdstring, or−1 on error

c.
#include <sys/stat.h>
int mkfifo(const char *path, mode_t mode);
int mkfifoat(int fd, const char *path, mode_t mode);
Both return: 0 if OK,−1 on error

d.
#include <sys/ipc.h>
key_t ftok(const char *path, int id);
Returns: key if OK, (key_t)−1 on erro

e.
#include <sys/msg.h>
int msgget(key_t key, int flag);
Returns: message queue ID if OK,−1 on error

#include <sys/msg.h>
int msgctl(int msqid, int cmd, struct msqid_ds *buf );
Returns: 0 if OK,−1 on error

#include <sys/msg.h>
int msgsnd(int msqid, const void *ptr, size_t nbytes, int flag);
Returns: 0 if OK,−1 on error

#include <sys/msg.h>
ssize_t msgrcv(int msqid, void *ptr, size_t nbytes, long type, int flag);
Returns: size of data portion of message if OK,−1 on error

f.
#include <sys/sem.h>
int semget(key_t key, int nsems, int flag);
Returns: semaphore ID if OK,−1 on error

#include <sys/sem.h>
int semctl(int semid, int semnum, int cmd, ... /* union semun arg */ );
Returns: (see following)

#include <sys/sem.h>
int semop(int semid, struct sembuf semoparray[], size_t nops);
Returns: 0 if OK,−1 on error

g.
#include <sys/shm.h>
int shmget(key_t key, size_t size, int flag);
Returns: shared memory ID if OK,−1 on error

#include <sys/shm.h>
int shmctl(int shmid, int cmd, struct shmid_ds *buf );
Returns: 0 if OK,−1 on error

#include <sys/shm.h>
void *shmat(int shmid, const void *addr, int flag);
Returns: pointer to shared memory segment if OK,−1 on error

#include <sys/shm.h>
int shmdt(const void *addr);
Returns: 0 if OK,−1 on error

h.
#include <semaphore.h>
sem_t *sem_open(const char *name, int oflag, ... /* mode_t mode,
unsigned int value */ );
Returns: Pointer to semaphore if OK, SEM_FAILEDon error

#include <semaphore.h>
int sem_close(sem_t *sem);
Returns: 0 if OK,−1 on error

#include <semaphore.h>
int sem_unlink(const char *name);
Returns: 0 if OK,−1 on error

#include <semaphore.h>
int sem_trywait(sem_t *sem);
int sem_wait(sem_t *sem);
Both return: 0 if OK,−1 on error

#include <semaphore.h>
#include <time.h>
int sem_timedwait(sem_t *restrict sem,
const struct timespec *restrict tsptr);
Returns: 0 if OK,−1 on error

#include <semaphore.h>
int sem_post(sem_t *sem);
Returns: 0 if OK,−1 on error

#include <semaphore.h>
int sem_init(sem_t *sem, int pshared, unsigned int value);
Returns: 0 if OK,−1 on error

#include <semaphore.h>
int sem_destroy(sem_t *sem);
Returns: 0 if OK,−1 on error

#include <semaphore.h>
int sem_getvalue(sem_t *restrict sem, int *restrict valp);
Returns: 0 if OK,−1 on error


==================================================================================================
==================================================================================================
#16.Network IPC: Sockets

a.Socket Descriptors
#include <sys/socket.h>
int socket(int domain, int type, int protocol);
Returns: file (socket) descriptor if OK,−1 on error

#include <sys/socket.h>
int shutdown(int sockfd, int how);
Returns: 0 if OK,−1 on error

b.
#include <arpa/inet.h>
uint32_t htonl(uint32_t hostint32);
Returns: 32-bit integer in network byte order
uint16_t htons(uint16_t hostint16);
Returns: 16-bit integer in network byte order
uint32_t ntohl(uint32_t netint32);
Returns: 32-bit integer in host byte order
uint16_t ntohs(uint16_t netint16);
Returns: 16-bit integer in host byte order

#include <arpa/inet.h>
const char *inet_ntop(int domain, const void *restrict addr,
char *restrict str, socklen_t size);
Returns: pointer to address string on success, NULLon error
int inet_pton(int domain, const char *restrict str,
void *restrict addr);
Returns: 1 on success, 0 if the format is invalid, or−1 on error

c.
#include <netdb.h>
struct hostent *gethostent(void);
Returns: pointer if OK, NULLon error
void sethostent(int stayopen);
void endhostent(void);

#include <netdb.h>
struct netent *getnetbyaddr(uint32_t net, int type);
struct netent *getnetbyname(const char *name);
struct netent *getnetent(void);
All return: pointer if OK, NULLon error
void setnetent(int stayopen);
void endnetent(void);

#include <netdb.h>
struct protoent *getprotobyname(const char *name);
struct protoent *getprotobynumber(int proto);
struct protoent *getprotoent(void);
All return: pointer if OK, NULLon error
void setprotoent(int stayopen);
void endprotoent(void);

#include <netdb.h>
struct servent *getservbyname(const char *name, const char *proto);
struct servent *getservbyport(int port, const char *proto);
struct servent *getservent(void);
All return: pointer if OK, NULLon error
void setservent(int stayopen);
void endservent(void);

#include <sys/socket.h>
#include <netdb.h>
int getaddrinfo(const char *restrict host,
const char *restrict service,
const struct addrinfo *restrict hint,
struct addrinfo **restrict res);
Returns: 0 if OK, nonzero error code on error
void freeaddrinfo(struct addrinfo *ai);

#include <netdb.h>
const char *gai_strerror(int error);
Returns: a pointer to a string describing the error

#include <sys/socket.h>
#include <netdb.h>
int getnameinfo(const struct sockaddr *restrict addr, socklen_t alen,
char *restrict host, socklen_t hostlen,
char *restrict service, socklen_t servlen, int flags);
Returns: 0 if OK, nonzero on erro

d.
#include <sys/socket.h>
int bind(int sockfd, const struct sockaddr *addr, socklen_t len);
Returns: 0 if OK,−1 on error

#include <sys/socket.h>
int getsockname(int sockfd, struct sockaddr *restrict addr,
socklen_t *restrict alenp);
Returns: 0 if OK,−1 on error

#include <sys/socket.h>
int getpeername(int sockfd, struct sockaddr *restrict addr,
socklen_t *restrict alenp);
Returns: 0 if OK,−1 on error

#include <sys/socket.h>
int connect(int sockfd, const struct sockaddr *addr, socklen_t len);
Returns: 0 if OK,−1 on error

#include <sys/socket.h>
int listen(int sockfd, int backlog);
Returns: 0 if OK,−1 on error

#include <sys/socket.h>
int accept(int sockfd, struct sockaddr *restrict addr,
socklen_t *restrict len);
Returns: file (socket) descriptor if OK,−1 on error

#include <sys/socket.h>
ssize_t send(int sockfd, const void *buf, size_t nbytes, int flags);
Returns: number of bytes sent if OK,−1 on error

#include <sys/socket.h>
ssize_t sendto(int sockfd, const void *buf, size_t nbytes, int flags,
const struct sockaddr *destaddr, socklen_t destlen);
Returns: number of bytes sent if OK,−1 on error

#include <sys/socket.h>
ssize_t recv(int sockfd, void *buf, size_t nbytes, int flags);
Returns: length of message in bytes,
0 if no messages are available and peer has done an orderly shutdown,
or−1 on error

#include <sys/socket.h>
ssize_t recvfrom(int sockfd, void *restrict buf, size_t len, int flags,
struct sockaddr *restrict addr,
socklen_t *restrict addrlen);
Returns: length of message in bytes,
0 if no messages are available and peer has done an orderly shutdown,
or−1 on error

#include <sys/socket.h>
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
Returns: length of message in bytes,
0 if no messages are available and peer has done an orderly shutdown,
or−1 on error


#include <sys/socket.h>
int setsockopt(int sockfd, int level, int option, const void *val,
socklen_t len);
Returns: 0 if OK,−1 on error

#include <sys/socket.h>
int getsockopt(int sockfd, int level, int option, void *restrict val,
socklen_t *restrict lenp);
Returns: 0 if OK,−1 on error

#include <sys/socket.h>
int sockatmark(int sockfd);
Returns: 1 if at mark, 0 if not at mark,−1 on error
==================================================================================================
==================================================================================================
#17.Advanced IPC
a.
#include <sys/socket.h>
int socketpair(int domain, int type, int protocol, int sockfd[2]);
Returns: 0 if OK,−1 on erro

b.
int serv_listen(const char *name);
Returns: file descriptor to listen on if OK, negative value on error
int serv_accept(int listenfd, uid_t *uidptr);
Returns: new file descriptor if OK, negative value on error
int cli_conn(const char *name);
Returns: file descriptor if OK, negative value on error

c.
int send_fd(int fd, int fd_to_send);
int send_err(int fd, int status, const char *errmsg);
Both return: 0 if OK,−1 on error
int recv_fd(int fd, ssize_t (*userfunc)(int, const void *, size_t));
Returns: file descriptor if OK, negative value on error

d.
#include <sys/socket.h>
unsigned char *CMSG_DATA(struct cmsghdr *cp);
Returns: pointer to data associated with cmsghdrstructure
struct cmsghdr *CMSG_FIRSTHDR(struct msghdr *mp);
Returns: pointer to first cmsghdrstructure associated
with the msghdrstructure, or NULLif none exists
struct cmsghdr *CMSG_NXTHDR(struct msghdr *mp,
struct cmsghdr *cp);
Returns: pointer to next cmsghdrstructure associated with
the msghdrstructure given the current cmsghdr
structure, or NULLif we’re at the last one
unsigned int CMSG_LEN(unsigned int nbytes);
Returns: size to allocate for data object nbytes large
==================================================================================================
==================================================================================================
#18.Terminal
a.


==================================================================================================
==================================================================================================
#19.Pseudo Terminals


==================================================================================================
==================================================================================================
20.A Database Library