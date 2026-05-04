#include "uix_regex.h"
#include "uix_stdlib.h"
#include "uix_string.h"
#include "uix_errno.h"

/* Minimal literal-string regex for UIOX */
int uix_regcomp(uix_regex_t *preg, const char *pattern, int cflags)
{
    if (!preg || !pattern) { uix_errno = UIX_EINVAL; return UIX_REG_BADPAT; }
    preg->re_nsub      = 0;
    preg->re_internal  = (void *)uix_strdup(pattern);
    (void)cflags;
    return preg->re_internal ? 0 : UIX_REG_ESPACE;
}

int uix_regexec(const uix_regex_t *preg, const char *string,
                uix_size_t nmatch, uix_regmatch_t *pmatch, int eflags)
{
    if (!preg || !string) return UIX_REG_NOMATCH;
    (void)eflags;
    const char *pat = (const char *)preg->re_internal;
    const char *found = uix_strstr(string, pat);
    if (!found) return UIX_REG_NOMATCH;

    if (nmatch > 0 && pmatch) {
        pmatch[0].rm_so = (uix_regoff_t)(found - string);
        pmatch[0].rm_eo = pmatch[0].rm_so + uix_strlen(pat);
    }
    return 0;
}

void uix_regfree(uix_regex_t *preg)
{
    if (preg && preg->re_internal) {
        uix_free(preg->re_internal);
        preg->re_internal = NULL;
    }
}

uix_size_t uix_regerror(int errcode, const uix_regex_t *preg,
                        char *errbuf, uix_size_t errbuf_size)
{
    (void)preg;
    const char *msg;
    switch (errcode) {
    case 0:              msg = "Success";            break;
    case UIX_REG_NOMATCH: msg = "No match";          break;
    case UIX_REG_BADPAT:  msg = "Invalid pattern";   break;
    case UIX_REG_ESPACE:  msg = "Out of memory";     break;
    default:             msg = "Unknown regex error"; break;
    }
    uix_size_t len = uix_strlen(msg) + 1;
    if (errbuf && errbuf_size > 0)
        uix_strncpy(errbuf, msg, errbuf_size - 1);
    return len;
}
