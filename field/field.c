#include "field.h"
#include "GLFW/glfw3.h"
#include "colours.h"
#include "constraints.h"
#include "containers.h"
#include "curves.h"
#include "primitives.h"
#include "sdfaabb.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>

/*****************************************************************************************************************************/

sector* createSector(field* o, sector* node, uint32_t pos, SectorType type, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t flags) {
    o->at[pos].type         = type;
    o->at[pos].bounds.l     = x;
    o->at[pos].bounds.t     = y;
    o->at[pos].bounds.r     = x + w;
    o->at[pos].bounds.b     = y + h;
    o->at[pos].width        = w;
    o->at[pos].height       = h;
    o->at[pos].value        = 0.0f;
    o->at[pos].range        = 4096.0f;
    o->at[pos].step         = 1.0f;
    o->at[pos].repaint      = true;
    o->at[pos].flags        = flags;
    o->at[pos].carrier      = o;

    for(uint32_t i = 0; i < CALLBACK_LIMIT; ++i) {
        o->at[pos].callback[i] = &fuse_link;
    }

    draw_ltrb_f (
        o->layer[SC],
        &o->at[pos].bounds,
        o->at[pos].id
    );

    init_sector[type](&o->at[pos]);

    if(node)
        link_sector(node, &o->at[pos]);

    return &o->at[pos];
}

void init_field(field* o, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t size, uint32_t flags) {
    o->bounds.l     = x;
    o->bounds.t     = y;
    o->bounds.r     = x + w;
    o->bounds.b     = y + h;

    o->width        = w;
    o->height       = h;

    o->drag         = false;
    o->move         = false;
    o->count        = size + 1u;
    o->repaint      = true;
    o->refresh      = true;
    o->nodes        = 0;
    o->flags        = flags;
    o->step         = 25;
    o->staging      = false;

    if(true)
    {
        for(uint32_t i = 0; i < CC; ++i) {
            o->layer[i] = malloc(sizeof(frame));
            frame_init(o->layer[i],  w, h);
        }

        frame_clr(o->layer[SC]);
        frame_clr(o->layer[SN]);
        frame_fill(o->layer[BG], BACKGROUND);
    }

    o->at = malloc(o->count * sizeof(sector));
    for(uint32_t i = 0; i < o->count; i++) o->at[i].id = i;

    o->at[0].type = CANVAS;

    for(uint32_t i = 0; i < CALLBACK_LIMIT; ++i) {
        o->at[0].callback[i] = &fuse_link;
    }

}

void destroy_field(field* o) {
    for(uint32_t i = 0; i < CC; ++i) {
        frame_flush(o->layer[i]);
        free(o->layer[i]);
    }
    free(o->at);
}

void empty(){
    printf("Empty callback\n");
}

void fuse_link(sector*, sector*) {
    printf("Fuse callback\n");
}

void link_sector(sector* parent, sector* child) {
    if (parent->nodes >= parent->capacity) {
        parent->capacity = parent->capacity * 2 + 4;
        parent->node = realloc(parent->node, parent->capacity * sizeof(sector*));
    }
    parent->node[parent->nodes] = child;
    child->root = parent;
    ++parent->nodes;
}

void move_sector(sector* s, ltrb32u* bounds) {
    draw_ltrb_f(s->carrier->layer[FG], &s->bounds, 0x0);
    draw_ltrb_f(s->carrier->layer[SC], &s->bounds, 0x0);
    
    draw_ltrb_f(s->carrier->layer[SC], bounds, s->id);

    s->bounds = *bounds;
}

void add_mod_link(sector* source, sector* target, CallbackType type, void (*fn)(sector*, sector*)) {

    source->callback[type] = fn;
    source->target[type] = target;

}
/*****************************************************************************************************************************/

