/**********************uix_fnmatch.c ***********************************/
#include "uix_fnmatch.h"
#include "uix_ctype.h"
#include "uix_string.h"

int uix_fnmatch(const char *pat, const char *str, int flags)
{
    while (*pat) {
        if (*pat == '?') {
            if (!*str) return UIX_FNM_NOMATCH;
            if ((flags & UIX_FNM_PATHNAME) && *str == '/')
                return UIX_FNM_NOMATCH;
            pat++; str++;
        } else if (*pat == '*') {
            while (*pat == '*') pat++;
            if (!*pat) return 0;
            while (*str) {
                if (uix_fnmatch(pat, str, flags) == 0) return 0;
                if ((flags & UIX_FNM_PATHNAME) && *str == '/')
                    return UIX_FNM_NOMATCH;
                str++;
            }
            return UIX_FNM_NOMATCH;
        } else if (*pat == '[') {
            pat++;
            int inv = (*pat == '!'); if (inv) pat++;
            int matched = 0;
            char prev = 0;
            while (*pat && *pat != ']') {
                if (*pat == '-' && prev && *(pat+1) && *(pat+1) != ']') {
                    matched |= (*str >= prev && *str <= *(pat+1));
                    pat += 2;
                } else {
                    if (*pat == *str) matched = 1;
                    prev = *pat++;
                }
            }
            if (*pat == ']') pat++;
            if (matched == inv) return UIX_FNM_NOMATCH;
            str++;
        } else {
            char p = *pat, s = *str;
            if (flags & UIX_FNM_CASEFOLD) {
                p = (char)uix_tolower((unsigned char)p);
                s = (char)uix_tolower((unsigned char)s);
            }
            if (p != s) return UIX_FNM_NOMATCH;
            pat++; str++;
        }
    }
    return *str ? UIX_FNM_NOMATCH : 0;
}

/* ***This is End of file, there is no more line should be added after this line*** */
