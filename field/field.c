#include "field.h"
#include "colours.h"
#include "constraints.h"
#include <stdint.h>
#include <stdio.h>


/*****************************************************************************************************************************/

void init_sector(field* o, int pos, sector_type type, int x, int y, int w, int h)
{
    o->at[pos].type      = type;
    o->at[pos].bounds.l  = x;
    o->at[pos].bounds.t  = y;
    o->at[pos].bounds.r  = x + w;
    o->at[pos].bounds.b  = y + h;
    o->at[pos].value     = 0.0f;
    o->at[pos].range     = 4096.0f;
    o->at[pos].step      = 1.0f;
    o->at[pos].repaint   = true;
    o->at[pos].callback  = &empty;

    draw_rectangle_filled
    (
        &o->stencil,
        o->at[pos].bounds.l,
        o->at[pos].bounds.t, 
        o->at[pos].bounds.r, 
        o->at[pos].bounds.b, 
        o->at[pos].id
    );
}

void init_field(field* o, uint32_t x, uint32_t y, uint32_t w, uint32_t h, unsigned size)
{
    o->bounds.l     = x;
    o->bounds.t     = y;
    o->bounds.r     = x + w;
    o->bounds.b     = y + h;

    o->drag         = false;
    o->move         = false;
    o->count        = size + 1u;
    o->repaint      = true;
    o->refresh      = true;

    frame_init(&o->canvas,  w, h);
    frame_init(&o->stencil, w, h);
    frame_init(&o->bg, w, h);
    frame_clr (&o->stencil, 0x00);

    o->at = malloc(o->count * sizeof(sector));
    for(int i = 0; i < o->count; i++) o->at[i].id = i;

    o->at[0].type = CANVAS;
    o->at[0].callback = &empty;
}

void destroy_field(field* o)
{
    frame_flush(&o->canvas);
    frame_flush(&o->stencil);
    free(o->at);
}

void empty(){};

/*****************************************************************************************************************************/

void hit_test_down(field* o, int x, int y, mouse_button button)
{
    o->current = frame_get(&o->stencil, x, y);

    if (o->at[o->current].dragable)
    {
        if (button == LMB) o->drag = true;
    }
    else 
    {
        set_sector[o->at[o->current].type](o, x, y);
        o->at[o->current].callback();
    }
    o->repaint = true;
    o->refresh = true;
    o->prior = o->current;
}

void hit_test(field* o, int x, int y)
{
    auto uid = frame_get(&o->stencil, x, y);

    if(o->current != uid) 
    {
        o->prior = o->current;
        o->current = uid;
        
        o->at[o->prior].hovered = false;
        o->at[o->prior].repaint = true;
        
        o->at[o->current].hovered = true;
        o->at[o->current].repaint = true;
    }

    o->repaint = true;
    o->refresh = true;
}

void hit_test_drag(field* o, int x, int y)
{
    drag_sector[o->at[o->current].type](o, x, y);
    o->repaint = true;
    o->refresh = true;
}

void hit_test_up(field* o, int x, int y, mouse_button button)
{
    o->current = frame_get(&o->stencil, x, y);
    leave_sector[o->at[o->prior].type](o, x, y);
    o->drag    = false;
    o->move    = false;
    o->repaint = true;
    o->refresh = true;
}

/*****************************************************************************************************************************/