void hit_test_down(field* o, int x, int y, MouseButton button) {
    o->current = frame_get(o->layer[SC], x, y);
    auto p = &o->at[o->current];

    o->pressed = p;
    o->lt[CP_PRESS].x = p->bounds.l;
    o->lt[CP_PRESS].y = p->bounds.t;

    o->mp[CP_PRESS].x = x;
    o->mp[CP_PRESS].y = y;

    o->mp[CP_PRIOR].x = x;
    o->mp[CP_PRIOR].y = y;

    if (p->flags & MOVEABLE) {
        if (button == LMB) o->drag = true;
        o->staging = true;

        draw_ltrb_f(
            p->carrier->layer[SN],
            &p->bounds, 
            0x0
        );

    }
    else {
        set_sector[p->type](o, x, y);
    }
    o->repaint = true;
    o->refresh = true;
    o->prior = o->current;
        
    p->callback[CALLBACK_PRESS](p, p->target[CALLBACK_PRESS]);
}

void hit_test(field* o, int x, int y) {
    auto uid = frame_get(o->layer[SC], x, y);

    if(o->current != uid) {
        o->prior = o->current;
        o->current = uid;
        
        o->at[o->prior].hovered = false;
        o->at[o->prior].repaint = true;
        
        o->at[o->current].hovered = true;
        o->at[o->current].repaint = true;
        
        o->repaint = true;
        o->refresh = true;
    }
}

void hit_test_drag(field* o, int x, int y) {
    drag_sector[o->at[o->current].type](o, x, y);
    o->repaint = true;
    o->refresh = true;
}

