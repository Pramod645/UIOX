#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_errno.h"

/* ── Minimal heap allocator ─────────────────────────────────── */
#define HEAP_SIZE   (4 * 1024 * 1024)   /* 4 MB */

static unsigned char heap[HEAP_SIZE];

typedef struct chunk {
    uix_size_t    size;     /* payload size (bytes)             */
    int           free;     /* 1 = free, 0 = in use             */
    struct chunk *next;
} chunk_t;

static chunk_t *heap_head = NULL;

static void heap_init(void)
{
    heap_head        = (chunk_t *)heap;
    heap_head->size  = HEAP_SIZE - sizeof(chunk_t);
    heap_head->free  = 1;
    heap_head->next  = NULL;
}

void *uix_malloc(uix_size_t size)
{
    if (!heap_head) heap_init();
    if (size == 0) return NULL;

    /* Align to 8 bytes */
    size = (size + 7) & ~(uix_size_t)7;

    chunk_t *c = heap_head;
    while (c) {
        if (c->free && c->size >= size) {
            /* Split chunk if large enough */
            if (c->size >= size + sizeof(chunk_t) + 8) {
                chunk_t *new_c = (chunk_t *)
                    ((unsigned char *)c + sizeof(chunk_t) + size);
                new_c->size  = c->size - size - sizeof(chunk_t);
                new_c->free  = 1;
                new_c->next  = c->next;
                c->next = new_c;
                c->size = size;
            }
            c->free = 0;
            return (void *)((unsigned char *)c + sizeof(chunk_t));
        }
        c = c->next;
    }
    uix_errno = UIX_ENOMEM;
    return NULL;
}

void *uix_calloc(uix_size_t nmemb, uix_size_t size)
{
    uix_size_t total = nmemb * size;
    void *ptr = uix_malloc(total);
    if (ptr) uix_memset(ptr, 0, total);
    return ptr;
}

void *uix_realloc(void *ptr, uix_size_t size)
{
    if (!ptr)   return uix_malloc(size);
    if (!size)  { uix_free(ptr); return NULL; }

    chunk_t *c = (chunk_t *)((unsigned char *)ptr - sizeof(chunk_t));
    if (c->size >= size) return ptr;

    void *new_ptr = uix_malloc(size);
    if (!new_ptr) return NULL;
    uix_memcpy(new_ptr, ptr, c->size);
    uix_free(ptr);
    return new_ptr;
}

void uix_free(void *ptr)
{
    if (!ptr) return;
    chunk_t *c = (chunk_t *)((unsigned char *)ptr - sizeof(chunk_t));
    c->free = 1;

    /* Coalesce adjacent free chunks */
    chunk_t *cur = heap_head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += sizeof(chunk_t) + cur->next->size;
            cur->next  = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

/* ── Process control ────────────────────────────────────────── */
static void (*atexit_funcs[32])(void);
static int   atexit_count = 0;

int uix_atexit(void (*function)(void))
{
    if (atexit_count >= 32) return -1;
    atexit_funcs[atexit_count++] = function;
    return 0;
}

void uix_exit(int status)
{
    for (int i = atexit_count - 1; i >= 0; i--)
        atexit_funcs[i]();
    (void)status;
    while (1) {}   /* halt — real impl calls sys_exit() */
}

void uix_abort(void)
{
    while (1) {}
}

int uix_system(const char *command)
{
    (void)command;
    return -1;   /* stub */
}

/* ── Environment ─────────────────────────────────────────────── */
static char *env_store[256];
static int   env_count = 0;

char *uix_getenv(const char *name)
{
    uix_size_t len = uix_strlen(name);
    for (int i = 0; i < env_count; i++) {
        if (uix_strncmp(env_store[i], name, len) == 0 &&
            env_store[i][len] == '=')
            return env_store[i] + len + 1;
    }
    return NULL;
}

int uix_setenv(const char *name, const char *value, int overwrite)
{
    uix_size_t nlen = uix_strlen(name);
    uix_size_t vlen = uix_strlen(value);

    for (int i = 0; i < env_count; i++) {
        if (uix_strncmp(env_store[i], name, nlen) == 0 &&
            env_store[i][nlen] == '=') {
            if (!overwrite) return 0;
            char *entry = uix_malloc(nlen + 1 + vlen + 1);
            if (!entry) return -1;
            uix_strcpy(entry, name);
            entry[nlen] = '=';
            uix_strcpy(entry + nlen + 1, value);
            uix_free(env_store[i]);
            env_store[i] = entry;
            return 0;
        }
    }
    if (env_count >= 256) { uix_errno = UIX_ENOMEM; return -1; }
    char *entry = uix_malloc(nlen + 1 + vlen + 1);
    if (!entry) return -1;
    uix_strcpy(entry, name);
    entry[nlen] = '=';
    uix_strcpy(entry + nlen + 1, value);
    env_store[env_count++] = entry;
    return 0;
}

int uix_unsetenv(const char *name)
{
    uix_size_t len = uix_strlen(name);
    for (int i = 0; i < env_count; i++) {
        if (uix_strncmp(env_store[i], name, len) == 0 &&
            env_store[i][len] == '=') {
            uix_free(env_store[i]);
            env_store[i] = env_store[--env_count];
            return 0;
        }
    }
    return 0;
}

int uix_putenv(char *string)
{
    char *eq = uix_strchr(string, '=');
    if (!eq) return -1;
    *eq = '\0';
    int r = uix_setenv(string, eq + 1, 1);
    *eq = '=';
    return r;
}

/* ── Number conversion ──────────────────────────────────────── */
int uix_atoi(const char *str)
{
    int result = 0, sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9')
        result = result * 10 + (*str++ - '0');
    return sign * result;
}

long uix_atol(const char *str)
{
    long result = 0;
    int  sign   = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9')
        result = result * 10 + (*str++ - '0');
    return sign * result;
}

double uix_atof(const char *str)
{
    return uix_strtod(str, NULL);
}

long uix_strtol(const char *str, char **endptr, int base)
{
    long result = 0;
    int  sign   = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    if (base == 0) {
        if (*str == '0' && (*(str+1) == 'x' || *(str+1) == 'X'))
            { base = 16; str += 2; }
        else if (*str == '0') { base = 8; str++; }
        else base = 10;
    }
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9')      digit = *str - '0';
        else if (*str >= 'a' && *str <= 'f') digit = *str - 'a' + 10;
        else if (*str >= 'A' && *str <= 'F') digit = *str - 'A' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        str++;
    }
    if (endptr) *endptr = (char *)str;
    return sign * result;
}

