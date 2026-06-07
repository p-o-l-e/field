#include "field/colours.h"
#include "field/field.h"
#include "field_glfw.h"
#include "field/image_load.h"
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

static field context;

#define TARGET_FPS 60.0
#define WIDTH  800
#define HEIGHT 600

double last_time = 0;

void button_call()
{
    static int t;
    t++;
    printf("BUTTON PRESSED: %d times\n", t);
}

void timer_callback(union sigval) 
{
    pthread_cond_signal(&_repaint_condition);
    //printf("\n[TIMER EXPIRED] 3 seconds passed in the background!\n");
}

int main(int, char**)
{
    timer_t timer_id;
    struct sigevent sev;
    struct itimerspec its;

    sev = (struct sigevent){0};
    its = (struct itimerspec){0};


    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_callback;
    sev.sigev_value.sival_ptr = &timer_id;


    if (timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1) 
    {
        perror("timer_create");
        return 1;
    }

    its.it_value.tv_sec     = 1;
    its.it_value.tv_nsec    = 0;
    its.it_interval.tv_sec  = 1;
    its.it_interval.tv_nsec = 0;


    if (timer_settime(timer_id, 0, &its, NULL) == -1) {
        perror("timer_settime");
        timer_delete(timer_id);
        return 1;
    }

    {
        init_field (&context, 0, 0, WIDTH, HEIGHT, 16, ROOT);
        createSector(&context,  1, NODE    , 10,  10, 300, 200, MOVEABLE);
        
        createSector(&context,  2, SLIDER, 20, 60, 20, 80, MOVEABLE | VERTICAL);
        context.at[2].range = 8;

        createSector(&context,  3, STEP_SLIDER, 50, 60, 20, 80, MOVEABLE | VERTICAL);
        context.at[3].range = 4;

        createSector(&context,  4, SLIDER, 80, 60, 180, 10, MOVEABLE);
        context.at[4].range = 8;

        createSector(&context,  5, STEP_SLIDER, 80, 80, 180, 10, MOVEABLE);
        context.at[5].range = 4;
        
        createSector(&context,  6, BUTTON, 20, 160, 20, 20, 0);
        context.at[6].callback = &button_call;

        createSector(&context,  7, CHECKBOX, 50, 160, 10, 10, 0);
        createSector(&context,  8, CHECKBOX, 70, 160, 10, 10, 0);
        createSector(&context,  9, SOCKET  , 90, 160, 10, 10, MOVEABLE);

        link_sector(&context.at[1], &context.at[2]);
        
    }

    field node;
    {
   //     frame_clr(node.bg, YELLOW);

    }

    pthread_mutex_init(&_screen_lock, NULL);
    pthread_mutex_init(&_main_lock, NULL);

    pthread_barrier_init(&_init_barrier, NULL, 2);

    pthread_create(&field_thread, NULL, draw_field, &context);
    pthread_create(&event_thread, NULL, events_process, NULL);

    pthread_barrier_destroy(&_init_barrier);
   

    frame_clr(context.layer[BG], BACKGROUND);

    draw_text_label(context.layer[BG], gtFont, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 20, 20, 0, 0, TEXT);
    draw_text_label(context.layer[BG], gtFont, "abcdefghijklmnopqrstuvwxyz", 20, 30, 0, 0, TEXT);
    draw_text_label(context.layer[BG], gtFont, "0123456789!#$^&*()[]{}/:;,", 20, 40, 0, 0, TEXT);

    while (_switch_on)
    {   
        pthread_mutex_lock(&_main_lock);
        pthread_cond_wait(&_escape_condition, &_main_lock);
        pthread_mutex_unlock(&_main_lock);
    }

    pthread_barrier_init(&_exit_barrier, NULL, 3);
    pthread_barrier_wait(&_exit_barrier);
    pthread_barrier_destroy(&_exit_barrier);

    printf("Main Terminated...\n");
    pthread_mutex_destroy(&_screen_lock);
    pthread_mutex_destroy(&_main_lock);

    timer_delete(timer_id);

    exit(EXIT_SUCCESS);
    return 0;
}