void draw_h_slider(field* o, sector* c)
{
    int gap     = constraints[1];
    int half    = constraints[0];

    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    int w   = c->bounds.r - c->bounds.l - half * 2 - gap * 2;
    int pos = (int)(c->value/c->range * (float)w) + c->bounds.l + half + gap;
    draw_rectangle_filled(&o->canvas, pos - half, c->bounds.t + gap, pos + half, c->bounds.b - gap, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_v_slider(field* o, sector* c)
{
    auto gap = constraints[1];
    auto half = constraints[0];

    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    int w   = c->bounds.b - c->bounds.t - half * 2 - gap * 2;
    int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + half + gap;
    draw_rectangle_filled(&o->canvas, c->bounds.l + gap, pos - half, c->bounds.r - gap, pos + half, SELECTIONBACKGROUND);
    c->repaint = false;
}


void draw_checkbox(field* o, sector* c)
{
    int gap = constraints[1];

    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    if(c->value > 0.5f)
        draw_rectangle_filled(&o->canvas, c->bounds.l + gap, c->bounds.t + gap, c->bounds.r - gap, c->bounds.b - gap, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_scene(field* o)
{

    printf("Draw scene:\n");
    for(int i = 0; i < o->count; i++)
    {
        if(o->at[i].repaint)
        {
            draw_sector[o->at[i].type](o, &o->at[i]);
            printf("-- Repaint: %d\n", i);
        }
    }
}


/*****************************************************************************************************************************/

void set_checkbox(field* o, int x, int y)
{
    if (o->at[o->current].value < 0.5f) o->at[o->current].value = 1.0f;
    else o->at[o->current].value = 0.0f;
    o->at[o->current].repaint = true;
}

void set_h_slider(field* o, int x, int y)
{
    o->at[o->current].dragable = true; // On init
    int xs = x - o->at[o->current].bounds.l;

    o->at[o->current].value = (float)xs/(float)(o->at[o->current].bounds.r - o->at[o->current].bounds.l)*o->at[o->current].range;
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;
}


void set_v_slider(field* o, int x, int y)
{
    o->at[o->current].dragable = true; // On init
    int ys = y - o->at[o->current].bounds.t;

    o->at[o->current].value = o->at[o->current].range - (float)ys/(float)(o->at[o->current].bounds.b - o->at[o->current].bounds.t)*o->at[o->current].range;
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;
}

void set_v_step_slider(field* o, int x, int y)
{
    int ys = y - o->at[o->current].bounds.t;

    o->at[o->current].value = roundf(o->at[o->current].range - (float)ys/(float)(o->at[o->current].bounds.b - o->at[o->current].bounds.t)*o->at[o->current].range);
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;

    o->at[o->current].repaint = true;
}

void draw_v_step_slider(field* o, sector* c)
{
    int gap     = constraints[1];
    int half    = (int)((float)(c->bounds.b - c->bounds.t - gap*2)/(float)(c->range+1)/2.0f);

    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    int w   = c->bounds.b - c->bounds.t - half*2 - gap*2;
    int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + half + gap;
    draw_rectangle_filled(&o->canvas, c->bounds.l + gap, pos - half, c->bounds.r - gap, pos + half, SELECTIONBACKGROUND);

    c->repaint = false;
}

void set_h_step_slider(field* o, int x, int y)
{
    int xs = x - o->at[o->current].bounds.l;

    o->at[o->current].value = roundf((float)xs/(float)(o->at[o->current].bounds.r - o->at[o->current].bounds.l)*o->at[o->current].range);
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;
}

void draw_h_step_slider(field* o, sector* c)
{
    int gap     = constraints[1];
    int half    = (int)((float)(c->bounds.r - c->bounds.l - gap*2)/(float)(c->range+1)/2.0f);

    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    int w   = c->bounds.r - c->bounds.l - half*2 - gap*2;
    int pos = (int)(c->value/c->range * (float)w) + c->bounds.l + half + gap;
    draw_rectangle_filled(&o->canvas, pos - half, c->bounds.t + gap, pos + half, c->bounds.b - gap, SELECTIONBACKGROUND);

    c->repaint = false;
}


void scroll_slider(field* o, int x, int y)
{
    o->at[o->current].value += (float) y * o->at[o->current].step;
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;
}

void scroll_step_slider(field* o, int x, int y)
{
    o->at[o->current].value += (float) y;
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;
}


void draw_h_progress_bar(field* o, sector* c)
{
    int gap     = constraints[1];
    int half    = constraints[0];

    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    int w   = c->bounds.r - c->bounds.l - gap;
    int pos = (int)(c->value/c->range * (float)w) + c->bounds.l;
    draw_rectangle_filled(&o->canvas, c->bounds.l + gap, c->bounds.t + gap, pos, c->bounds.b - gap, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_v_progress_bar(field* o, sector* c)
{
    int gap = constraints[1];
    int half = constraints[0];

    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    int w   = c->bounds.b - c->bounds.t - gap;
    int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + gap;
    draw_rectangle_filled(&o->canvas, c->bounds.l + gap, pos, c->bounds.r - gap, c->bounds.b - gap, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_button(field* o, sector* c)
{
    int gap = constraints[1];

    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);

    auto colour = c->hovered ? ACTIVE : BUTTONS;
    printf("Draw Button\n");

    draw_rectangle_filled(&o->canvas, c->bounds.l + gap, c->bounds.t + gap, c->bounds.r - gap, c->bounds.b - gap, colour);
    if(c->value > 0.5f)
        draw_rectangle_filled(&o->canvas, c->bounds.l + gap, c->bounds.t + gap, c->bounds.r - gap, c->bounds.b - gap, ACCENT);
    c->repaint = false;
}

void draw_sprite_slider(field* o, sector* c)
{
    sprite* temp = (sprite*)c->data;
    int f = (c->value / c->range) * temp->nframes;
    frame_copy_at(&o->canvas, &temp->data[f], c->bounds.l, c->bounds.t);

    c->repaint = false;
}


void set_sprite_slider(field* o, int x, int y)
{
    int dy = roundf((o->at[o->current].lp.y - y)/o->at[o->current].step);
    int dx = roundf((x - o->at[o->current].lp.x)/o->at[o->current].step);

    o->at[o->current].value += (dy + dx);

    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;

    o->at[o->current].lp.x = (float)x;
    o->at[o->current].lp.y = (float)y;
}


void set_sprite_inf_slider(field* o, int x, int y)
{
    int dy = roundf((y - o->at[o->current].lp.y)/o->at[o->current].step);
    int dx = roundf((o->at[o->current].lp.x - x)/o->at[o->current].step);

    o->at[o->current].value += (dy + dx);

    if (o->at[o->current].value < 0.0f) o->at[o->current].value += o->at[o->current].range;
    else if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value -= o->at[o->current].range;

    // if (o->at[o->current].value <= 0.0f) o->at[o->current].value = o->at[o->current].range;
    // else if (o->at[o->current].value >= o->at[o->current].range) o->at[o->current].value = 0.0f;

    o->at[o->current].repaint = true;
    o->at[o->current].lp.x = (float)x;
    o->at[o->current].lp.y = (float)y;
}

void set_button(field* o, int x, int y)
{
    if  (o->at[o->current].value < 0.5f) o->at[o->current].value = 1.0f;
    else o->at[o->current].value = 0.0f;
    o->at[o->current].repaint = true;
    o->prior = o->current;
}

void release_button(field* o, int x, int y)
{
    if  (o->at[o->prior].value < 0.5f) o->at[o->prior].value = 1.0f;
    else o->at[o->prior].value = 0.0f;
    o->at[o->prior].repaint = true;    
}

void draw_sprite_button(field* o, sector* c)
{
    sprite* temp = (sprite*)c->data;

    if(c->value > 0.5f)
        frame_copy_at(&o->canvas, &temp->data[1], c->bounds.l, c->bounds.t);
    else
        frame_copy_at(&o->canvas, &temp->data[0], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

void draw_socket(field* o, sector* c)
{
    sprite* temp = (sprite*)c->data;
    frame_copy_at(&o->canvas, &temp->data[0], c->bounds.l, c->bounds.t);
    c->repaint = false;
}

void drag_socket(field* o, int x, int y)
{
    frame_pset(&o->canvas, x, y, 0xFFFFFFFF);
}


void drag_title_bar(field* o, int x, int y)
{
    o->move = true;
    o->drag = false;
}

void draw_title_bar(field* o, sector* c)
{
    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
}


void draw_canvas(field* o, sector* c) {}

void set_none(field* o, int x, int y) {}


/*****************************************************************************************************************************/

void (*set_sector[])(field*, int, int) = 
{
    set_h_slider,
    set_v_slider,
    set_h_step_slider,
    set_v_step_slider,
    set_h_slider,
    set_v_slider,
    set_sprite_slider,
    set_sprite_inf_slider,
    set_none, // Set none
    set_none,
    set_checkbox,
    set_button,
    set_checkbox,
    set_button,
    set_none
};

void (*draw_sector[])(field*, sector*) =
{
    draw_h_slider,
    draw_v_slider,
    draw_h_step_slider,
    draw_v_step_slider,
    draw_h_progress_bar,
    draw_v_progress_bar,
    draw_sprite_slider, // Sprite
    draw_sprite_slider, // Sprite
    draw_socket,
    draw_title_bar,
    draw_checkbox,
    draw_button,
    draw_sprite_button,
    draw_sprite_button,
    draw_canvas
};

void (*drag_sector[])(field*, int, int) =
{
    set_h_slider,
    set_v_slider,
    set_h_step_slider,
    set_v_step_slider,
    set_h_slider,
    set_v_slider,
    set_sprite_slider,
    set_sprite_inf_slider,   
    drag_socket, // Socket
    drag_title_bar,
    set_none,
    set_none,
    set_none,
    set_none,
    set_none
};

void (*scroll_sector[])(field*, int, int) =
{
    scroll_slider,
    scroll_slider,
    scroll_step_slider,
    scroll_step_slider,
    scroll_slider,
    scroll_slider,
    scroll_slider,
    scroll_slider,
    set_none, // Socket
    set_none,
    set_none,
    set_none,
    set_none,
    set_none,
    set_none
};

void (*leave_sector[])(field*, int, int) =
{
    set_none,
    set_none,
    set_none,
    set_none,
    set_none,
    set_none,
    set_none,
    set_none,
    set_none, // Socket
    set_none,
    set_none,
    release_button,
    set_none,
    release_button,
    set_none
};