unsigned long uix_strtoul(const char *str, char **endptr, int base)
{
    return (unsigned long)uix_strtol(str, endptr, base);
}

double uix_strtod(const char *str, char **endptr)
{
    double result = 0.0, frac = 1.0;
    int    sign   = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9')
        result = result * 10.0 + (*str++ - '0');
    if (*str == '.') {
        str++;
        while (*str >= '0' && *str <= '9') {
            frac  /= 10.0;
            result += (*str++ - '0') * frac;
        }
    }
    if (endptr) *endptr = (char *)str;
    return sign * result;
}

/* ── Math ────────────────────────────────────────────────────── */
int  uix_abs (int  x) { return x < 0 ? -x : x; }
long uix_labs(long x) { return x < 0 ? -x : x; }

/* ── Random ──────────────────────────────────────────────────── */
static unsigned long uix_rand_seed = 1;

int uix_rand(void)
{
    uix_rand_seed = uix_rand_seed * 1103515245UL + 12345UL;
    return (int)((uix_rand_seed >> 16) & 0x7FFF);
}

void uix_srand(unsigned int seed) { uix_rand_seed = seed; }

/* ── Sorting ─────────────────────────────────────────────────── */
void uix_qsort(void *base, uix_size_t nmemb, uix_size_t size,
               int (*compar)(const void *, const void *))
{
    if (nmemb < 2) return;
    unsigned char *arr = (unsigned char *)base;
    unsigned char *tmp = uix_malloc(size);
    if (!tmp) return;

    /* Insertion sort (simple, adequate for UIOX) */
    for (uix_size_t i = 1; i < nmemb; i++) {
        uix_memcpy(tmp, arr + i * size, size);
        uix_ssize_t j = (uix_ssize_t)i - 1;
        while (j >= 0 &&
               compar(arr + j * size, tmp) > 0) {
            uix_memcpy(arr + (j + 1) * size,
                       arr + j * size, size);
            j--;
        }
        uix_memcpy(arr + (j + 1) * size, tmp, size);
    }
    uix_free(tmp);
}

void *uix_bsearch(const void *key, const void *base,
                  uix_size_t nmemb, uix_size_t size,
                  int (*compar)(const void *, const void *))
{
    const unsigned char *arr = (const unsigned char *)base;
    uix_size_t lo = 0, hi = nmemb;

    while (lo < hi) {
        uix_size_t  mid = lo + (hi - lo) / 2;
        const void *mid_ptr = arr + mid * size;
        int cmp = compar(key, mid_ptr);
        if      (cmp == 0) return (void *)mid_ptr;
        else if (cmp <  0) hi  = mid;
        else               lo  = mid + 1;
    }
    return NULL;
}
