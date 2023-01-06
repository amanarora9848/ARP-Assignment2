#define _XOPEN_SOURCE 700
#include "./../include/processB_utilities.h"
#include <bmpfile.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include "./../include/bmp_functions.h"
#include <time.h>

#define DT 25 // Time in ms (40Hz)
// #define SHM_NAME "/bmp_memory"
// #define SEM_NAME "/bmp_sem"

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
    pid_t pid_b = getpid();
    char *b_fifo = "./tmp/pid";
    if (mkfifo(b_fifo, 0666) < 0) {
        perror("Error creating pid fifo (B)");
    }
    int fd_b = open(b_fifo, O_WRONLY);
    if (fd_b < 0 && errno != EINTR)
        perror("Error opening process A-B fifo (B)");
    char buf[10];
    sprintf(buf, "%d", pid_b);
    if (write(fd_b, buf, 10) < 0) perror("Error writing to process A-B fifo (B)");
    sleep(2);
    close(fd_b);

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
        length = snprintf(log_msg, 64, "Cannot catch SIGTERM.\n");
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (cmd)");
    }

    const struct timespec delay_nano = {
        .tv_sec = 0,
        .tv_nsec = DT*1e6
    }; // 25ms

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Drawing initial state:
    // bmpfile_t *bmp;
    const size_t shm_size = WIDTH * HEIGHT * sizeof(rgb_pixel_t);

    // Access shared memory
    char shm_name[] = "/bmp_memory";
    int shm_fd = shm_open(shm_name, O_RDONLY, 0666);
    if (shm_fd < 0 && errno != EINTR){
        length = snprintf(log_msg, 64, "Error reading shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (B)");
        exit(1);
    }

    // open semaphore
    char sem_name[] = "/bmp_sem";
    sem_t *sem_id = sem_open(sem_name, 1);
    if (sem_id == SEM_FAILED) {
        length = snprintf(log_msg, 64, "Error opening semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (B)");
        close(shm_fd);
        shm_unlink(shm_name);
        exit(1);
    }

    // Initialize semaphore: (Debug)
    if (sem_init(sem_id, 1, 0) < 0)
    {
        perror("Error initializing semaphore");
        sem_close(sem_id);
        sem_unlink(sem_name);
    }

    // Map shared memory
    rgb_pixel_t *ptr = (rgb_pixel_t *)mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        length = snprintf(log_msg, 64, "Error mapping shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (B)");
        close(shm_fd);
        shm_unlink(shm_name);
        sem_close(sem_id);
        sem_unlink(sem_name);
        exit(1);
    }


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
            // Lock semaphore
            if (sem_wait(sem_id) < 0) {
                length = snprintf(log_msg, 64, "Error waiting semaphore: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (B)");
                finish = 1;
            }

            // // Get bitmap from shared memory - Might not need it.
            // load_bmp(bmp, ptr);
            int* center_pos;
            // TODO: confirm the operation on scaling factor
            find_circle_center(ptr, center_pos);
            length = snprintf(log_msg, 64, "Center of circle found at (%d, %d).", center_pos[0], center_pos[1]);
            if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                perror("Error writing to log (B)");
            
            // Draw circle center
            mvaddch(center_pos[0] / SCALE, center_pos[1] / SCALE, '0');

            refresh();

            nanosleep(&delay_nano, NULL);
        }
    }

    // Termination:
    length = snprintf(log_msg, 64, "Exited successfully.");
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (B)");
    close(shm_fd);
    shm_unlink(shm_name);
    sem_close(sem_id);
    sem_unlink(sem_name);
    close(fd_log);
    endwin();
    return 0;
}
