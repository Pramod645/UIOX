#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_errno.h"
#include "uix_ctype.h"

/* ── Heap ─────────────────────────────────────────────────── */
#define HEAP_SIZE (8*1024*1024)
static unsigned char _heap[HEAP_SIZE];
typedef struct chunk { uix_size_t sz; int free; struct chunk *next; } chunk_t;
static chunk_t *_head = NULL;

static void _heap_init(void)
{
    _head = (chunk_t*)_heap;
    _head->sz   = HEAP_SIZE - sizeof(chunk_t);
    _head->free = 1;
    _head->next = NULL;
}

void *uix_malloc(uix_size_t sz)
{
    if (!_head) _heap_init();
    if (!sz) return NULL;
    sz = (sz+7)&~(uix_size_t)7;
    for (chunk_t *c = _head; c; c = c->next) {
        if (!c->free || c->sz < sz) continue;
        if (c->sz >= sz + sizeof(chunk_t) + 8) {
            chunk_t *n = (chunk_t*)((unsigned char*)c+sizeof(chunk_t)+sz);
            n->sz   = c->sz - sz - sizeof(chunk_t);
            n->free = 1;
            n->next = c->next;
            c->next = n;
            c->sz   = sz;
        }
        c->free = 0;
        return (void*)((unsigned char*)c + sizeof(chunk_t));
    }
    uix_errno = UIX_ENOMEM; return NULL;
}

void *uix_calloc(uix_size_t n, uix_size_t sz)
{
    void *p = uix_malloc(n*sz);
    if (p) uix_memset(p, 0, n*sz);
    return p;
}

void *uix_realloc(void *ptr, uix_size_t sz)
{
    if (!ptr) return uix_malloc(sz);
    if (!sz)  { uix_free(ptr); return NULL; }
    chunk_t *c = (chunk_t*)((unsigned char*)ptr - sizeof(chunk_t));
    if (c->sz >= sz) return ptr;
    void *np = uix_malloc(sz);
    if (!np) return NULL;
    uix_memcpy(np, ptr, c->sz);
    uix_free(ptr);
    return np;
}

void uix_free(void *ptr)
{
    if (!ptr) return;
    chunk_t *c = (chunk_t*)((unsigned char*)ptr - sizeof(chunk_t));
    c->free = 1;
    for (chunk_t *p = _head; p && p->next; p = p->next)
        if (p->free && p->next->free) {
            p->sz  += sizeof(chunk_t) + p->next->sz;
            p->next = p->next->next;
        }
}

/* ── atexit ──────────────────────────────────────────────── */
static void (*_atexit[64])(void);
static int   _atexit_n = 0;

int  uix_atexit(void (*fn)(void)) { if (_atexit_n>=64) return -1; _atexit[_atexit_n++]=fn; return 0; }
#if 0// UIX_EXIT_IN_UINSTD
//void uix_exit  (int s) { for (int i=_atexit_n-1;i>=0;i--) _atexit[i](); (void)s; while(1){} } // duplicatie of uix_uinstd.c
void uix_exit(int status)
{
    sys_exit(status);
    __builtin_unreachable();   /* fixes -Wno-invalid-noreturn too */
}
#endif

void uix_abort (void)  { while(1){} }
int  uix_system(const char *c) { (void)c; return -1; }

/* ── env ─────────────────────────────────────────────────── */
static char *_env[256]; static int _envc=0;

char *uix_getenv(const char *name)
{
    uix_size_t l = uix_strlen(name);
    for (int i=0;i<_envc;i++)
        if (!uix_strncmp(_env[i],name,l) && _env[i][l]=='=') return _env[i]+l+1;
    return NULL;
}
int uix_setenv(const char *n, const char *v, int ow)
{
    uix_size_t nl=uix_strlen(n), vl=uix_strlen(v);
    for (int i=0;i<_envc;i++)
        if (!uix_strncmp(_env[i],n,nl) && _env[i][nl]=='=') {
            if (!ow) return 0;
            char *e = (char*)uix_malloc(nl+1+vl+1);
            if (!e) return -1;
            uix_memcpy(e,n,nl); e[nl]='='; uix_memcpy(e+nl+1,v,vl+1);
            uix_free(_env[i]); _env[i]=e; return 0;
        }
    if (_envc>=256) return -1;
    char *e=(char*)uix_malloc(nl+1+vl+1);
    if (!e) return -1;
    uix_memcpy(e,n,nl); e[nl]='='; uix_memcpy(e+nl+1,v,vl+1);
    _env[_envc++]=e; return 0;
}
int uix_unsetenv(const char *n)
{
    uix_size_t l=uix_strlen(n);
    for (int i=0;i<_envc;i++)
        if (!uix_strncmp(_env[i],n,l) && _env[i][l]=='=') {
            uix_free(_env[i]); _env[i]=_env[--_envc]; return 0; }
    return 0;
}
int uix_putenv(char *s)
{
    char *eq=uix_strchr(s,'=');
    if (!eq) return -1;
    *eq='\0'; int r=uix_setenv(s,eq+1,1); *eq='='; return r;
}

