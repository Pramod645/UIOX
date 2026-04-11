// termiosdemo.c — Demonstrating terminal mode manipulation /

#include <stdio.h>
#include <unistd.h>
#include <termios.h>

int libtermios(void) {
    struct termios oldt, newt;
    char ch;

    // Get current terminal attributes /
    if (tcgetattr(STDINFILENO, &oldt) == -1) {
        perror("tcgetattr");
        return 1;
    }

    // Copy current settings to modify /
    newt = oldt;
    newt.clflag &= ~(ICANON | ECHO);  / Disable canonical mode & echo /
    newt.ccc[VMIN] = 1;               / Minimum number of bytes to read /
    newt.ccc[VTIME] = 0;              / No timeout /

    if (tcsetattr(STDINFILENO, TCSANOW, &newt) == -1) {
        perror("tcsetattr");
        return 1;
    }

    printf("Type characters (press 'q' to quit):\n");

    // Read characters without waiting for newline /
    while (read(STDINFILENO, &ch, 1) == 1 && ch != 'q') {
        printf("You typed: '%c' (0x%x)\n", ch, ch);
    }

    // Restore original terminal settings /
    if (tcsetattr(STDINFILENO, TCSANOW, &oldt) == -1) {
        perror("tcsetattr restore");
        return 1;
    }

    printf("\nTerminal settings restored. Exiting.\n");
    return 0;
}
/*
Summary
• Header: <termios.h>  
• Controls: Input, output, local, and control modes for terminal I/O.  
• Main functions:
  - tcgetattr(), tcsetattr() — read and apply settings.
  - cfsetispeed(), cfsetospeed() — set baud rates.  
• Use cases:  
  - Command-line apps, shells, password prompts, serial communication.  
  - Tools like ssh, screen, or interactive terminals rely on it.

*/