#include "uix_string.h"
#include "uix_errno.h"
#include "uix_stdlib.h"

/* ── Memory functions ───────────────────────────────────────── */
void *uix_memset(void *ptr, int value, uix_size_t n)
{
    unsigned char *p = (unsigned char *)ptr;
    while (n--) *p++ = (unsigned char)value;
    return ptr;
}

void *uix_memcpy(void *dest, const void *src, uix_size_t n)
{
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dest;
}

void *uix_memmove(void *dest, const void *src, uix_size_t n)
{
    unsigned char       *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n; s += n;
        while (n--) *--d = *--s;
    }
    return dest;
}

int uix_memcmp(const void *s1, const void *s2, uix_size_t n)
{
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2) return (int)*p1 - (int)*p2;
        p1++; p2++;
    }
    return 0;
}

void *uix_memchr(const void *ptr, int value, uix_size_t n)
{
    const unsigned char *p = (const unsigned char *)ptr;
    while (n--) {
        if (*p == (unsigned char)value) return (void *)p;
        p++;
    }
    return NULL;
}

/* ── String functions ───────────────────────────────────────── */
uix_size_t uix_strlen(const char *str)
{
    const char *p = str;
    while (*p) p++;
    return (uix_size_t)(p - str);
}

char *uix_strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++)) continue;
    return dest;
}

char *uix_strncpy(char *dest, const char *src, uix_size_t n)
{
    char *d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

char *uix_strcat(char *dest, const char *src)
{
    char *d = dest + uix_strlen(dest);
    while ((*d++ = *src++)) continue;
    return dest;
}

char *uix_strncat(char *dest, const char *src, uix_size_t n)
{
    char *d = dest + uix_strlen(dest);
    while (n && *src) { *d++ = *src++; n--; }
    *d = '\0';
    return dest;
}

int uix_strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

int uix_strncmp(const char *s1, const char *s2, uix_size_t n)
{
    while (n && *s1 && *s1 == *s2) { s1++; s2++; n--; }
    return n ? (int)(unsigned char)*s1 -
               (int)(unsigned char)*s2 : 0;
}

char *uix_strchr(const char *str, int c)
{
    while (*str) {
        if (*str == (char)c) return (char *)str;
        str++;
    }
    return ((char)c == '\0') ? (char *)str : NULL;
}

char *uix_strrchr(const char *str, int c)
{
    const char *last = NULL;
    while (*str) {
        if (*str == (char)c) last = str;
        str++;
    }
    return ((char)c == '\0') ? (char *)str : (char *)last;
}

char *uix_strstr(const char *haystack, const char *needle)
{
    if (!*needle) return (char *)haystack;
    while (*haystack) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char *)haystack;
        haystack++;
    }
    return NULL;
}

char *uix_strtok(char *str, const char *delim)
{
    static char *saved = NULL;
    if (str) saved = str;
    if (!saved) return NULL;
    while (*saved && uix_strchr(delim, *saved)) saved++;
    if (!*saved) { saved = NULL; return NULL; }
    char *token = saved;
    while (*saved && !uix_strchr(delim, *saved)) saved++;
    if (*saved) *saved++ = '\0';
    else         saved   = NULL;
    return token;
}

uix_size_t uix_strspn(const char *str, const char *accept)
{
    uix_size_t n = 0;
    while (*str && uix_strchr(accept, *str)) { str++; n++; }
    return n;
}

uix_size_t uix_strcspn(const char *str, const char *reject)
{
    uix_size_t n = 0;
    while (*str && !uix_strchr(reject, *str)) { str++; n++; }
    return n;
}
