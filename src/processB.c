#include "./../include/processB_utilities.h"
#include <bmpfile.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>

int finish = 0;

int write_log(int fd_log, char *msg, int lmsg)
{
    char log_msg[64];
    time_t now = time(NULL);
    struct tm *timenow = localtime(&now);
    int length = strftime(log_msg, 64, "[%H:%M:%S]: ", timenow);
    if (write(fd_log, log_msg, length) < 0 && errno != EINTR)
        return -1;
    if (write(fd_log, msg, lmsg) < 0 && errno != EINTR)
        return -1;
    return 0;
}

void handler_exit(int sig)
{
  finish = 1;
}

int main(int argc, char const *argv[])
{
    // Send PID to process A:
    // pid_t pid_b = getpid();
    // char *b_fifo = "./tmp/pid";
    // mkfifo(b_fifo, 0666);
    // int fd_b = open(b_fifo, O_WRONLY);
    // if (fd_b < 0 && errno != EINTR)
    //     perror("Error opening process A-B fifo (B)");
    // char buf[10];
    // sprintf(buf, "%d", pid_b);
    // if (write(fd_b, buf, 10) < 0) perror("Error writing to process A-B fifo (B)");
    // sleep(2);
    // close(fd_b);

    // Log file:
    int fd_log = creat("./logs/processB.txt", 0666);
    char log_msg[64];
    int length;

    // Signal handling to exit process:
    struct sigaction sa_exit;
    sigemptyset(&sa_exit.sa_mask);
    sa_exit.sa_handler = &handler_exit;
    sa_exit.sa_flags = SA_RESTART;
    if (sigaction(SIGTERM, &sa_exit, NULL) < 0)
    {
        int length = snprintf(log_msg, 64, "Cannot catch SIGTERM.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (cmd)");
    }

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Infinite loop
    while (!finish) {

        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
            }
            else {
                reset_console_ui();
            }
        }

        else {
            mvaddch(LINES/2, COLS/2, '0');
            refresh();
        }
    }

    endwin();
    return 0;
}