/* ── conversions ─────────────────────────────────────────── */
int  uix_atoi(const char *s) { return (int)uix_strtol(s,NULL,10); }
long uix_atol(const char *s) { return uix_strtol(s,NULL,10); }
double uix_atof(const char *s) { return uix_strtod(s,NULL); }

long uix_strtol(const char *s, char **ep, int base)
{
    long r=0; int sg=1;
    while (uix_isspace((unsigned char)*s)) s++;
    if (*s=='-'){sg=-1;s++;} else if (*s=='+') s++;
    if (base==0) {
        if (*s=='0'&&(s[1]=='x'||s[1]=='X')){base=16;s+=2;}
        else if (*s=='0'){base=8;s++;}
        else base=10;
    }
    while (*s) {
        int d;
        if (*s>='0'&&*s<='9') d=*s-'0';
        else if (*s>='a'&&*s<='f') d=*s-'a'+10;
        else if (*s>='A'&&*s<='F') d=*s-'A'+10;
        else break;
        if (d>=base) break;
        r=r*base+d; s++;
    }
    if (ep) *ep=(char*)s;
    return sg*r;
}
unsigned long uix_strtoul(const char *s, char **ep, int base)
{ return (unsigned long)uix_strtol(s,ep,base); }
double uix_strtod(const char *s, char **ep)
{
    double r=0,f=1; int sg=1;
    while (uix_isspace((unsigned char)*s)) s++;
    if (*s=='-'){sg=-1;s++;} else if (*s=='+') s++;
    while (*s>='0'&&*s<='9') r=r*10+(*s++-'0');
    if (*s=='.'){s++; while(*s>='0'&&*s<='9'){f/=10;r+=(*s++-'0')*f;}}
    if (ep) *ep=(char*)s;
    return sg*r;
}

int  uix_abs (int  x) { return x<0?-x:x; }
long uix_labs(long x) { return x<0?-x:x; }

static unsigned long _rseed=1;
int  uix_rand (void) { _rseed=_rseed*1103515245UL+12345UL; return (int)((_rseed>>16)&0x7FFF); }
void uix_srand(unsigned int s) { _rseed=s; }

void uix_qsort(void *base, uix_size_t n, uix_size_t sz,
               int(*cmp)(const void*,const void*))
{
    if (n<2) return;
    unsigned char *a=(unsigned char*)base;
    unsigned char *tmp=(unsigned char*)uix_malloc(sz);
    if (!tmp) return;
    for (uix_size_t i=1;i<n;i++){
        uix_memcpy(tmp,a+i*sz,sz);
        uix_ssize_t j=(uix_ssize_t)i-1;
        while (j>=0 && cmp(a+j*sz,tmp)>0) {
            uix_memcpy(a+(j+1)*sz,a+j*sz,sz); j--;
        }
        uix_memcpy(a+(j+1)*sz,tmp,sz);
    }
    uix_free(tmp);
}

void *uix_bsearch(const void *key, const void *base,
                  uix_size_t n, uix_size_t sz,
                  int(*cmp)(const void*,const void*))
{
    const unsigned char *a=(const unsigned char*)base;
    uix_size_t lo=0,hi=n;
    while (lo<hi){
        uix_size_t mid=lo+(hi-lo)/2;
        int c=cmp(key,a+mid*sz);
        if (c==0) return (void*)(a+mid*sz);
        else if (c<0) hi=mid; else lo=mid+1;
    }
    return NULL;
}

/* isspace needed above — keep reference here */
int uix_isspace(int c);
