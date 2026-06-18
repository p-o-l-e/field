#include "field/field.h"
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
    
    auto mname = createSector(&context, node, ST_TEXTBOX, 10, 10, 60, 10, TRANSPARENT);
    set_text_data(mname, "VCO");

    auto slider = createSector(&context, node, ST_SLIDER, 20, 40, 20, 80, MOVEABLE | VERTICAL);
    slider->step[CP_COARSE] = 1.3f;
    slider->step[CP_FINE] = 0.013f;
    slider->range[0] = -5.0f;
    slider->range[1] = 5.0f;

    auto vs_slider = createSector(&context, node, ST_SLIDER, 50, 40, 20, 80, MOVEABLE | VERTICAL);

    createSector(&context, node, ST_SLIDER, 80, 40, 180, 10, MOVEABLE);
    //context.at[4].range = 8;

    auto hs_slider = createSector(&context, node, ST_SLIDER, 80, 60, 180, 10, MOVEABLE);
    
    createSector(&context, node, ST_MOMENTARY, 20, 160, 20, 20, 0);


    auto R1 = createSector(&context, node, ST_CHECKBOX,  50, 140, 10, 10, 0);
    R1->radio = true;
    R1->radio_id = 1;
    auto R2 = createSector(&context, node, ST_CHECKBOX,  70, 140, 10, 10, 0);
    R2->radio = true;
    R2->radio_id = 1;
    auto R3 = createSector(&context, node, ST_CHECKBOX,  90, 140, 10, 10, 0);
    R3->radio = true;
    R3->radio_id = 1;
    auto R4 = createSector(&context, node, ST_CHECKBOX, 110, 140, 10, 10, 0);
    R4->radio = true;
    R4->radio_id = 1;
   

    createSector(&context, node, ST_SOCKET  , 280, 100, 15, 15, MOVEABLE | INTERCON | OUTPUT);
    createSector(&context, node, ST_SOCKET  , 280, 120, 15, 15, MOVEABLE | INTERCON | OUTPUT);
    createSector(&context, node, ST_SOCKET  , 280, 140, 15, 15, MOVEABLE | INTERCON | OUTPUT);
    auto tbox = createSector(&context, node, ST_TEXTBOX, 50, 10, 60, 10, 0);

    add_mod_link(slider, tbox, CT_VALUE, slider_call);

    auto cbox = createSector(&context, node, ST_SPRITE_BUTTON, 280, 4, 16, 16, 0);

    auto node2 = createSector(&context, nullptr, ST_NODE    , 325,  25, 299, 199, MOVEABLE);
    createSector(&context, node2, ST_SOCKET  , 330,  80, 15, 15, MOVEABLE | INTERCON | INPUT);
    createSector(&context, node2, ST_SOCKET  , 330, 100, 15, 15, MOVEABLE | INTERCON | INPUT);
    createSector(&context, node2, ST_SOCKET  , 330, 120, 15, 15, MOVEABLE | INTERCON | INPUT);
    createSector(&context, node2, ST_MOMENTARY, 330, 30, 20, 20, 0);
    auto ribbon = createSector(&context, node2, ST_SLIDER, 370, 30, 200, 20, MOVEABLE);
    ribbon->subtype = SS_B;
    auto vribbon = createSector(&context, node2, ST_SLIDER, 370, 70, 20, 70, MOVEABLE | VERTICAL);
    vribbon->subtype = SS_B;
    qoi_desc desc;
    // size as int per qoi_decode's signature
    void *pixels = qoi_decode(image_rk16f, (int)sizeof(image_rk16f), &desc, 4);
    if(!pixels) printf("Image load failed!\n");

    auto b_off = load_qoi_to_frame(image_rk16f, (int)sizeof(image_rk16f));
    auto b_y   = load_qoi_to_frame(image_rk16y, (int)sizeof(image_rk16y));

    sprite_init(&button, 16, 16, 2);
    button.data[0] = b_off;
    button.data[1] = b_y;
    
    cbox->data = &button;
    /*****************************************************************************************************************************/

    field_loop(&context);

    timer_delete(timer_id);
    return 0;
}
