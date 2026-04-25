// demo Source Code Using sys/utsname.h

#include <stdio.h>
#include <sys/utsname.h>

int utsname(void) {
    struct utsname systeminfo;

    if (uname(&systeminfo) == -1) {
        perror("uname");
        return 1;
    }

    printf("System name:  %s\n", systeminfo.sysname);
    printf("Node name:    %s\n", systeminfo.nodename);
    printf("Release:      %s\n", systeminfo.release);
    printf("Version:      %s\n", systeminfo.version);
    printf("Machine:      %s\n", systeminfo.machine);

    return 0;
}
/*
This program prints details about your system when compiled and run—for example:

`
System name:  Linux
Node name:    workstation
Release:      6.5.0-28-generic
Version:      #29-Ubuntu SMP PREEMPTDYNAMIC ...
Machine:      x86_64
`
*/