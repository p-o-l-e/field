#include "field.h"
#include "colours.h"
#include "constraints.h"
#include "containers.h"
#include "curves.h"
#include <stdint.h>
#include <stdio.h>
#include <math.h>

/*****************************************************************************************************************************/

void init_sector(field* o, int pos, sector_type type, int x, int y, int w, int h, uint32_t flags)
{
    o->at[pos].type         = type;
    o->at[pos].bounds.l     = x;
    o->at[pos].bounds.t     = y;
    o->at[pos].bounds.r     = x + w;
    o->at[pos].bounds.b     = y + h;
    o->at[pos].value        = 0.0f;
    o->at[pos].range        = 4096.0f;
    o->at[pos].step         = 1.0f;
    o->at[pos].repaint      = true;
    o->at[pos].callback     = &empty;
    o->at[pos].flag         = flags;

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
    for(uint32_t i = 0; i < o->count; i++) o->at[i].id = i;

    o->at[0].type = CANVAS;
    o->at[0].callback = &empty;
}

void destroy_field(field* o)
{
    frame_flush(&o->canvas);
    frame_flush(&o->stencil);
    free(o->at);
}

void empty(){}

/*****************************************************************************************************************************/

void hit_test_down(field* o, int x, int y, mouse_button button)
{
    o->current = frame_get(&o->stencil, x, y);

    if (o->at[o->current].flag & MOVEABLE)
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

    switch (button) {
        case LMB:
            break;
        case MMB:
            break;
        case RMB:
            break;
        default:
            break; 
    } 
}

/*****************************************************************************************************************************/

void draw_slider(field* o, sector* c)
{
    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);

    if(c->flag & VERTICAL)
    {
        int w   = c->bounds.b - c->bounds.t - GRIP * 2 - GAP * 2;
        int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + GRIP + GAP;
        draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, pos - GRIP, c->bounds.r - GAP, pos + GRIP, SELECTIONBACKGROUND);
    }
    else 
    {
        int w   = c->bounds.r - c->bounds.l - GRIP * 2 - GAP * 2;
        int pos = (int)(c->value/c->range * (float)w) + c->bounds.l + GRIP + GAP;
        draw_rectangle_filled(&o->canvas, pos - GRIP, c->bounds.t + GAP, pos + GRIP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }

    c->repaint = false;
}

void draw_checkbox(field* o, sector* c)
{
    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    if(c->value > 0.5f)
        draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_scene(field* o)
{
    printf("Draw scene:\n");
    for(uint32_t i = 0; i < o->count; i++)
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

    if(false) printf("[%d : %d]", x, y);
}

void set_slider(field* o, int x, int y)
{
    if(o->at[o->current].flag & VERTICAL)
    {
        int ys = y - o->at[o->current].bounds.t;
        o->at[o->current].value = o->at[o->current].range - (float)ys/(float)(o->at[o->current].bounds.b - o->at[o->current].bounds.t)*o->at[o->current].range;
    }
    else 
    {
        int xs = x - o->at[o->current].bounds.l;
        o->at[o->current].value = (float)xs/(float)(o->at[o->current].bounds.r - o->at[o->current].bounds.l)*o->at[o->current].range;
    }

    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;
}

void set_step_slider(field* o, int x, int y)
{
    if(o->at[o->current].flag & VERTICAL)
    {
        int ys = y - o->at[o->current].bounds.t;
        o->at[o->current].value = roundf(o->at[o->current].range - (float)ys/(float)(o->at[o->current].bounds.b - o->at[o->current].bounds.t)*o->at[o->current].range);
    }
    else 
    {
        int xs = x - o->at[o->current].bounds.l;
        o->at[o->current].value = roundf((float)xs/(float)(o->at[o->current].bounds.r - o->at[o->current].bounds.l)*o->at[o->current].range);
    }

    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;

    o->at[o->current].repaint = true;
}

void draw_step_slider(field* o, sector* c)
{
    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);

    if(c->flag & VERTICAL)
    {
        int w   = c->bounds.b - c->bounds.t - GRIP * 2 - GAP * 2;
        int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + GRIP + GAP;
        draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, pos - GRIP, c->bounds.r - GAP, pos + GRIP, SELECTIONBACKGROUND);
    }
    else 
    {
        int w   = c->bounds.r - c->bounds.l - GRIP * 2 - GAP  *2;
        int pos = (int)(c->value/c->range * (float)w) + c->bounds.l + GRIP + GAP;
        draw_rectangle_filled(&o->canvas, pos - GRIP, c->bounds.t + GAP, pos + GRIP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }

    c->repaint = false;
}

void scroll_slider(field* o, int x, int y)
{
    o->at[o->current].value += (float) y * o->at[o->current].step;
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;
    
    if(false) printf("[%d : %d]", x, y);
}

void scroll_step_slider(field* o, int x, int y)
{
    o->at[o->current].value += (float) y;
    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;

    if(false) printf("[%d : %d]", x, y);
}

void draw_progress_bar(field* o, sector* c)
{
    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);

    if(o->at[o->current].flag & VERTICAL)
    {
        int w   = c->bounds.b - c->bounds.t - GAP;
        int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + GAP;
        draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, pos, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }
    else 
    {
        int w   = c->bounds.r - c->bounds.l - GAP;
        int pos = (int)(c->value/c->range * (float)w) + c->bounds.l;
        draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, c->bounds.t + GAP, pos, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }

    c->repaint = false;
}

void draw_button(field* o, sector* c)
{
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);

    auto colour = c->hovered ? ACTIVE : BUTTONS;
    printf("Draw Button\n");

    draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, colour);
    if(c->value > 0.5f)
        draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACCENT);
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

