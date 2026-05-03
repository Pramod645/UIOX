#include "uix_pwd.h"
#include "uix_string.h"
#include "uix_errno.h"

static uix_passwd_t pw_root = {
    "root", "x", 0, 0, "root", "/root", "/bin/sh"
};
static uix_passwd_t pw_uiox = {
    "uiox", "x", 1000, 1000, "UIOX User", "/home/uiox", "/bin/sh"
};
static uix_passwd_t *pw_list[] = { &pw_root, &pw_uiox, NULL };
static int pw_iter = 0;

uix_passwd_t *uix_getpwuid(uix_uid_t uid)
{
    for (int i = 0; pw_list[i]; i++)
        if (pw_list[i]->pw_uid == uid) return pw_list[i];
    uix_errno = UIX_ENOENT; return NULL;
}

uix_passwd_t *uix_getpwnam(const char *name)
{
    for (int i = 0; pw_list[i]; i++)
        if (uix_strcmp(pw_list[i]->pw_name, name) == 0) return pw_list[i];
    uix_errno = UIX_ENOENT; return NULL;
}

int uix_getpwuid_r(uix_uid_t uid, uix_passwd_t *pwd,
                   char *buf, uix_size_t buflen,
                   uix_passwd_t **result)
{
    (void)buf; (void)buflen;
    uix_passwd_t *p = uix_getpwuid(uid);
    if (p) { *pwd = *p; *result = pwd; return 0; }
    *result = NULL; return UIX_ENOENT;
}

int uix_getpwnam_r(const char *name, uix_passwd_t *pwd,
                   char *buf, uix_size_t buflen,
                   uix_passwd_t **result)
{
    (void)buf; (void)buflen;
    uix_passwd_t *p = uix_getpwnam(name);
    if (p) { *pwd = *p; *result = pwd; return 0; }
    *result = NULL; return UIX_ENOENT;
}

void          uix_setpwent(void) { pw_iter = 0; }
void          uix_endpwent(void) { pw_iter = 0; }
uix_passwd_t *uix_getpwent(void)
{
    return pw_list[pw_iter] ? pw_list[pw_iter++] : NULL;
}
