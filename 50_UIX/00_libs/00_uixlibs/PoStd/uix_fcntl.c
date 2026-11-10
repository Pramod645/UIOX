#include "uix_fcntl.h"
#include "uix_errno.h"
#include "../sys/uix_stat.h"
//#include "../sys/uix_types.h"
#include "uix_stdarg.h"

#include "uix_sys.h"


int uix_creat(const char *path, uix_mode_t mode)
{
    return uix_open(path, UIX_O_CREAT | UIX_O_WRONLY | UIX_O_TRUNC,
                    mode);
}



/* ***This is End of file, there is no more line should be added after this line*** */
