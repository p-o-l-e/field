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

    o->at[pos].type                     = type;
    o->at[pos].bounds.l                 = x;
    o->at[pos].bounds.t                 = y;
    o->at[pos].bounds.r                 = x + w;
    o->at[pos].bounds.b                 = y + h;
    o->at[pos].width                    = w;
    o->at[pos].height                   = h;
    o->at[pos].value[CP_COARSE]         = 0.0f;
    o->at[pos].value[CP_FINE]           = 0.0f;
    o->at[pos].range[0]                 = -8.0f;
    o->at[pos].range[1]                 = +8.0f;
    o->at[pos].step[CP_COARSE]          = 1.0f;
    o->at[pos].step[CP_FINE]            = 0.1f;
    o->at[pos].repaint                  = true;
    o->at[pos].flags                    = flags;
    o->at[pos].carrier                  = o;

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
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

    canvas->type = ST_CANVAS;
    canvas->carrier = o;
    canvas->repaint = true;

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
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
    o->memory[SP_LT_PRESS].x = p->bounds.l;
    o->memory[SP_LT_PRESS].y = p->bounds.t;

    o->memory[SP_CURSOR_PRESS].x = x;
    o->memory[SP_CURSOR_PRESS].y = y;

    o->memory[SP_CURSOR_PRIOR].x = x;
    o->memory[SP_CURSOR_PRIOR].y = y;

    p->memory[CP_COARSE] = p->value[CP_COARSE];
    p->memory[CP_FINE] = p->value[CP_FINE];

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
        
    p->callback[CT_PRESS](p, p->target[CT_PRESS]);
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
    //printf("Hit Test Drag : %d\n", o->pressed->type);
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

static void draw_checkbox(sector* c) {
    auto o = c->carrier;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);
    if(c->value[CP_COARSE] > 0.5f)
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
    auto coarse = &o->value[CP_COARSE];
    if (*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    o->repaint = true;

    if(false) printf("[%d : %d]", x, y);
}


static void set_slider(sector* o, int x, int y) {
    const float v = 0.1f;
    auto coarse = &o->value[CP_COARSE];
    auto fine = &o->value[CP_FINE];
    auto range = o->range[1] - o->range[0];
    printf("coarse initial : %f\n", *coarse);

    float value = {};

    if(o->flags & VERTICAL) {
        int dy = o->carrier->memory[SP_CURSOR_PRIOR].y - y;
        value = roundf((float)dy * v) + o->memory[CP_COARSE];
        printf("coarse : %f - dy : %d\n", *coarse, dy);
    }
    else {
        int dx = o->carrier->memory[SP_CURSOR_PRIOR].x - x;
        value = -roundf((float)dx * v) - o->memory[CP_COARSE];
        printf("coarse : %f - value : %f\n", *coarse, value);
    }

    if(value < o->range[0]) {
        *coarse = o->range[0];
        *fine = 0.0f;
    }
    else if(value > o->range[1]) {
        *coarse = o->range[1];
        *fine = 0.0f;
    }
    else *coarse = value;

    o->repaint = true;
    o->callback[CT_VALUE](o, o->target[CT_VALUE]);
}

static void draw_slider(sector* c) {
    auto o = c->carrier;
    auto range = c->range[1] - c->range[0];
    auto coarse = &c->value[CP_COARSE];
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    if(c->flags & VERTICAL) {
        int w   = c->bounds.b - c->bounds.t - GRIP * 2 - GAP * 2;
        int pos = (int)((c->range[1] - *coarse)/range * (float)w) + c->bounds.t + GRIP + GAP;
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, pos - GRIP, c->bounds.r - GAP, pos + GRIP, SELECTIONBACKGROUND);
    }
    else {
        int h   = c->bounds.r - c->bounds.l - GRIP * 2 - GAP * 2;
        int pos = (int)((c->range[0] + *coarse)/range * (float)h) + c->bounds.r - GRIP - GAP;
        draw_rect_f(o->layer[FG], pos - GRIP, c->bounds.t + GAP, pos + GRIP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }

    c->repaint = false;
}

