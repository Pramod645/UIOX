/************************ uix_locale.c ***********************************/
#include "uix_locale.h"
#include "uix_string.h"

static char _locale_name[32] = "C";

static uix_lconv_t _lconv = {
    ".", "", "", "", "$", ".", "", "", "+", "-",
    2, 2, 1, 0, 1, 0, 1, 1
};

char *uix_setlocale(int category, const char *locale)
{
    (void)category;
    if (!locale) return _locale_name;
    if (uix_strcmp(locale,"C")==0 || uix_strcmp(locale,"POSIX")==0
        || uix_strcmp(locale,"")==0) {
        uix_strncpy(_locale_name, *locale?locale:"C", 31);
        return _locale_name;
    }
    return NULL;
}

uix_lconv_t *uix_localeconv(void) { return &_lconv; }

/* ***This is End of file, there is no more line should be added after this line*** */
