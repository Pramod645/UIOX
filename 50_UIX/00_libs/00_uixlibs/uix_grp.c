#include "uix_grp.h"
#include "uix_string.h"
#include "uix_errno.h"

static char *root_members[] = { "root", NULL };
static char *uiox_members[] = { "uiox", NULL };

static uix_group_t gr_root = { "root", "x", 0,    root_members };
static uix_group_t gr_uiox = { "uiox", "x", 1000, uiox_members };
static uix_group_t *gr_list[] = { &gr_root, &gr_uiox, NULL };
static int gr_iter = 0;

uix_group_t *uix_getgrgid(uix_gid_t gid)
{
    for (int i = 0; gr_list[i]; i++)
        if (gr_list[i]->gr_gid == gid) return gr_list[i];
    uix_errno = UIX_ENOENT; return NULL;
}

uix_group_t *uix_getgrnam(const char *name)
{
    for (int i = 0; gr_list[i]; i++)
        if (uix_strcmp(gr_list[i]->gr_name, name) == 0)
            return gr_list[i];
    uix_errno = UIX_ENOENT; return NULL;
}

int uix_getgrgid_r(uix_gid_t gid, uix_group_t *grp,
                   char *buf, uix_size_t buflen,
                   uix_group_t **result)
{
    (void)buf; (void)buflen;
    uix_group_t *g = uix_getgrgid(gid);
    if (g) { *grp = *g; *result = grp; return 0; }
    *result = NULL; return UIX_ENOENT;
}

int uix_getgrnam_r(const char *name, uix_group_t *grp,
                   char *buf, uix_size_t buflen,
                   uix_group_t **result)
{
    (void)buf; (void)buflen;
    uix_group_t *g = uix_getgrnam(name);
    if (g) { *grp = *g; *result = grp; return 0; }
    *result = NULL; return UIX_ENOENT;
}

void         uix_setgrent(void) { gr_iter = 0; }
void         uix_endgrent(void) { gr_iter = 0; }
uix_group_t *uix_getgrent(void)
{
    return gr_list[gr_iter] ? gr_list[gr_iter++] : NULL;
}

/* ***This is End of file, there is no more line should be added after this line*** */
