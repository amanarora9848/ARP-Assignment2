#include "./../include/processA_utilities.h"
#include <bmpfile.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>

#define MAX_WIDTH 1600
#define MAX_HEIGHT 600
#define SCALE 20

int finish = 0;

void draw_bmp(bmpfile_t *bmp, int x0, int y0) {
    // Configuration about the circle:
    const int radius = 50;
    rgb_pixel_t pixel = {150, 230, 40, 0}; // Color of the circle (BGRA)

    // Check if x0,y0 are inside boudaries:
    if (x0 < radius) {
        x0 = radius;
    } else if (MAX_WIDTH - x0 < radius) {
        x0 = MAX_WIDTH - radius;
    }
    if (y0 < radius) {
        y0 = radius;
    } else if (MAX_HEIGHT - y0 < radius) {
        y0 = MAX_HEIGHT - radius;
    }

    // Code for drawing a circle of given radius...
    for(int x = -radius; x <= radius; x++) {
        for(int y = -radius; y <= radius; y++) {
        // If distance is smaller, point is within the circle
        if(sqrt(x*x + y*y) < radius) {
            /*
            * Color the pixel at the specified (x,y) position
            * with the given pixel values
            */
            bmp_set_pixel(bmp, x0 + x, y0 + y, pixel);
            }
        }
    }
}

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
    // pid_t pid_b;
    // char *pid_fifo = "./tmp/pid";
    // mkfifo(pid_fifo, 0666);
    // int fd_b = open(pid_fifo, O_RDONLY);
    // if (fd_b < 0 && errno != EINTR)
    //     perror("Error opening cmd-master fifo");
    // char buf[10];
    // if (read(fd_b, buf, 10) < 0) perror("Error reading from cmd fifo (master)");
    // sscanf(buf, "%d", &pid_b);
    // close(fd_b);

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
    bmp = bmp_create(MAX_WIDTH, MAX_HEIGHT, 4);
    draw_bmp(bmp, get_circle_x() * SCALE, get_circle_y() * SCALE);

    // Create shared memory:
    int shm_fd = shm_open("./tmp/bmp_memory", O_CREAT | O_RDWR, 0666);

    // Set the size of the shm:
    ftruncate(shm_fd, sizeof(bmp));

    // Open semaphore:
    sem_t * sem_id = sem_open("./tmp/bmp_sem", O_CREAT, S_IRUSR | S_IWUSR, 1);

    // Initialize semaphore:
    sem_init(sem_id, 1, 0);

    // Map the shm:
    bmpfile_t *ptr = mmap(0, sizeof(bmp), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Copy bitmap to shm:
    memcpy(ptr, bmp, sizeof(bmp));

    // Tell the process B that writing is done:
    sem_post(sem_id);

    // Infinite loop
    while (!finish)
    {
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

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    bmp_save(bmp, "./out/snapshot.bmp");
                }
                else if(check_button_pressed(exit_btn, &event)) {
                    finish = 1;
                    // kill(pid_b, SIGTERM);
                }
            } 
        }

        // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {
            move_circle(cmd);
            draw_circle();
            // Destroy bitmap:
            bmp_destroy(bmp);
            // Create new bitmap:
            bmp = bmp_create(MAX_WIDTH, MAX_HEIGHT, 4);
            // Draw new bitmap:
            draw_bmp(bmp, get_circle_x() * SCALE, get_circle_y() * SCALE);
            // Copy bitmap to shm:
            memcpy(ptr, bmp, sizeof(bmp));
            // Tell the process B that writing is done:
            sem_post(sem_id);
        }
    }
    
    endwin();
    return 0;
}
