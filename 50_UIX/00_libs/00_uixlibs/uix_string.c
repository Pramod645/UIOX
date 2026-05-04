#include "uix_string.h"
#include "uix_stdlib.h"

uix_size_t uix_strlen(const char *s)
{
    const char *p = s; while (*p) p++; return (uix_size_t)(p - s);
}

char *uix_strcpy(char *d, const char *s)
{
    char *r = d; while ((*d++ = *s++)); return r;
}

char *uix_strncpy(char *d, const char *s, uix_size_t n)
{
    char *r = d;
    while (n && (*d++ = *s++)) n--;
    while (n--) *d++ = '\0';
    return r;
}

char *uix_strcat(char *d, const char *s)
{
    char *r = d; d += uix_strlen(d); while ((*d++ = *s++)); return r;
}

char *uix_strncat(char *d, const char *s, uix_size_t n)
{
    char *r = d; d += uix_strlen(d);
    while (n-- && *s) *d++ = *s++;
    *d = '\0'; return r;
}

int uix_strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int uix_strncmp(const char *s1, const char *s2, uix_size_t n)
{
    while (n && *s1 && *s1 == *s2) { s1++; s2++; n--; }
    return n ? (unsigned char)*s1 - (unsigned char)*s2 : 0;
}

char *uix_strchr(const char *s, int c)
{
    while (*s) { if (*s == (char)c) return (char*)s; s++; }
    return ((char)c == '\0') ? (char*)s : NULL;
}

char *uix_strrchr(const char *s, int c)
{
    const char *last = NULL;
    while (*s) { if (*s == (char)c) last = s; s++; }
    return ((char)c == '\0') ? (char*)s : (char*)last;
}

char *uix_strstr(const char *h, const char *n)
{
    if (!*n) return (char*)h;
    while (*h) {
        const char *a = h, *b = n;
        while (*a && *b && *a == *b) { a++; b++; }
        if (!*b) return (char*)h;
        h++;
    }
    return NULL;
}

char *uix_strtok(char *s, const char *d)
{
    static char *sv;
    if (s) sv = s;
    if (!sv) return NULL;
    while (*sv && uix_strchr(d, *sv)) sv++;
    if (!*sv) { sv = NULL; return NULL; }
    char *t = sv;
    while (*sv && !uix_strchr(d, *sv)) sv++;
    if (*sv) *sv++ = '\0'; else sv = NULL;
    return t;
}

uix_size_t uix_strspn(const char *s, const char *a)
{
    uix_size_t n = 0;
    while (*s && uix_strchr(a, *s)) { s++; n++; }
    return n;
}

uix_size_t uix_strcspn(const char *s, const char *r)
{
    uix_size_t n = 0;
    while (*s && !uix_strchr(r, *s)) { s++; n++; }
    return n;
}

char *uix_strdup(const char *s)
{
    uix_size_t n = uix_strlen(s) + 1;
    char *p = (char *)uix_malloc(n);
    if (p) uix_memcpy(p, s, n);
    return p;
}

char *uix_strndup(const char *s, uix_size_t n)
{
    uix_size_t len = uix_strlen(s);
    if (len > n) len = n;
    char *p = (char *)uix_malloc(len + 1);
    if (p) { uix_memcpy(p, s, len); p[len] = '\0'; }
    return p;
}

void *uix_memset(void *p, int v, uix_size_t n)
{
    unsigned char *d = (unsigned char*)p;
    while (n--) *d++ = (unsigned char)v;
    return p;
}

void *uix_memcpy(void *d, const void *s, uix_size_t n)
{
    unsigned char *dp = (unsigned char*)d;
    const unsigned char *sp = (const unsigned char*)s;
    while (n--) *dp++ = *sp++;
    return d;
}

void *uix_memmove(void *d, const void *s, uix_size_t n)
{
    unsigned char *dp = (unsigned char*)d;
    const unsigned char *sp = (const unsigned char*)s;
    if (dp < sp) { while (n--) *dp++ = *sp++; }
    else { dp += n; sp += n; while (n--) *--dp = *--sp; }
    return d;
}

int uix_memcmp(const void *a, const void *b, uix_size_t n)
{
    const unsigned char *p = (const unsigned char*)a;
    const unsigned char *q = (const unsigned char*)b;
    while (n--) { if (*p != *q) return *p - *q; p++; q++; }
    return 0;
}

void *uix_memchr(const void *s, int c, uix_size_t n)
{
    const unsigned char *p = (const unsigned char*)s;
    while (n--) { if (*p == (unsigned char)c) return (void*)p; p++; }
    return NULL;
}
