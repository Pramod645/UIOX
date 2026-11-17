#ifndef UIOX_DEV_STREAM_H
#define UIOX_DEV_STREAM_H

#include "uiox_dev.h"
#include "../sys/uix_types.h"     /* uix_ssize_t, uix_size_t */
#include "../PoStd/uix_poll.h"    /* uix_pollfd_t, uix_nfds_t, UIX_POLLIN */

uiox_dev_err_t uiox_dev_stream_open_name(uiox_dev_handle_t *h,
                                         const char *name, int flags);
uiox_dev_err_t uiox_dev_stream_open(uiox_dev_handle_t *h,
                                    const char *prefix, int unit, int flags);

uix_ssize_t    uiox_dev_stream_read (uiox_dev_handle_t *h, void *buf, uix_size_t n);
uix_ssize_t    uiox_dev_stream_write(uiox_dev_handle_t *h, const void *buf, uix_size_t n);

/* Builds a one-element pollfd and calls uix_poll; returns revents, or -1. */
uiox_dev_err_t uiox_dev_stream_poll(uiox_dev_handle_t *h, short events, int timeout_ms);
uiox_dev_err_t uiox_dev_stream_close(uiox_dev_handle_t *h);
#endif
