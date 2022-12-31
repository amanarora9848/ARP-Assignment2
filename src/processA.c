#include "./../include/processA_utilities.h"
#include <bmpfile.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include "./../include/bmp_functions.h"

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

int main(int argc, char *argv[])
{
    pid_t pid_b;
    char *pid_fifo = "./tmp/pid";
    mkfifo(pid_fifo, 0666);
    int fd_b = open(pid_fifo, O_RDONLY);
    if (fd_b < 0 && errno != EINTR)
        perror("Error opening pid fifo (A)");
    char buf[10];
    if (read(fd_b, buf, 10) < 0)
        perror("Error reading from pid fifo (A)");
    sscanf(buf, "%d", &pid_b);
    close(fd_b);

    // Log file:
    int fd_log = creat("./logs/processA.txt", 0666);
    char log_msg[64];
    int length;

    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Draw initial state:
    bmpfile_t *bmp;
    const size_t shm_size = WIDTH * HEIGHT * sizeof(rgb_pixel_t);
    bmp = bmp_create(WIDTH, HEIGHT, 4);
    if (bmp == NULL)
    {
        length = snprintf(log_msg, 64, "Error creating bitmap: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        exit(1);
    }
    draw_bmp(bmp, get_circle_x() * SCALE, get_circle_y() * SCALE);

    // Create shared memory:
    char shm_name[] = "/bmp_memory";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        length = snprintf(log_msg, 64, "Error opening shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        bmp_destroy(bmp);
        exit(1);
    }

    // Set the size of the shm:
    if (ftruncate(shm_fd, shm_size) < 0)
    {
        length = snprintf(log_msg, 64, "Error truncating shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        close(shm_fd);
        shm_unlink(shm_name);
        bmp_destroy(bmp);
        exit(1);
    }

    // Open semaphore:
    char sem_name[] = "/bmp_sem";
    sem_t *sem_id = sem_open(sem_name, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (sem_id == SEM_FAILED)
    {
        length = snprintf(log_msg, 64, "Error opening semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        close(shm_fd);
        shm_unlink(shm_name);
        bmp_destroy(bmp);
        exit(1);
    }

    // Initialize semaphore:
    if (sem_init(sem_id, 1, 0) < 0)
    {
        length = snprintf(log_msg, 64, "Error initializing semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        close(shm_fd);
        shm_unlink(shm_name);
        bmp_destroy(bmp);
        sem_close(sem_id);
        sem_unlink(sem_name);
        exit(1);
    }

    // Map the shm:
    rgb_pixel_t *ptr = (rgb_pixel_t *)mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        length = snprintf(log_msg, 64, "Error mapping shared memory: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        bmp_destroy(bmp);
        close(shm_fd);
        shm_unlink(shm_name);
        sem_close(sem_id);
        sem_unlink(sem_name);
        exit(1);
    }

    // Copy bitmap to shm:
    save_bmp(bmp, ptr);

    // Tell the process B that writing is done:
    if (sem_post(sem_id) < 0)
    {
        length = snprintf(log_msg, 64, "Error posting to semaphore: %d.\n", errno);
        if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
            perror("Error writing to log (A)");
        bmp_destroy(bmp);
        close(shm_fd);
        shm_unlink(shm_name);
        sem_close(sem_id);
        sem_unlink(sem_name);
        exit(1);
    }

    // Infinite loop
    while (!finish)
    {
        // Get input in non-blocking mode
        int cmd = getch();

        // If user resizes screen, re-draw UI...
        if (cmd == KEY_RESIZE)
        {
            if (first_resize)
            {
                first_resize = FALSE;
            }
            else
            {
                reset_console_ui();
            }
        }

        // Else, if user presses print button...
        else if (cmd == KEY_MOUSE)
        {
            if (getmouse(&event) == OK)
            {
                if (check_button_pressed(print_btn, &event))
                {
                    // Save snapshot:
                    bmp_save(bmp, "./out/snapshot.bmp");

                    mvprintw(LINES - 1, 1, "Print button pressed");
                    refresh();
                    sleep(1);
                    for (int j = 0; j < COLS - BTN_SIZE_X - 2; j++)
                    {
                        mvaddch(LINES - 1, j, ' ');
                    }
                }
                else if (check_button_pressed(exit_btn, &event))
                {
                    finish = 1;
                    kill(pid_b, SIGTERM);
                }
            }
        }

        // If input is an arrow key, move circle accordingly...
        else if (cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN)
        {
            move_circle(cmd);
            draw_circle();
            // Destroy bitmap:
            bmp_destroy(bmp);
            // Create new bitmap:
            bmp = bmp_create(WIDTH, HEIGHT, 4);
            if (bmp == NULL) {
                length = snprintf(log_msg, 64, "Error creating bitmap: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                finish = 1;
            }
            // Draw new bitmap:
            draw_bmp(bmp, get_circle_x() * SCALE, get_circle_y() * SCALE);
            // Copy bitmap to shm:
            save_bmp(bmp, ptr);
            // Tell the process B that writing is done:
            if (sem_post(sem_id) < 0)
            {
                length = snprintf(log_msg, 64, "Error posting to semaphore: %d.\n", errno);
                if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
                    perror("Error writing to log (A)");
                finish = 1;
            }
        }
    }

    // Termination:
    length = snprintf(log_msg, 64, "Exited successfully.");
    if (write_log(fd_log, log_msg, length) < 0 && errno != EINTR)
        perror("Error writing to log (A)");
    bmp_destroy(bmp);
    close(shm_fd);
    shm_unlink(shm_name);
    sem_close(sem_id);
    sem_unlink(sem_name);

    endwin();
    return 0;
}
