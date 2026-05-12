#include "uix_stdio.h"
#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_errno.h"
#include "uix_unistd.h"
#include "uix_fcntl.h"
#include "uix_ctype.h"

static uix_FILE _sin  = {UIX_STDIN_FILENO,  0,0,0,NULL,0,0,0};
static uix_FILE _sout = {UIX_STDOUT_FILENO, 0,0,0,NULL,0,0,0};
static uix_FILE _serr = {UIX_STDERR_FILENO, 0,0,0,NULL,0,0,0};

uix_FILE *uix_stdin  = &_sin;
uix_FILE *uix_stdout = &_sout;
uix_FILE *uix_stderr = &_serr;

uix_FILE *uix_fopen(const char *path, const char *mode)
{
    int fl=0;
    if (!uix_strcmp(mode,"r"))       fl=UIX_O_RDONLY;
    else if (!uix_strcmp(mode,"w"))  fl=UIX_O_WRONLY|UIX_O_CREAT|UIX_O_TRUNC;
    else if (!uix_strcmp(mode,"a"))  fl=UIX_O_WRONLY|UIX_O_CREAT|UIX_O_APPEND;
    else if (!uix_strcmp(mode,"r+")) fl=UIX_O_RDWR;
    else if (!uix_strcmp(mode,"w+")) fl=UIX_O_RDWR|UIX_O_CREAT|UIX_O_TRUNC;
    else if (!uix_strcmp(mode,"a+")) fl=UIX_O_RDWR|UIX_O_CREAT|UIX_O_APPEND;
    else { uix_errno=UIX_EINVAL; return NULL; }

    int fd=uix_open(path,fl,0644);
    if (fd<0) return NULL;

    uix_FILE *fp=(uix_FILE*)uix_malloc(sizeof(uix_FILE));
    if (!fp){ uix_close(fd); return NULL; }
    fp->fd=fd; fp->flags=fl; fp->error=fp->eof=0;
    fp->buffer=(char*)uix_malloc(UIX_BUFSIZ);
    fp->buf_size=UIX_BUFSIZ; fp->buf_pos=fp->buf_len=0;
    return fp;
}

int uix_fclose(uix_FILE *s)
{
    if (!s) return UIX_EOF;
    uix_fflush(s);
    int r=uix_close(s->fd);
    uix_free(s->buffer); uix_free(s);
    return r;
}

int uix_fflush(uix_FILE *s)
{
    if (!s||!s->buffer||!s->buf_len) return 0;
    uix_write(s->fd,s->buffer,s->buf_len);
    s->buf_pos=s->buf_len=0;
    return 0;
}

int uix_fputc(int c, uix_FILE *s)
{
    if (!s) return UIX_EOF;
    if (!s->buffer){ char ch=(char)c; return uix_write(s->fd,&ch,1)==1?c:UIX_EOF; }
    if (s->buf_len>=s->buf_size) uix_fflush(s);
    s->buffer[s->buf_len++]=(char)c;
    if (c=='\n') uix_fflush(s);
    return c;
}
int uix_putchar(int c) { return uix_fputc(c,uix_stdout); }

int uix_fgetc(uix_FILE *s)
{
    if (!s) return UIX_EOF;
    char c;
    if (uix_read(s->fd,&c,1)!=1){ s->eof=1; return UIX_EOF; }
    return (unsigned char)c;
}
int uix_getchar(void) { return uix_fgetc(uix_stdin); }

int uix_ungetc(int c, uix_FILE *s)
{
    (void)s; (void)c; return UIX_EOF; /* simplified */
}

char *uix_fgets(char *buf, int n, uix_FILE *s)
{
    if (!buf||n<=0||!s) return NULL;
    int i=0;
    while (i<n-1){
        int c=uix_fgetc(s);
        if (c==UIX_EOF){ if(!i) return NULL; break; }
        buf[i++]=(char)c;
        if (c=='\n') break;
    }
    buf[i]='\0'; return buf;
}

