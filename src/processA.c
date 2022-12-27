#include "./../include/processA_utilities.h"
#include <bmpfile.h>

#define MAX_WIDTH 1600
#define MAX_HEIGHT 600
#define SCALE 20

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

int main(int argc, char *argv[])
{
    // Utility variable to avoid trigger resize event on launch
    int first_resize = TRUE;

    // Initialize UI
    init_console_ui();

    // Draw initial state:
    bmpfile_t *bmp;
    bmp = bmp_create(MAX_WIDTH, MAX_HEIGHT, 4);
    draw_bmp(bmp, get_circle_x() * SCALE, get_circle_y() * SCALE);

    // TODO: post to shared memory with semaphore.

    // Infinite loop
    while (TRUE)
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
                // if(check_button_pressed(print_btn, &event)) {
                //     mvprintw(LINES - 1, 1, "Print button pressed");
                //     refresh();
                //     sleep(1);
                //     for(int j = 0; j < COLS - BTN_SIZE_X - 2; j++) {
                //         mvaddch(LINES - 1, j, ' ');
                //     }
                // }
                bmp_save(bmp, "./out/snapshot.bmp");
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

            // TODO: post to shared memory with semaphore.
        }
    }
    
    endwin();
    return 0;
}
