#include "../../40_SystemCallInterface/uix_sys.h"

#define TEST_PATH "/tmp/uix_fstest.txt"

int main(void)
{
    /* creat */
    int fd = sys_creat(TEST_PATH, 0644);
    if (fd < 0) sys_exit(1);
    const char *msg = "UIX filesystem test\n";
    sys_write(fd, msg, 20);
    sys_close(fd);

    /* open + read */
    fd = sys_open(TEST_PATH, 0, 0);
    if (fd < 0) sys_exit(1);
    char buf[64];
    uix_ssize_t n = sys_read(fd, buf, 63);
    if (n > 0) buf[n] = '\0';
    sys_close(fd);

    /* stat */
    uix_stat_t st;
    sys_stat(TEST_PATH, &st);

    /* chmod */
    sys_chmod(TEST_PATH, 0600);

    /* mkdir / rmdir */
    sys_mkdir("/tmp/uix_testdir", 0755);
    sys_rmdir("/tmp/uix_testdir");

    /* link / unlink */
    sys_link(TEST_PATH, "/tmp/uix_fstest_link.txt");
    sys_unlink("/tmp/uix_fstest_link.txt");
    sys_unlink(TEST_PATH);

    return 0;
}
