#include "field/colours.h"
#include "field/field.h"
#include "field_glfw.h"
#include "field/image_load.h"
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

static field context;
static sprite button;

#define TARGET_FPS 60.0
#define WIDTH  1280
#define HEIGHT 720

double last_time = 0;

static void slider_call(sector* slider, sector* tbox)
{
    char *text = tbox->data;
    float val = slider->value[CP_COARSE] + slider->value[CP_FINE];

    snprintf(text, 16, "%.3f", val);
    tbox->repaint = true;

    //printf("Slider callback\n");
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

    
    /*****************************************************************************************************************************/
    initField (&context, 0, 0, WIDTH, HEIGHT, 36, ROOT);
    auto node = createSector(&context, nullptr, ST_NODE    , 0,  0, 299, 199, MOVEABLE);
    
    auto slider = createSector(&context, node, ST_SLIDER, 20, 40, 20, 80, MOVEABLE | VERTICAL);
    slider->step[CP_COARSE] = 1.3f;
    slider->step[CP_FINE] = 0.013f;
    slider->range[0] = -5.0f;
    slider->range[1] = 5.0f;

    auto vs_slider = createSector(&context, node, ST_SLIDER, 50, 40, 20, 80, MOVEABLE | VERTICAL);

    createSector(&context, node, ST_SLIDER, 80, 40, 180, 10, MOVEABLE);
    //context.at[4].range = 8;

    auto hs_slider = createSector(&context, node, ST_SLIDER, 80, 60, 180, 10, MOVEABLE);
    
    createSector(&context, node, ST_BUTTON, 20, 160, 20, 20, 0);

    createSector(&context, node, ST_CHECKBOX, 50, 140, 10, 10, 0);
    createSector(&context, node, ST_CHECKBOX, 70, 140, 10, 10, 0);
    createSector(&context, node, ST_SOCKET  , 280, 100, 10, 10, MOVEABLE | INTERCON | OUTPUT);
    createSector(&context, node, ST_SOCKET  , 280, 120, 10, 10, MOVEABLE | INTERCON | OUTPUT);
    createSector(&context, node, ST_SOCKET  , 280, 140, 10, 10, MOVEABLE | INTERCON | OUTPUT);
    auto tbox = createSector(&context, node, ST_TEXTBOX, 15, 15, 60, 10, 0);

    add_mod_link(slider, tbox, CT_VALUE, slider_call);

    
    auto cbox = createSector(&context, node, ST_SPRITE_BUTTON, 280, 4, 16, 16, 0);



    auto node2 = createSector(&context, nullptr, ST_NODE    , 325,  25, 199, 199, MOVEABLE);
    createSector(&context, node2, ST_SOCKET  , 330, 80, 10, 10, MOVEABLE | INTERCON | INPUT);
    createSector(&context, node2, ST_SOCKET  , 330, 100, 10, 10, MOVEABLE | INTERCON | INPUT);
    createSector(&context, node2, ST_SOCKET  , 330, 120, 10, 10, MOVEABLE | INTERCON | INPUT);
    createSector(&context, node2, ST_BUTTON, 330, 160, 20, 20, 0);


    sprite_init(&button, 16, 16, 2);
    load_png_rgba("resin_knob_16_y.png", &button.data[1]);
    load_png_rgba("resin_knob_16_off.png", &button.data[0]);

    cbox->data = &button;
    /*****************************************************************************************************************************/

    pthread_mutex_init(&_screen_lock, NULL);
    pthread_mutex_init(&_main_lock, NULL);

    pthread_barrier_init(&_init_barrier, NULL, 2);

    pthread_create(&field_thread, NULL, draw_field, &context);
    pthread_create(&event_thread, NULL, events_process, NULL);

    pthread_barrier_destroy(&_init_barrier);
   

    frame_fill(context.layer[BG], BACKGROUND);

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
