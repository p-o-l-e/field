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

sector* createSector(field* o, sector* node, SectorType type, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t flags) {
    auto pos = ++o->sectors;

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

void initField(field* o, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t size, uint32_t flags) {
    o->bounds.l     = x;
    o->bounds.t     = y;
    o->bounds.r     = x + w;
    o->bounds.b     = y + h;

    o->width        = w;
    o->height       = h;

    o->drag         = false;
    o->move         = false;
    o->capacity     = size + 1u;
    o->repaint      = true;
    o->refresh      = true;
    o->sectors      = 0;
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

    o->at = malloc(o->capacity * sizeof(sector));
    for(uint32_t i = 0; i < o->capacity; i++) o->at[i].id = i;

    auto canvas = &o->at[0];

    canvas->type = CANVAS;
    canvas->carrier = o;
    canvas->repaint = true;

    for(uint32_t i = 0; i < CALLBACK_LIMIT; ++i) {
        canvas->callback[i] = &fuse_link;
    }
}

void destroyField(field* o) {
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

    if(p->flags & INTERCON) {
        o->connecting = true;
    }

    if (p->flags & MOVEABLE) {
        if (button == LMB) o->drag = true;
        o->staging = true;
        p->staging = true;

        draw_ltrb_f(
            p->carrier->layer[SN],
            &p->bounds, 
            0x0
        );

    }
    else {
        set_sector[p->type](o->pressed, x, y);
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
    drag_sector[o->pressed->type](o->pressed, x, y);
    o->repaint = true;
    o->refresh = true;
    printf("Hit Test Drag : %d\n", o->pressed->type);
}

void hit_test_up(field* o, int x, int y, MouseButton button) {
    o->current = frame_get(o->layer[SC], x, y);

    if(o->pressed) {
        release_sector[o->pressed->type](o->pressed, x, y);
        o->pressed = nullptr;
    }

    o->drag    = false;
    o->move    = false;
    o->connecting = false;
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

static void draw_slider(sector* c) {
    auto o = c->carrier;
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

static void draw_checkbox(sector* c) {
    auto o = c->carrier;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);
    if(c->value > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_scene(field* o) {
    // printf("Draw scene:\n");
    for(uint32_t i = 0; i <= o->sectors; ++i) {
        auto s = &o->at[i];
        if(s->repaint) {
            draw_sector[s->type](s);
            // printf("-- Repaint: %d\n", i);
        }
    }
    
    o->repaint = false;
}

/*****************************************************************************************************************************/

static void set_checkbox(sector* o, int x, int y) {
    if (o->value < 0.5f) o->value = 1.0f;
    else o->value = 0.0f;
    o->repaint = true;

    if(false) printf("[%d : %d]", x, y);
}

static void set_slider(sector* o, int x, int y) {
    if(o->flags & VERTICAL) {
        int ys = y - o->bounds.t;
        o->value = o->range - (float)ys/(float)(o->bounds.b - o->bounds.t) * o->range;
    }
    else {
        int xs = x - o->bounds.l;
        o->value = (float)xs/(float)(o->bounds.r - o->bounds.l) * o->range;
    }

    if (o->value < 0.0f) o->value = 0.0f;
    if (o->value > o->range) o->value = o->range;
    o->repaint = true;

    o->callback[CALLBACK_VALUE](o, o->target[CALLBACK_VALUE]);
}

static void set_step_slider(sector* o, int x, int y) {
    if(o->flags & VERTICAL) {
        int ys = y - o->bounds.t;
        o->value = roundf(o->range - (float)ys/(float)(o->bounds.b - o->bounds.t) * o->range);
    }
    else {
        int xs = x - o->bounds.l;
        o->value = roundf((float)xs/(float)(o->bounds.r - o->bounds.l) * o->range);
    }

    if (o->value < 0.0f) o->value = 0.0f;
    if (o->value > o->range) o->value = o->range;

    o->repaint = true;
}

static void draw_step_slider(sector* c) {
    auto o = c->carrier;
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

static void scroll_slider(sector* o, int x, int y) {
    o->value += (float) y * o->step;
    if (o->value < 0.0f) o->value = 0.0f;
    if (o->value > o->range) o->value = o->range;
    o->repaint = true;
    
    if(false) printf("[%d : %d]", x, y);
    o->callback[CALLBACK_VALUE](o, o->target[CALLBACK_VALUE]);
}

static void scroll_step_slider(sector* o, int x, int y) {
    o->value += (float) y;
    if (o->value < 0.0f) o->value = 0.0f;
    if (o->value > o->range) o->value = o->range;
    o->repaint = true;

    if(false) printf("[%d : %d]", x, y);
}

static void draw_progress_bar(sector* c) {
    auto o = c->carrier;
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

static void draw_button(sector* c) {
    auto o = c->carrier;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    if(c->hovered)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACTIVE);

    if(c->value > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACCENT);
    c->repaint = false;
    
    printf("Draw Button\n");
}

static void draw_sprite_slider(sector* c) {
    auto o = c->carrier;
    sprite* temp = (sprite*)c->data;
    int f = (c->value / c->range) * temp->nframes;
    frame_copy_at(o->layer[FG], &temp->data[f], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void set_sprite_slider(sector* o, int x, int y) {
    auto f = o->carrier;

    int dy = roundf((f->mp[CP_PRIOR].y - y)/o->step);
    int dx = roundf((x - f->mp[CP_PRIOR].x)/o->step);

    o->value += (dy + dx);

    if (o->value < 0.0f) o->value = 0.0f;
    if (o->value > o->range) o->value = o->range;
    o->repaint = true;

    f->mp[CP_PRIOR].x = (float)x;
    f->mp[CP_PRIOR].y = (float)y;
}

static void set_sprite_inf_slider(sector* o, int x, int y) {
    auto f = o->carrier;
    int dy = roundf((y - f->mp[CP_PRIOR].y)/o->step);
    int dx = roundf((f->mp[CP_PRIOR].x - x)/o->step);

    o->value += (dy + dx);

    if (o->value < 0.0f) o->value += o->range;
    else if (o->value > o->range) o->value -= o->range;

    // if (o->at[o->current].value <= 0.0f) o->at[o->current].value = o->at[o->current].range;
    // else if (o->at[o->current].value >= o->at[o->current].range) o->at[o->current].value = 0.0f;

    o->repaint = true;
    f->mp[CP_PRIOR].x = (float)x;
    f->mp[CP_PRIOR].y = (float)y;
}

static void set_button(sector* o, int, int) {
    if  (o->value < 0.5f) o->value = 1.0f;
    else o->value = 0.0f;
    o->repaint = true;

    auto f = o->carrier;
    f->prior = f->current;
}

static void release_button(sector* s, int, int) {
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

static void draw_sprite_button(sector* c) {
    auto o = c->carrier;
    sprite* temp = (sprite*)c->data;

    if(c->value > 0.5f)
        frame_copy_at(o->layer[FG], &temp->data[1], c->bounds.l, c->bounds.t);
    else
        frame_copy_at(o->layer[FG], &temp->data[0], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void init_socket(sector* s) {
    s->data = (float*)calloc(64, sizeof(float));
}

static void set_socket(sector* o, int, int) {
    if (o->value < 0.5f) o->value = 1.0f;
    else o->value = 0.0f;
    o->repaint = true;

    auto f = o->carrier;
    f->prior = f->current;
}

static void draw_socket(sector* c) {
    auto o = c->carrier;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTON);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);
    if(c->value > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

static void drag_socket(sector* s, int x, int y)
{
    printf("Drag socket : %d\n", s->type);

    auto o = s->carrier;

    float xe = (float)x;
    float ye = (float)y;

    if(xe > o->width) xe = (float)o->width;
    else if(xe < 0.0f) xe = 0.0f;

    if(ye > o->height) ye = (float)o->height;
    else if(ye < 0.0f) ye = 0.0f;

    point a, b, c, d;

    float xo = (float)s->bounds.l + 0.5f * (float)s->width;
    float yo = (float)s->bounds.t + 0.5f * (float)s->height;

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
        point p = interpolate_bezier(a, b, c, d, t);
        point uv = screen_to_uv(p.x, p.y, o->width, o->height);

        ((float*)s->data)[j++] = uv.x;
        ((float*)s->data)[j++] = uv.y;

        t += inc;
    }
}
/*****************************************************************************************************************************/

static void init_node(sector* s) {
    s->data = (frame*)calloc(1, sizeof(frame));
    frame_init((frame*)s->data, s->width, s->height);
}

static void set_node(sector* o, int, int) {
    o->repaint = true;
    auto f = o->carrier;
    f->prior = f->current;
}

static void draw_node(sector* c) {
    auto o = c->carrier;
    auto l = c->staging ? ST : NG;

    if(l == ST) {
        frame_clr(o->layer[l]);
        draw_ltrb_o(o->layer[l], &c->bounds, HIGHLIGHT);
    }
    else
        draw_ltrb_f(o->layer[l], &c->bounds, SECONDBACKGROUND);

    c->repaint = false;
}

static void drag_node(sector* o, int x, int y)
{
    auto f = o->carrier;
    bool moved = false;

    int dx = ((x - f->mp[CP_PRIOR].x) / f->step) * f->step;
    {
        int l = ((o->bounds.l + dx) / f->step) * f->step;
        if((l >= 0) && (l + o->width <= f->width)) {
            o->bounds.l = l;
            o->bounds.r = l + o->width;
            moved = true;
        }
    }

    int dy = ((y - f->mp[CP_PRIOR].y) / f->step) * f->step;
    {
        int t = ((o->bounds.t + dy) / f->step) * f->step;
        if((t >= 0) && (t + o->height <= f->height)) {
            o->bounds.t = t; 
            o->bounds.b = t + o->height;
            moved = true;
        }
    }

    if (moved) {
        
        f->mp[CP_PRIOR].x += dx;
        f->mp[CP_PRIOR].y += dy;
        
        o->repaint = true;
        f->repaint = true;
        f->refresh = true;
    }
    
    printf("Drag node\n");
}

static void release_node(sector* s, int, int) 
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
    s->staging = false;
    s->repaint = true;
    s->carrier->repaint = true;
    s->carrier->refresh = true;

    printf("-- Node Released ");
}

/*****************************************************************************************************************************/

static void init_textbox(sector* s) {
    s->data = (char*)calloc(64, sizeof(char)); 
}

static void set_textbox(sector* o, int, int) {
    o->repaint = true;
}

static void draw_textbox(sector* c) {
    auto o = c->carrier;
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
    printf("-- Draw TEXTBOX");
}

/*****************************************************************************************************************************/

static void draw_canvas(sector* s) 
{
    auto o = s->carrier;
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

static inline void init_none(sector*) {}

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

static inline void set_none(sector*, int, int) {}

void (*set_sector[])(sector*, int, int) = {
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

void (*draw_sector[])(sector*) = {
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

static inline void drag_none(sector*, int, int) {}

void (*drag_sector[])(sector*, int, int) = {
    [SLIDER]                = set_slider,           
    [STEP_SLIDER]           = set_step_slider,      
    [PROGRESS_BAR]          = set_slider,           
    [SPRITE_SLIDER]         = set_sprite_slider,    
    [SPRITE_INF_SLIDER]     = set_sprite_inf_slider,
    [SOCKET]                = drag_socket,          
    [CHECKBOX]              = drag_none,             
    [BUTTON]                = drag_none,             
    [SPRITE_CHECKBOX]       = drag_none,             
    [SPRITE_BUTTON]         = drag_none,             
    [CANVAS]                = drag_none,             
    [TEXTBOX]               = drag_none,  
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

static inline void leave_none(sector*, int, int) {}

void (*leave_sector[])(sector*, int, int) = {
    [SLIDER]                = leave_none,         
    [STEP_SLIDER]           = leave_none,         
    [PROGRESS_BAR]          = leave_none,         
    [SPRITE_SLIDER]         = leave_none,         
    [SPRITE_INF_SLIDER]     = leave_none,         
    [SOCKET]                = leave_none,         
    [CHECKBOX]              = leave_none,         
    [BUTTON]                = leave_none,   
    [SPRITE_CHECKBOX]       = leave_none,         
    [SPRITE_BUTTON]         = leave_none,   
    [CANVAS]                = leave_none,         
    [TEXTBOX]               = leave_none,  
    [NODE]                  = leave_none          
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
    
    
    
    
    
    
    
    
    
    
    
