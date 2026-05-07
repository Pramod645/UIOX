not implement gets() because it is unsafe and removed from modern C. The header explicitly documents that.
fmemopen() and open_memstream() are enabled with _GNU_SOURCE; on non-glibc systems they may not exist.
uiox_print_stream_buffering() is intentionally best-effort because inspecting FILE internals is not portable. The safe function is uiox_print_stream_basic().
uiox_fflush_fsync() shows the correct way to combine standard I/O buffering with fsync: first fflush(fp), then fsync(fileno(fp)).