#include "uix_ctype.h"

int uix_isalpha (int c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z'); }
int uix_isdigit (int c) { return c>='0' && c<='9'; }
int uix_isalnum (int c) { return uix_isalpha(c) || uix_isdigit(c); }
int uix_isspace (int c) { return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v'; }
int uix_isupper (int c) { return c>='A' && c<='Z'; }
int uix_islower (int c) { return c>='a' && c<='z'; }
int uix_isprint (int c) { return c>=' ' && c<='~'; }
int uix_isgraph (int c) { return c>'!' && c<='~'; }
int uix_iscntrl (int c) { return (c>=0 && c<32) || c==127; }
int uix_isxdigit(int c) { return uix_isdigit(c)||(c>='a'&&c<='f')||(c>='A'&&c<='F'); }
int uix_isblank (int c) { return c==' ' || c=='\t'; }
int uix_ispunct (int c) { return uix_isprint(c) && !uix_isalnum(c) && c!=' '; }
int uix_toupper (int c) { return uix_islower(c) ? c - 'a' + 'A' : c; }
int uix_tolower (int c) { return uix_isupper(c) ? c - 'A' + 'a' : c; }

/* ***This is End of file, there is no more line should be added after this line*** */
