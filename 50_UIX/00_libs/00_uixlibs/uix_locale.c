localedemo.c — Demonstrate basic use of <locale.h> /

#include <stdio.h>
#include <locale.h>
#include <stdlib.h>

int locale(void) {
    double number = 1234567.89;

    // 1. Default "C" locale /
    printf("Default locale: %s\n", setlocale(LCALL, NULL));
    printf("Number (default): %.2f\n", number);

    // 2. Switch to a local locale, e.g., German or French /
    if (setlocale(LCALL, "deDE.UTF-8") == NULL) {
        printf("German locale not available.\n");
    } else {
        printf("\nLocale changed to: %s\n", setlocale(LCALL, NULL));
        printf("Number (German locale): %.2f\n", number);

        struct lconv lc = localeconv();
        printf("Decimal point: '%s'\n", lc->decimalpoint);
        printf("Thousands separator: '%s'\n", lc->thousandssep);
    }

    // 3. Switch to French locale /
    if (setlocale(LCALL, "frFR.UTF-8") == NULL) {
        printf("\nFrench locale not available.\n");
    } else {
        printf("\nLocale changed to: %s\n", setlocale(LCALL, NULL));
        printf("Number (French locale): %.2f\n", number);

        struct lconv *lc = localeconv();
        printf("Decimal point: '%s'\n", lc->decimalpoint);
        printf("Thousands separator: '%s'\n", lc->thousandssep);
    }

    return 0;
}
/*
Example output (depends on what locales are installed on your system):


Default locale: C
Number (default): 1234567.89

Locale changed to: deDE.UTF-8
Number (German locale): 1234567,89
Decimal point: ','
Thousands separator: '.'

Locale changed to: frFR.UTF-8
Number (French locale): 1234567,89
Decimal point: ','
Thousands separator: ' '
`

Explanation
• setlocale(category, locale)  
  Selects which locale rules to use for output and classification.  
  Example:  
  - "C" — the default standard locale.  
  - "deDE.UTF-8" — German (Germany).  
  - "frFR.UTF-8" — French (France).

• localeconv()  
  Returns a pointer to a struct lconv describing formatting conventions (decimal point, currency, separators, etc.).

• Categories like LCTIME and LCMONETARY affect functions such as strftime() and printf("%'f").

Summary

| Function / Macro | Purpose |
|-------------------|----------|
| setlocale(cat, loc) | Set or query locale settings. |
| localeconv() | Get locale-specific formatting data. |
| LCNUMERIC | Affects decimal point and number formatting. |
| LCTIME | Affects strftime date/time formats. |
| LCMONETARY | Affects monetary formatting via localeconv. |
| LC_ALL | Applies to all locale categories. |

*/
////////////
/* src/uix_locale.c */
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