void hit_test_up(field* o, int x, int y, MouseButton button) {
    o->current = frame_get(o->layer[SC], x, y);

    if(o->pressed) {
        release_sector[o->pressed->type](o->pressed, x, y);
        o->pressed = nullptr;
    }

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

void draw_slider(field* o, sector* c) {
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    if(c->flags & VERTICAL) {
        int w   = c->bounds.b - c->bounds.t - GRIP * 2 - GAP * 2;
        int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + GRIP + GAP;
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, pos - GRIP, c->bounds.r - GAP, pos + GRIP, SELECTIONBACKGROUND);
    }
    else {
        int w   = c->bounds.r - c->bounds.l - GRIP * 2 - GAP * 2;
        int pos = (int)(c->value/c->range * (float)w) + c->bounds.l + GRIP + GAP;
        draw_rect_f(o->layer[FG], pos - GRIP, c->bounds.t + GAP, pos + GRIP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }

    c->repaint = false;
}

void draw_checkbox(field* o, sector* c) {
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);
    if(c->value > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_scene(field* o) {
    printf("Draw scene:\n");
    for(uint32_t i = 0; i < o->count; ++i) {
        if(o->at[i].repaint) {
            draw_sector[o->at[i].type](o, &o->at[i]);
            printf("-- Repaint: %d\n", i);
        }
    }
    
    o->repaint = false;
}

/*****************************************************************************************************************************/

void set_checkbox(field* o, int x, int y) {
    if (o->at[o->current].value < 0.5f) o->at[o->current].value = 1.0f;
    else o->at[o->current].value = 0.0f;
    o->at[o->current].repaint = true;

    if(false) printf("[%d : %d]", x, y);
}

void set_slider(field* o, int x, int y) {
    auto cs = &o->at[o->current];

    if(cs->flags & VERTICAL) {
        int ys = y - cs->bounds.t;
        cs->value = cs->range - (float)ys/(float)(cs->bounds.b - cs->bounds.t) * cs->range;
    }
    else {
        int xs = x - cs->bounds.l;
        cs->value = (float)xs/(float)(cs->bounds.r - cs->bounds.l) * cs->range;
    }

    if (cs->value < 0.0f) cs->value = 0.0f;
    if (cs->value > cs->range) cs->value = cs->range;
    cs->repaint = true;

    cs->callback[CALLBACK_VALUE](cs, cs->target[CALLBACK_VALUE]);
}

void set_step_slider(field* o, int x, int y) {
    if(o->at[o->current].flags & VERTICAL) {
        int ys = y - o->at[o->current].bounds.t;
        o->at[o->current].value = roundf(o->at[o->current].range - (float)ys/(float)(o->at[o->current].bounds.b - o->at[o->current].bounds.t)*o->at[o->current].range);
    }
    else {
        int xs = x - o->at[o->current].bounds.l;
        o->at[o->current].value = roundf((float)xs/(float)(o->at[o->current].bounds.r - o->at[o->current].bounds.l)*o->at[o->current].range);
    }

    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;

    o->at[o->current].repaint = true;
}

void draw_step_slider(field* o, sector* c) {
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    if(c->flags & VERTICAL) {
        int w   = c->bounds.b - c->bounds.t - GRIP * 2 - GAP * 2;
        int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + GRIP + GAP;
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, pos - GRIP, c->bounds.r - GAP, pos + GRIP, SELECTIONBACKGROUND);
    }
    else {
        int w   = c->bounds.r - c->bounds.l - GRIP * 2 - GAP  *2;
        int pos = (int)(c->value/c->range * (float)w) + c->bounds.l + GRIP + GAP;
        draw_rect_f(o->layer[FG], pos - GRIP, c->bounds.t + GAP, pos + GRIP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }

    c->repaint = false;
}

void scroll_slider(sector* o, int x, int y) {
    o->value += (float) y * o->step;
    if (o->value < 0.0f) o->value = 0.0f;
    if (o->value > o->range) o->value = o->range;
    o->repaint = true;
    
    if(false) printf("[%d : %d]", x, y);
    o->callback[CALLBACK_VALUE](o, o->target[CALLBACK_VALUE]);
}

void scroll_step_slider(sector* o, int x, int y) {
    o->value += (float) y;
    if (o->value < 0.0f) o->value = 0.0f;
    if (o->value > o->range) o->value = o->range;
    o->repaint = true;

    if(false) printf("[%d : %d]", x, y);
}

void draw_progress_bar(field* o, sector* c) {
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_rectangle(o->layer[FG], c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);

    if(o->at[o->current].flags & VERTICAL) {
        int w   = c->bounds.b - c->bounds.t - GAP;
        int pos = (int)((c->range - c->value)/c->range * (float)w) + c->bounds.t + GAP;
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, pos, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }
    else {
        int w   = c->bounds.r - c->bounds.l - GAP;
        int pos = (int)(c->value/c->range * (float)w) + c->bounds.l;
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, pos, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }

    c->repaint = false;
}

void draw_button(field* o, sector* c) {
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    if(c->hovered)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACTIVE);

    if(c->value > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACCENT);
    c->repaint = false;
    
    printf("Draw Button\n");
}

void draw_sprite_slider(field* o, sector* c) {
    sprite* temp = (sprite*)c->data;
    int f = (c->value / c->range) * temp->nframes;
    frame_copy_at(o->layer[FG], &temp->data[f], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

void set_sprite_slider(field* o, int x, int y) {
    int dy = roundf((o->mp[CP_PRIOR].y - y)/o->at[o->current].step);
    int dx = roundf((x - o->mp[CP_PRIOR].x)/o->at[o->current].step);

    o->at[o->current].value += (dy + dx);

    if (o->at[o->current].value < 0.0f) o->at[o->current].value = 0.0f;
    if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value = o->at[o->current].range;
    o->at[o->current].repaint = true;

    o->mp[CP_PRIOR].x = (float)x;
    o->mp[CP_PRIOR].y = (float)y;
}

void set_sprite_inf_slider(field* o, int x, int y) {
    int dy = roundf((y - o->mp[CP_PRIOR].y)/o->at[o->current].step);
    int dx = roundf((o->mp[CP_PRIOR].x - x)/o->at[o->current].step);

    o->at[o->current].value += (dy + dx);

    if (o->at[o->current].value < 0.0f) o->at[o->current].value += o->at[o->current].range;
    else if (o->at[o->current].value > o->at[o->current].range) o->at[o->current].value -= o->at[o->current].range;

    // if (o->at[o->current].value <= 0.0f) o->at[o->current].value = o->at[o->current].range;
    // else if (o->at[o->current].value >= o->at[o->current].range) o->at[o->current].value = 0.0f;

    o->at[o->current].repaint = true;
    o->mp[CP_PRIOR].x = (float)x;
    o->mp[CP_PRIOR].y = (float)y;
}

void set_button(field* o, int, int) {
    if  (o->at[o->current].value < 0.5f) o->at[o->current].value = 1.0f;
    else o->at[o->current].value = 0.0f;
    o->at[o->current].repaint = true;
    o->prior = o->current;
}

void release_button(sector* s, int, int) {
    auto p = s->carrier->pressed; 
    if(p) {
        if(p == s) {
            if  (p->value < 0.5f) p->value = 1.0f;
            else p->value = 0.0f;
        }
        else p->value = 0.0f;

        p->repaint = true;
    }
}

void draw_sprite_button(field* o, sector* c) {
    sprite* temp = (sprite*)c->data;

    if(c->value > 0.5f)
        frame_copy_at(o->layer[FG], &temp->data[1], c->bounds.l, c->bounds.t);
    else
        frame_copy_at(o->layer[FG], &temp->data[0], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

void init_socket(sector* s) {
    s->data = (float*)calloc(64, sizeof(float));
}

void set_socket(field* o, int, int) {
    auto cs = &o->at[o->current];
    if (cs->value < 0.5f) cs->value = 1.0f;
    else cs->value = 0.0f;
    cs->repaint = true;
    o->prior = o->current;
}

void draw_socket(field* o, sector* c) {
    draw_ltrb_f(o->layer[FG], &c->bounds, BACKGROUND);
    draw_rectangle(o->layer[FG], c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);
    if(c->value > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

void drag_socket(field* o, int x, int y)
{
    auto p = o->pressed;

    if(p) {

        float xe = (float)x;
        float ye = (float)y;

        if(xe > o->width) xe = (float)o->width;
        else if(xe < 0.0f) xe = 0.0f;

        if(ye > o->height) ye = (float)o->height;
        else if(ye < 0.0f) ye = 0.0f;

        point a, b, c, d;

        float xo = (float)p->bounds.l + 0.5f * (float)p->width;
        float yo = (float)p->bounds.t + 0.5f * (float)p->height;

        a.x = xo;
        a.y = yo;

        b.x = (xe - xo) * (1.0f/3.0f) + xo;
        c.x = (xe - xo) * (2.0f/3.0f) + xo;

        if(ye < yo) {
            b.y = fabs(yo - ye) * (1.0f - 1.0E-6f) + ye;
            c.y = fabs(yo - ye) * (1.0E-6f) + ye;
        }
        else {
            b.y = fabs(yo - ye) * (1.0E-6f) + yo;
            c.y = fabs(yo - ye) * (1.0f - 1.0E-6f) + yo;
        }

        d.x = xe;
        d.y = ye;

        int iterations = 32;
        const float inc = 1.0f / (float)(iterations - 1) ;
        float t = 0.0;

        for(int i = 0, j = 0; i < iterations; i++) {
            point s = interpolate_bezier(a, b, c, d, t);
            point uv = screen_to_uv(s.x, s.y, o->width, o->height);

            ((float*)o->at[o->current].data)[j++] = uv.x;
            ((float*)o->at[o->current].data)[j++] = uv.y;

            t += inc;
        }
    }
}
/*****************************************************************************************************************************/

void init_node(sector* s) {
    s->data = (frame*)calloc(1, sizeof(frame));
    frame_init((frame*)s->data, s->width, s->height);
}

void set_node(field* o, int, int) {
    o->at[o->current].repaint = true;
    o->prior = o->current;
}

void draw_node(field* o, sector* c) {

    auto L = o->staging ? ST : NG;

    if(L == ST) {
        frame_clr(o->layer[L]);
        draw_rectangle(o->layer[L], c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, HIGHLIGHT);
    }
    else
        draw_ltrb_f(o->layer[L], &c->bounds, SECONDBACKGROUND);


    c->repaint = false;
}

void drag_node(field* o, int x, int y)
{
    auto p = o->pressed;
    if(p) {

        bool moved = false;

        int dx = ((x - o->mp[CP_PRIOR].x) / o->step) * o->step;
        {
            int l = ((p->bounds.l + dx) / o->step) * o->step;
            if((l >= 0) && (l + p->width <= o->width)) {
                p->bounds.l = l;
                p->bounds.r = l + p->width;
                moved = true;
            }
        }

        int dy = ((y - o->mp[CP_PRIOR].y) / o->step) * o->step;
        {
            int t = ((p->bounds.t + dy) / o->step) * o->step;
            if((t >= 0) && (t + p->height <= o->height)) {
                p->bounds.t = t; 
                p->bounds.b = t + p->height;
                moved = true;
            }
        }

        if (moved) {
            
            o->mp[CP_PRIOR].x += dx;
            o->mp[CP_PRIOR].y += dy;
            
            p->repaint = true;
            o->repaint = true;
            o->refresh = true;
        }
    }
}

void release_node(sector* s, int x, int y) 
{
    ltrb32u ir = {
        .l = s->carrier->lt[CP_PRESS].x, 
        .t = s->carrier->lt[CP_PRESS].y, 
        .r = s->carrier->lt[CP_PRESS].x + s->width, 
        .b = s->carrier->lt[CP_PRESS].y + s->height
    };

    draw_ltrb_f(s->carrier->layer[SC], &ir, 0x0);
    draw_ltrb_f(s->carrier->layer[NG], &ir, 0x0);
    draw_ltrb_f(s->carrier->layer[FG], &ir, 0x0);

    draw_ltrb_f(s->carrier->layer[SC], &s->bounds, s->id);


    int dx = s->bounds.l - s->carrier->lt[CP_PRESS].x;
    int dy = s->bounds.t - s->carrier->lt[CP_PRESS].y;

    for(uint32_t i = 0; i < s->nodes; ++i) {

        ltrb32u r = {
            .l = s->node[i]->bounds.l + dx,
            .t = s->node[i]->bounds.t + dy,
            .r = s->node[i]->bounds.r + dx,
            .b = s->node[i]->bounds.b + dy
        };

        move_sector(s->node[i], &r);

        s->node[i]->repaint = true;
    }


    frame_clr(s->carrier->layer[ST]);
    s->carrier->staging = false;
    s->repaint = true;
    s->carrier->repaint = true;
    s->carrier->refresh = true;

    printf("-- Node Released ");
}

/*****************************************************************************************************************************/

void init_textbox(sector* s) {
    s->data = (char*)calloc(64, sizeof(char)); 
}

void set_textbox(field* o, int, int) {
    o->at[o->current].repaint = true;
    char *text = o->at[o->current].data;
    strcpy(text, "text");
}

void draw_textbox(field* o, sector* c) {

    draw_ltrb_f(o->layer[FG], &c->bounds, SECONDBACKGROUND);

    char *text = c->data;


    draw_text_label(
        o->layer[FG],
        gtFont,
        text,
        c->bounds.l,
        c->bounds.t,
        0,
        0,
        TEXT
    );

    c->repaint = false;
}

/*****************************************************************************************************************************/

void draw_canvas(field* o, sector*) 
{
    uint32_t major = 4;
    if(true) {
            
        for(uint32_t x = o->step, l = 1; x < o->layer[BG]->width; x += o->step) {
            for(uint32_t y = 0; y < o->layer[BG]->height; ++y) {
                if(!frame_get(o->layer[SC], x, y))
                    frame_pset(o->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
   
        for(uint32_t y = o->step, l = 1; y < o->layer[BG]->height; y += o->step) {
            for(uint32_t x = 0; x < o->layer[BG]->width; ++x) {
                if(!frame_get(o->layer[SC], x, y))
                    frame_pset(o->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
    }
}


/*****************************************************************************************************************************/

void init_none(sector*) {}

void (*init_sector[])(sector*) = {
    [SLIDER]                = init_none,  
    [STEP_SLIDER]           = init_none,  
    [PROGRESS_BAR]          = init_none,  
    [SPRITE_SLIDER]         = init_none,  
    [SPRITE_INF_SLIDER]     = init_none,  
    [SOCKET]                = init_socket,
    [CHECKBOX]              = init_none,  
    [BUTTON]                = init_none,  
    [SPRITE_CHECKBOX]       = init_none,  
    [SPRITE_BUTTON]         = init_none,  
    [CANVAS]                = init_none,  
    [TEXTBOX]               = init_textbox,  
    [NODE]                  = init_node,  
};

static inline void set_none(field*, int, int) {}

void (*set_sector[])(field*, int, int) = {
    [SLIDER]                = set_slider,            
    [STEP_SLIDER]           = set_step_slider,       
    [PROGRESS_BAR]          = set_slider,            
    [SPRITE_SLIDER]         = set_sprite_slider,     
    [SPRITE_INF_SLIDER]     = set_sprite_inf_slider, 
    [SOCKET]                = set_socket,            
    [CHECKBOX]              = set_checkbox,          
    [BUTTON]                = set_button,            
    [SPRITE_CHECKBOX]       = set_checkbox,          
    [SPRITE_BUTTON]         = set_button,            
    [CANVAS]                = set_none,              
    [TEXTBOX]               = set_textbox,
    [NODE]                  = set_node               
};

void (*draw_sector[])(field*, sector*) = {
    [SLIDER]                = draw_slider,        
    [STEP_SLIDER]           = draw_step_slider,   
    [PROGRESS_BAR]          = draw_progress_bar,  
    [SPRITE_SLIDER]         = draw_sprite_slider, 
    [SPRITE_INF_SLIDER]     = draw_sprite_slider, 
    [SOCKET]                = draw_socket,        
    [CHECKBOX]              = draw_checkbox,      
    [BUTTON]                = draw_button,        
    [SPRITE_CHECKBOX]       = draw_sprite_button, 
    [SPRITE_BUTTON]         = draw_sprite_button, 
    [CANVAS]                = draw_canvas,        
    [TEXTBOX]               = draw_textbox,
    [NODE]                  = draw_node           
};

void (*drag_sector[])(field*, int, int) = {
    [SLIDER]                = set_slider,           
    [STEP_SLIDER]           = set_step_slider,      
    [PROGRESS_BAR]          = set_slider,           
    [SPRITE_SLIDER]         = set_sprite_slider,    
    [SPRITE_INF_SLIDER]     = set_sprite_inf_slider,
    [SOCKET]                = drag_socket,          
    [CHECKBOX]              = set_none,             
    [BUTTON]                = set_none,             
    [SPRITE_CHECKBOX]       = set_none,             
    [SPRITE_BUTTON]         = set_none,             
    [CANVAS]                = set_none,             
    [TEXTBOX]               = set_none,  
    [NODE]                  = drag_node             
};

static inline void scroll_none(sector*, int, int) {}

void (*scroll_sector[])(sector*, int, int) = {
    [SLIDER]                = scroll_slider,     
    [STEP_SLIDER]           = scroll_step_slider,
    [PROGRESS_BAR]          = scroll_slider,     
    [SPRITE_SLIDER]         = scroll_slider,     
    [SPRITE_INF_SLIDER]     = scroll_slider,     
    [SOCKET]                = scroll_none,          
    [CHECKBOX]              = scroll_none,          
    [BUTTON]                = scroll_none,          
    [SPRITE_CHECKBOX]       = scroll_none,          
    [SPRITE_BUTTON]         = scroll_none,          
    [CANVAS]                = scroll_none,          
    [TEXTBOX]               = scroll_none,  
    [NODE]                  = scroll_none 
};

void (*leave_sector[])(field*, int, int) = {
    [SLIDER]                = set_none,         
    [STEP_SLIDER]           = set_none,         
    [PROGRESS_BAR]          = set_none,         
    [SPRITE_SLIDER]         = set_none,         
    [SPRITE_INF_SLIDER]     = set_none,         
    [SOCKET]                = set_none,         
    [CHECKBOX]              = set_none,         
    [BUTTON]                = set_none,   
    [SPRITE_CHECKBOX]       = set_none,         
    [SPRITE_BUTTON]         = set_none,   
    [CANVAS]                = set_none,         
    [TEXTBOX]               = set_none,  
    [NODE]                  = set_none          
};

static inline void release_none(sector*, int, int) {}

void (*release_sector[])(sector*, int, int) = {
    [SLIDER]                = release_none,
    [STEP_SLIDER]           = release_none,
    [PROGRESS_BAR]          = release_none,
    [SPRITE_SLIDER]         = release_none,
    [SPRITE_INF_SLIDER]     = release_none,
    [SOCKET]                = release_none,
    [CHECKBOX]              = release_none,
    [BUTTON]                = release_button,
    [SPRITE_CHECKBOX]       = release_none,
    [SPRITE_BUTTON]         = release_button,
    [CANVAS]                = release_none,
    [TEXTBOX]               = release_none,  
    [NODE]                  = release_node 
}; 
    
    
    
    
    
    
    
    
    
    
    
