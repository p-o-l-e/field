#include "field/field_glfw.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

#define WIDTH  1280
#define HEIGHT 720

double last_time = 0;

const uint8_t image_rk16f[] = {
    #embed "assets/rk16f.qoi"
};

const uint8_t image_rk16y[] = {
    #embed "assets/rk16y.qoi"
};

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

frame load_qoi_to_frame(const uint8_t *image_data, int size)
{
    frame f = { NULL, 0, 0 };

    qoi_desc desc;
    void *pixels = qoi_decode(image_data, size, &desc, 4);

    if (!pixels) {
        return f;
    }

    f.width = desc.width;
    f.height = desc.height;

    uint32_t *out = (uint32_t *)malloc(f.width * f.height * sizeof(uint32_t));
    if (!out) {
        free(pixels);
        return f;
    }
    f.data = out;

    const uint8_t *in = (const uint8_t *)pixels;
    size_t num_pixels = (size_t)f.width * f.height;

    for (size_t i = 0; i < num_pixels; ++i) {
        uint8_t a = in[i * 4 + 0];
        uint8_t b = in[i * 4 + 1];
        uint8_t g = in[i * 4 + 2];
        uint8_t r = in[i * 4 + 3];

        uint32_t px =
            ((uint32_t)r) |
            ((uint32_t)g << 8) |
            ((uint32_t)b << 16) |
            ((uint32_t)a << 24);

        out[i] = px;
    }

    free(pixels);
    return f;
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
    field context;
    sprite button;
    
    initField (&context, 0, 0, WIDTH, HEIGHT, 36, ROOT);
    printf("[MAIN] initField\n");

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

    qoi_desc desc;
    // size as int per qoi_decode's signature
    void *pixels = qoi_decode(image_rk16f, (int)sizeof(image_rk16f), &desc, 4);
    if(!pixels) printf("Image load failed!\n");

    auto b_off = load_qoi_to_frame(image_rk16f, (int)sizeof(image_rk16f));
    auto b_y   = load_qoi_to_frame(image_rk16y, (int)sizeof(image_rk16y));
    button.data[0] = b_off;
    button.data[1] = b_y;
    
    // load_png_rgba("resin_knob_16_y.png", &button.data[1]);
    // load_png_rgba("resin_knob_16_off.png", &button.data[0]);

    cbox->data = &button;
    /*****************************************************************************************************************************/

    field_loop(&context);

    timer_delete(timer_id);
    return 0;
}