int uix_fputs(const char *s, uix_FILE *f)
{
    if (!s||!f) return UIX_EOF;
    uix_size_t l=uix_strlen(s);
    return uix_write(f->fd,s,l)==(uix_ssize_t)l?0:UIX_EOF;
}
int uix_puts(const char *s){ uix_fputs(s,uix_stdout); uix_fputc('\n',uix_stdout); return 0; }

uix_size_t uix_fread(void *p, uix_size_t sz, uix_size_t n, uix_FILE *s)
{
    if (!p||!s||!sz) return 0;
    uix_ssize_t r=uix_read(s->fd,p,sz*n);
    if (r<=0){ s->eof=1; return 0; }
    return (uix_size_t)r/sz;
}
uix_size_t uix_fwrite(const void *p, uix_size_t sz, uix_size_t n, uix_FILE *s)
{
    if (!p||!s||!sz) return 0;
    uix_ssize_t r=uix_write(s->fd,p,sz*n);
    return r<=0?0:(uix_size_t)r/sz;
}

int uix_fseek(uix_FILE *s, uix_off_t off, int w)
{
    if (!s) return -1;
    uix_fflush(s);
    return uix_lseek(s->fd,off,w)<0?-1:0;
}
uix_off_t uix_ftell(uix_FILE *s){ return s?uix_lseek(s->fd,0,UIX_SEEK_CUR):-1; }
void uix_rewind(uix_FILE *s){ if(s){uix_fseek(s,0,UIX_SEEK_SET);s->error=0;} }

void uix_clearerr(uix_FILE *s){ if(s){s->error=s->eof=0;} }
int  uix_feof   (uix_FILE *s){ return s?s->eof:0; }
int  uix_ferror (uix_FILE *s){ return s?s->error:0; }
void uix_perror (const char *s)
{
    if (s&&*s){ uix_fputs(s,uix_stderr); uix_fputs(": ",uix_stderr); }
    uix_fputs(uix_strerror(uix_errno),uix_stderr);
    uix_fputc('\n',uix_stderr);
}

int uix_vsnprintf(char *buf, uix_size_t sz,
                  const char *fmt, uix_va_list ap)
{
    uix_size_t pos=0;
#define O(c) do{ if(pos<sz-1) buf[pos]=(c); pos++; }while(0)
    while (*fmt) {
        if (*fmt!='%'){ O(*fmt++); continue; }
        fmt++;
        int left=0,zero=0,width=0; char pad=' ';
        while (*fmt=='-'||*fmt=='0'){
            if(*fmt=='-') left=1; else zero=1; fmt++;
        }
        while (*fmt>='0'&&*fmt<='9') width=width*10+(*fmt++)-'0';
        pad=zero&&!left?'0':' ';
        int ll=0,lng=0;
        if(*fmt=='l'){fmt++;lng=1; if(*fmt=='l'){fmt++;ll=1;}}
        char tmp[64]; const char *p=tmp; uix_size_t pl=0;
        switch(*fmt++){
        case 'd': case 'i':{
            long long v=ll?uix_va_arg(ap,long long):lng?uix_va_arg(ap,long):(long long)uix_va_arg(ap,int);
            int neg=v<0; if(neg) v=-v;
            int i=63; tmp[i]='\0';
            do{tmp[--i]='0'+(int)(v%10);v/=10;}while(v);
            if(neg) tmp[--i]='-';
            p=&tmp[i]; pl=uix_strlen(p); break;}
        case 'u':{
            unsigned long long v=ll?uix_va_arg(ap,unsigned long long):lng?uix_va_arg(ap,unsigned long):(unsigned long long)uix_va_arg(ap,unsigned int);
            int i=63; tmp[i]='\0';
            do{tmp[--i]='0'+(int)(v%10);v/=10;}while(v);
            p=&tmp[i]; pl=uix_strlen(p); break;}
        case 'x': case 'X':{
            unsigned long long v=ll?uix_va_arg(ap,unsigned long long):lng?uix_va_arg(ap,unsigned long):(unsigned long long)uix_va_arg(ap,unsigned int);
            const char *hx=*(fmt-1)=='x'?"0123456789abcdef":"0123456789ABCDEF";
            int i=63; tmp[i]='\0';
            if(!v) tmp[--i]='0'; else while(v){tmp[--i]=hx[v&0xf];v>>=4;}
            p=&tmp[i]; pl=uix_strlen(p); break;}
        case 'c':{tmp[0]=(char)uix_va_arg(ap,int);tmp[1]='\0';p=tmp;pl=1;break;}
        case 's':{p=uix_va_arg(ap,const char*);if(!p)p="(null)";pl=uix_strlen(p);break;}
        case 'p':{
            uix_uintptr_t v=(uix_uintptr_t)uix_va_arg(ap,void*);
            int i=63; tmp[i]='\0';
            if(!v)tmp[--i]='0'; else while(v){tmp[--i]="0123456789abcdef"[v&0xf];v>>=4;}
            tmp[--i]='x'; tmp[--i]='0';
            p=&tmp[i]; pl=uix_strlen(p); break;}
        case '%': O('%'); continue;
        default:  O(*(fmt-1)); continue;
        }
        if (!left) for (uix_size_t i=pl;(int)i<width;i++) O(pad);
        for (uix_size_t i=0;i<pl;i++) O(p[i]);
        if (left)  for (uix_size_t i=pl;(int)i<width;i++) O(' ');
    }
    if (sz>0) buf[pos<sz?pos:sz-1]='\0';
    return (int)pos;
#undef O
}

