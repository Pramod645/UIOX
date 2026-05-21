CC     = gcc
CFLAGS = -Wall -Wextra -std=c99 \
         -I../../40_SystemCallInterface \
         -I../../50_UIX/00_libs/00_uixlibs \
         -DSYS_CALL_ENBLE_DISABLE=0

APPS = app_shell app_logger app_fileserver app_netserver \
       app_ipctest app_memtest app_scheduler app_fstest \
    	app_clocktest

all: $(APPS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(APPS)