void set_button(field* o, int, int)
{
    if  (o->at[o->current].value < 0.5f) o->at[o->current].value = 1.0f;
    else o->at[o->current].value = 0.0f;
    o->at[o->current].repaint = true;
    o->prior = o->current;
}

void release_button(field* o, int, int)
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

void set_socket(field* o, int, int)
{
    if  (o->at[o->current].value < 0.5f) o->at[o->current].value = 1.0f;
    else o->at[o->current].value = 0.0f;
    o->at[o->current].repaint = true;
    o->prior = o->current;
}

void draw_socket(field* o, sector* c)
{
    draw_rectangle_filled(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, BACKGROUND);
    draw_rectangle(&o->canvas, c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    if(c->value > 0.5f)
        draw_rectangle_filled(&o->canvas, c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

void drag_socket(field* o, int x, int y)
{
    frame_pset(&o->canvas, x, y, 0xFFFFFFFF);

    float xe = (float)x;
    float ye = (float)y;

    point a, b, c, d;

    float xo = a.x;
    float yo = a.y;

    b.y = (ye - yo) * (2.0f/3.0f) + yo;
    c.y = (ye - yo) * (1.0f/3.0f) + yo;

    if(xe < xo)
    {
        b.x = fabs(xo - xe) * 0.999f + xe;
        c.x = fabs(xo - xe) * 0.001f + xe;
    }

    if(xe > xo)
    {
        b.x = fabs(xo - xe) * 0.001f + xo;
        c.x = fabs(xo - xe) * 0.999f + xo;
    }

    d.x = xe;
    d.y = ye;

    int iterations = 16;
    const float inc = 1.04f / (float)iterations ;
    float t = 0.0;

    for(int i = 0; i < iterations; i++)
    {
        point carry = interpolate_bezier(a, b, c, d, t);
        //data[i].x = carry.x;
        //data[i].y = carry.y;
        t += inc;
    }
}

void draw_canvas(field* o, sector*) 
{
    if(true)
    {
        uint32_t step = 25;

        for(uint32_t x = step; x < o->canvas.width; x += step)
        {
            for(uint32_t y = 0; y < o->canvas.height; ++y)
            {
                if(!frame_get(&o->stencil, x, y))
                    frame_pset(&o->canvas, x, y, DIMMED);
            }
        }

        for(uint32_t y = step; y < o->canvas.height; y += step)
        {
            for(uint32_t x = 0; x < o->canvas.width; ++x)
            {
                if(!frame_get(&o->stencil, x, y))
                    frame_pset(&o->canvas, x, y, DIMMED);
            }
        }
    }
}

void set_none(field*, int, int) {}

/*****************************************************************************************************************************/

void (*set_sector[])(field*, int, int) = 
{
    set_slider,             //SLIDER, 
    set_step_slider,        //STEP_SLIDER, 
    set_slider,             //PROGRESS_BAR, 
    set_sprite_slider,      //SPRITE_SLIDER,
    set_sprite_inf_slider,  //SPRITE_INF_SLIDER,
    set_socket,             //SOCKET,
    set_checkbox,           //CHECKBOX, 
    set_button,             //BUTTON, 
    set_checkbox,           //SPRITE_CHECKBOX, 
    set_button,             //SPRITE_BUTTON, 
    set_none                //CANVAS            
};

void (*draw_sector[])(field*, sector*) =
{
    draw_slider,            //SLIDER, 
    draw_step_slider,       //STEP_SLIDER, 
    draw_progress_bar,      //PROGRESS_BAR, 
    draw_sprite_slider,     //SPRITE_SLIDER,
    draw_sprite_slider,     //SPRITE_INF_SLIDER,
    draw_socket,            //SOCKET,
    draw_checkbox,          //CHECKBOX, 
    draw_button,            //BUTTON, 
    draw_sprite_button,     //SPRITE_CHECKBOX, 
    draw_sprite_button,     //SPRITE_BUTTON, 
    draw_canvas             //CANVAS                
};

void (*drag_sector[])(field*, int, int) =
{
    set_slider,             //SLIDER, 
    set_step_slider,        //STEP_SLIDER, 
    set_slider,             //PROGRESS_BAR, 
    set_sprite_slider,      //SPRITE_SLIDER,
    set_sprite_inf_slider,  //SPRITE_INF_SLIDER,     
    drag_socket,            //SOCKET,
    set_none,               //CHECKBOX, 
    set_none,               //BUTTON, 
    set_none,               //SPRITE_CHECKBOX, 
    set_none,               //SPRITE_BUTTON, 
    set_none                //CANVAS                
};

void (*scroll_sector[])(field*, int, int) =
{
    scroll_slider,          //SLIDER, 
    scroll_step_slider,     //STEP_SLIDER, 
    scroll_slider,          //PROGRESS_BAR, 
    scroll_slider,          //SPRITE_SLIDER,
    scroll_slider,          //SPRITE_INF_SLIDER,
    set_none,               //SOCKET,
    set_none,               //CHECKBOX, 
    set_none,               //BUTTON, 
    set_none,               //SPRITE_CHECKBOX, 
    set_none,               //SPRITE_BUTTON, 
    set_none                //CANVAS                
};

void (*leave_sector[])(field*, int, int) =
{
    set_none,               //SLIDER, 
    set_none,               //STEP_SLIDER, 
    set_none,               //PROGRESS_BAR, 
    set_none,               //SPRITE_SLIDER,
    set_none,               //SPRITE_INF_SLIDER,
    set_none,               //SOCKET,
    set_none,               //CHECKBOX, 
    release_button,         //BUTTON, 
    set_none,               //SPRITE_CHECKBOX, 
    release_button,         //SPRITE_BUTTON, 
    set_none                //CANVAS                
};

    
    
    
    
    
    
    
    
    
    
    
    