int uix_vprintf(const char *fmt, uix_va_list ap)
{
    char buf[4096];
    int n=uix_vsnprintf(buf,sizeof(buf),fmt,ap);
    if (n>0) uix_write(UIX_STDOUT_FILENO,buf,(uix_size_t)n);
    return n;
}
int uix_vfprintf(uix_FILE *s, const char *fmt, uix_va_list ap)
{
    char buf[4096];
    int n=uix_vsnprintf(buf,sizeof(buf),fmt,ap);
    if (n>0) uix_write(s->fd,buf,(uix_size_t)n);
    return n;
}
int uix_vsprintf(char *s, const char *fmt, uix_va_list ap)
{ return uix_vsnprintf(s,(uix_size_t)-1,fmt,ap); }

int uix_printf(const char *fmt,...){ uix_va_list a; uix_va_start(a,fmt); int n=uix_vprintf(fmt,a); uix_va_end(a); return n; }
int uix_fprintf(uix_FILE *s,const char *fmt,...){ uix_va_list a; uix_va_start(a,fmt); int n=uix_vfprintf(s,fmt,a); uix_va_end(a); return n; }
int uix_sprintf(char *s,const char *fmt,...){ uix_va_list a; uix_va_start(a,fmt); int n=uix_vsprintf(s,fmt,a); uix_va_end(a); return n; }

int uix_snprintf(char *s,uix_size_t sz,const char *fmt,...)
{ 
    uix_va_list a; uix_va_start(a,fmt); 
    int n=uix_vsnprintf(s,sz,fmt,a); 
    uix_va_end(a); return n; 
}

int uix_sscanf(const char *str, const char *fmt, ...)
{
    uix_va_list ap; uix_va_start(ap,fmt);
    int cnt=0; const char *s=str;
    while (*fmt){
        if (*fmt!='%'){ if(*fmt!=*s) break; fmt++; s++; continue; }
        fmt++;
        switch(*fmt++){
        case 'd':{ while(uix_isspace((unsigned char)*s)) s++;
                   int *d=uix_va_arg(ap,int*); *d=(int)uix_strtol(s,(char**)&s,10); cnt++; break;}
        case 's':{ while(uix_isspace((unsigned char)*s)) s++;
                   char *d=uix_va_arg(ap,char*);
                   while(*s&&!uix_isspace((unsigned char)*s)) *d++=*s++;
                   *d='\0'; cnt++; break;}
        }
    }
    uix_va_end(ap); return cnt;
}
int uix_scanf(const char *fmt,...){
    char buf[1024]; uix_fgets(buf,sizeof(buf),uix_stdin);
    uix_va_list a; uix_va_start(a,fmt);
    int n=uix_sscanf(buf,fmt,a); uix_va_end(a); return n;
}

/* ***This is End of file, there is no more line should be added after this line*** */