static void scroll_slider(sector* o, int x, int y) {
    auto fine = &o->value[CP_FINE];
    auto coarse = &o->value[CP_COARSE];
    auto df = (float)y * o->step[CP_FINE];

    if((*coarse + df < o->range[1]) && (*coarse + df > o->range[0])) {
        *fine += df;
    }

    if((int)*fine != 0) {
        *coarse += (int)*fine;
        *fine = 0.0f;
    };

    o->repaint = true;
    
    printf("fine : %f - coarse : %f\n", *fine, *coarse);
    o->callback[CT_VALUE](o, o->target[CT_VALUE]);
}

static void draw_progress_bar(sector* c) {
    auto o = c->carrier;
    auto range = c->range[1] - c->range[0];
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_rectangle(o->layer[FG], c->bounds.l, c->bounds.t, c->bounds.r, c->bounds.b, SECONDBACKGROUND);

    if(o->at[o->current].flags & VERTICAL) {
        int w   = c->bounds.b - c->bounds.t - GAP;
        int pos = (int)((range - c->value[CP_COARSE])/range * (float)w) + c->bounds.t + GAP;
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, pos, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    }
    else {
        int w   = c->bounds.r - c->bounds.l - GAP;
        int pos = (int)((range - c->value[CP_COARSE])/range * (float)w) + c->bounds.l;
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

    if(c->value[CP_COARSE] > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACCENT);
    c->repaint = false;
    
    printf("Draw Button\n");
}

static void draw_sprite_slider(sector* c) {
    auto o = c->carrier;
    auto range = c->range[1] - c->range[0];
    sprite* temp = (sprite*)c->data;
    int f = (c->value[CP_COARSE] / range) * temp->nframes;
    frame_copy_at(o->layer[FG], &temp->data[f], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void set_sprite_slider(sector* o, int x, int y) {
    auto f = o->carrier;
    auto coarse = &o->value[CP_COARSE];
    int dy = roundf((f->memory[SP_CURSOR_PRIOR].y - y)/o->step[CP_COARSE]);
    int dx = roundf((x - f->memory[SP_CURSOR_PRIOR].x)/o->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < o->range[0]) *coarse = o->range[0];
    else if(*coarse > o->range[1]) *coarse = o->range[1];
    o->repaint = true;

    f->memory[SP_CURSOR_PRIOR].x = (float)x;
    f->memory[SP_CURSOR_PRIOR].y = (float)y;
}

static void set_sprite_inf_slider(sector* o, int x, int y) {
    auto f = o->carrier;
    auto coarse = &o->value[CP_COARSE];
    int dy = roundf((y - f->memory[SP_CURSOR_PRIOR].y)/o->step[CP_COARSE]);
    int dx = roundf((f->memory[SP_CURSOR_PRIOR].x - x)/o->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < o->range[0]) *coarse += o->range[0];
    else if(*coarse > o->range[1]) *coarse -= o->range[1];

    // if (o->at[o->current].value <= 0.0f) o->at[o->current].value = o->at[o->current].range;
    // else if (o->at[o->current].value >= o->at[o->current].range) o->at[o->current].value = 0.0f;

    o->repaint = true;
    f->memory[SP_CURSOR_PRIOR].x = (float)x;
    f->memory[SP_CURSOR_PRIOR].y = (float)y;
}

static void set_button(sector* o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    o->repaint = true;

    auto f = o->carrier;
    f->prior = f->current;
}

static void release_button(sector* o, int, int) {
    auto p = o->carrier->pressed; 
    auto coarse = &p->value[CP_COARSE];
    if(p) {
        if(p == o) {
            if(*coarse < 0.5f) *coarse = 1.0f;
            else *coarse = 0.0f;
        }
        else *coarse = 0.0f;

        p->repaint = true;
    }
}

static void draw_sprite_button(sector* c) {
    auto o = c->carrier;
    sprite* spr = (sprite*)c->data;

    if(c->value[CP_COARSE] > 0.5f)
        frame_copy_at(o->layer[FG], &spr->data[1], c->bounds.l, c->bounds.t);
    else
        frame_copy_at(o->layer[FG], &spr->data[0], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void init_socket(sector* s) {
    s->data = (float*)calloc(64, sizeof(float));
}

static void set_socket(sector* o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    o->repaint = true;

    auto f = o->carrier;
    f->prior = f->current;
}

static void draw_socket(sector* c) {
    auto o = c->carrier;
    auto r = c->width / 2;
    point32u center = {
        .x = c->bounds.l + r,
        .y = c->bounds.t + r
    };

    draw_circle_f(o->layer[FG], center.x, center.y, r, BORDER);

    r = c->width / 4;

    draw_circle_f(o->layer[FG], center.x, center.y, r, BUTTONS);
    if(c->value[CP_COARSE] > 0.5f)
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

    int dx = ((x - f->memory[SP_CURSOR_PRIOR].x) / f->step) * f->step;
    {
        int l = ((o->bounds.l + dx) / f->step) * f->step;
        if((l >= 0) && (l + o->width <= f->width)) {
            o->bounds.l = l;
            o->bounds.r = l + o->width;
            moved = true;
        }
    }

    int dy = ((y - f->memory[SP_CURSOR_PRIOR].y) / f->step) * f->step;
    {
        int t = ((o->bounds.t + dy) / f->step) * f->step;
        if((t >= 0) && (t + o->height <= f->height)) {
            o->bounds.t = t; 
            o->bounds.b = t + o->height;
            moved = true;
        }
    }

    if (moved) {
        
        f->memory[SP_CURSOR_PRIOR].x += dx;
        f->memory[SP_CURSOR_PRIOR].y += dy;
        
        o->repaint = true;
        f->repaint = true;
        f->refresh = true;
    }
    
    printf("Drag node\n");
}

static void release_node(sector* s, int, int) 
{
    ltrb32u ir = {
        .l = s->carrier->memory[SP_LT_PRESS].x, 
        .t = s->carrier->memory[SP_LT_PRESS].y, 
        .r = s->carrier->memory[SP_LT_PRESS].x + s->width, 
        .b = s->carrier->memory[SP_LT_PRESS].y + s->height
    };

    draw_ltrb_f(s->carrier->layer[SC], &ir, 0x0);
    draw_ltrb_f(s->carrier->layer[NG], &ir, 0x0);
    draw_ltrb_f(s->carrier->layer[FG], &ir, 0x0);

    draw_ltrb_f(s->carrier->layer[SC], &s->bounds, s->id);

    int dx = s->bounds.l - s->carrier->memory[SP_LT_PRESS].x;
    int dy = s->bounds.t - s->carrier->memory[SP_LT_PRESS].y;

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
    //printf("-- Draw TEXTBOX\n");
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
                    frame_set(o->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
   
        for(uint32_t y = o->step, l = 1; y < o->layer[BG]->height; y += o->step) {
            for(uint32_t x = 0; x < o->layer[BG]->width; ++x) {
                if(!frame_get(o->layer[SC], x, y))
                    frame_set(o->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
    }
}


/*****************************************************************************************************************************/

static inline void init_none(sector*) {}

void (*init_sector[])(sector*) = {
    [ST_SLIDER]                = init_none,  
    [ST_PROGRESS_BAR]          = init_none,  
    [ST_SPRITE_SLIDER]         = init_none,  
    [ST_SPRITE_INF_SLIDER]     = init_none,  
    [ST_SOCKET]                = init_socket,
    [ST_CHECKBOX]              = init_none,  
    [ST_BUTTON]                = init_none,  
    [ST_SPRITE_CHECKBOX]       = init_none,  
    [ST_SPRITE_BUTTON]         = init_none,  
    [ST_CANVAS]                = init_none,  
    [ST_TEXTBOX]               = init_textbox,  
    [ST_NODE]                  = init_node,  
};

static inline void set_none(sector*, int, int) {}

void (*set_sector[])(sector*, int, int) = {
    [ST_SLIDER]                = set_slider,            
    [ST_PROGRESS_BAR]          = set_slider,            
    [ST_SPRITE_SLIDER]         = set_sprite_slider,     
    [ST_SPRITE_INF_SLIDER]     = set_sprite_inf_slider, 
    [ST_SOCKET]                = set_socket,            
    [ST_CHECKBOX]              = set_checkbox,          
    [ST_BUTTON]                = set_button,            
    [ST_SPRITE_CHECKBOX]       = set_checkbox,          
    [ST_SPRITE_BUTTON]         = set_button,            
    [ST_CANVAS]                = set_none,              
    [ST_TEXTBOX]               = set_textbox,
    [ST_NODE]                  = set_node               
};

void (*draw_sector[])(sector*) = {
    [ST_SLIDER]                = draw_slider,        
    [ST_PROGRESS_BAR]          = draw_progress_bar,  
    [ST_SPRITE_SLIDER]         = draw_sprite_slider, 
    [ST_SPRITE_INF_SLIDER]     = draw_sprite_slider, 
    [ST_SOCKET]                = draw_socket,        
    [ST_CHECKBOX]              = draw_checkbox,      
    [ST_BUTTON]                = draw_button,        
    [ST_SPRITE_CHECKBOX]       = draw_sprite_button, 
    [ST_SPRITE_BUTTON]         = draw_sprite_button, 
    [ST_CANVAS]                = draw_canvas,        
    [ST_TEXTBOX]               = draw_textbox,
    [ST_NODE]                  = draw_node           
};

static inline void drag_none(sector*, int, int) {}

void (*drag_sector[])(sector*, int, int) = {
    [ST_SLIDER]                = set_slider,           
    [ST_PROGRESS_BAR]          = set_slider,           
    [ST_SPRITE_SLIDER]         = set_sprite_slider,    
    [ST_SPRITE_INF_SLIDER]     = set_sprite_inf_slider,
    [ST_SOCKET]                = drag_socket,          
    [ST_CHECKBOX]              = drag_none,             
    [ST_BUTTON]                = drag_none,             
    [ST_SPRITE_CHECKBOX]       = drag_none,             
    [ST_SPRITE_BUTTON]         = drag_none,             
    [ST_CANVAS]                = drag_none,             
    [ST_TEXTBOX]               = drag_none,  
    [ST_NODE]                  = drag_node             
};

static inline void scroll_none(sector*, int, int) {}

void (*scroll_sector[])(sector*, int, int) = {
    [ST_SLIDER]                = scroll_slider,     
    [ST_PROGRESS_BAR]          = scroll_slider,     
    [ST_SPRITE_SLIDER]         = scroll_slider,     
    [ST_SPRITE_INF_SLIDER]     = scroll_slider,     
    [ST_SOCKET]                = scroll_none,          
    [ST_CHECKBOX]              = scroll_none,          
    [ST_BUTTON]                = scroll_none,          
    [ST_SPRITE_CHECKBOX]       = scroll_none,          
    [ST_SPRITE_BUTTON]         = scroll_none,          
    [ST_CANVAS]                = scroll_none,          
    [ST_TEXTBOX]               = scroll_none,  
    [ST_NODE]                  = scroll_none 
};

static inline void leave_none(sector*, int, int) {}

void (*leave_sector[])(sector*, int, int) = {
    [ST_SLIDER]                = leave_none,         
    [ST_PROGRESS_BAR]          = leave_none,         
    [ST_SPRITE_SLIDER]         = leave_none,         
    [ST_SPRITE_INF_SLIDER]     = leave_none,         
    [ST_SOCKET]                = leave_none,         
    [ST_CHECKBOX]              = leave_none,         
    [ST_BUTTON]                = leave_none,   
    [ST_SPRITE_CHECKBOX]       = leave_none,         
    [ST_SPRITE_BUTTON]         = leave_none,   
    [ST_CANVAS]                = leave_none,         
    [ST_TEXTBOX]               = leave_none,  
    [ST_NODE]                  = leave_none          
};

static inline void release_none(sector*, int, int) {}

void (*release_sector[])(sector*, int, int) = {
    [ST_SLIDER]                = release_none,
    [ST_PROGRESS_BAR]          = release_none,
    [ST_SPRITE_SLIDER]         = release_none,
    [ST_SPRITE_INF_SLIDER]     = release_none,
    [ST_SOCKET]                = release_none,
    [ST_CHECKBOX]              = release_none,
    [ST_BUTTON]                = release_button,
    [ST_SPRITE_CHECKBOX]       = release_none,
    [ST_SPRITE_BUTTON]         = release_button,
    [ST_CANVAS]                = release_none,
    [ST_TEXTBOX]               = release_none,  
    [ST_NODE]                  = release_node 
}; 
    
    
    
    
    
    
    
    
    
    
    
