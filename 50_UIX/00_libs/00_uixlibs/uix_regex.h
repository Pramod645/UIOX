#ifndef UIX_REGEX_H
#define UIX_REGEX_H

#include "uix_types.h"

#define UIX_REG_EXTENDED 1
#define UIX_REG_ICASE    2
#define UIX_REG_NOSUB    4
#define UIX_REG_NEWLINE  8

#define UIX_REG_NOTBOL   1
#define UIX_REG_NOTEOL   2

#define UIX_REG_NOMATCH  1
#define UIX_REG_BADPAT   2
#define UIX_REG_ECOLLATE 3
#define UIX_REG_ECTYPE   4
#define UIX_REG_EESCAPE  5
#define UIX_REG_ESUBREG  6
#define UIX_REG_EBRACK   7
#define UIX_REG_EPAREN   8
#define UIX_REG_EBRACE   9
#define UIX_REG_BADBR    10
#define UIX_REG_ERANGE   11
#define UIX_REG_ESPACE   12
#define UIX_REG_BADRPT   13

typedef uix_size_t uix_regoff_t;

typedef struct uix_regmatch {
    uix_regoff_t rm_so;
    uix_regoff_t rm_eo;
} uix_regmatch_t;

typedef struct uix_regex {
    uix_size_t  re_nsub;
    void       *re_internal;
} uix_regex_t;

int        uix_regcomp (uix_regex_t *preg, const char *pattern, int cflags);
int        uix_regexec (const uix_regex_t *preg, const char *string,
                         uix_size_t nmatch, uix_regmatch_t *pmatch,
                         int eflags);
void       uix_regfree (uix_regex_t *preg);
uix_size_t uix_regerror(int errcode, const uix_regex_t *preg,
                         char *errbuf, uix_size_t errbuf_size);

#endif /* UIX_REGEX_H */
