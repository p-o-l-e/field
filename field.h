#pragma  once
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdatomic.h>

#define DEBUG_OVERLAY
#ifdef DEBUG_OVERLAY
atomic_bool force_repaint = false;
#endif
constexpr uint_fast32_t SPLINE_SEGMENTS = 32;
constexpr uint_fast32_t TEXTBOX_SIZE = 32;

typedef enum {
    ST_SLIDER, 
    ST_ROTARY,
    ST_SPRITE_INF_SLIDER,
    ST_SOCKET,
    ST_CHECKBOX, 
    ST_MOMENTARY, 
    ST_SPRITE_CHECKBOX, 
    ST_SPRITE_BUTTON, 
    ST_CANVAS,
    ST_TEXTBOX,
    ST_NODE,

    ST_LIMIT

} SectorType;

typedef enum {
    SS_A, 
    SS_B,

    SS_LIMIT

} SubType;

typedef enum {
    CT_PRESS,
    CT_RELEASE,
    CT_VALUE,

    CT_LIMIT

} CallbackType;
            
typedef enum: uint32_t { 
    LMB     = 1 << 0,
    MMB     = 1 << 1,
    RMB     = 1 << 2,
    LCTRL   = 1 << 3,
    RCTRL   = 1 << 4,
    LSHIFT  = 1 << 5,
    RSHIFT  = 1 << 6,

} ControlModifier;

typedef enum { 
    CP_COARSE,
    CP_FINE,

    CP_LIMIT

} ControlPrecision;

typedef enum { 
    SP_CURSOR_PRESS,
    SP_CURSOR_PRIOR,
    SP_LT_PRESS,

    SP_LIMIT

} SavedPosition;

typedef enum: uint32_t { 
    BG,
    NG,                         // nodes
    FG,
    SC,                         // controls stencil
    SN,                         // nodes stencil
    ST,                         // staging
    #ifdef DEBUG_OVERLAY
        DO,
    #endif
    CC,                         // controls count

} FieldLayer;

typedef enum: uint32_t {
    VERTICAL    = 1 << 0,
    MOVEABLE    = 1 << 1,
    INTERCON    = 1 << 2,
    INPUT       = 1 << 3,
    OUTPUT      = 1 << 4,
    TRANSPARENT = 1 << 5,
    RADIO       = 1 << 6,

} SectorFlags;

typedef enum {
    ROOT        = 1 << 0,

} FieldFlags;

typedef enum { PP = 0, PT = 8, MP = 16, MT = 24 } shift;

typedef struct 
{
    uint32_t mt: 8;
    uint32_t mp: 8;
    uint32_t pt: 8;
    uint32_t pp: 8;

} uid32;

typedef struct 
{
    float x;
    float y;
    
} point;

typedef struct 
{
    uint32_t x;
    uint32_t y;
    
} point32u;

typedef struct 
{
    int32_t x;
    int32_t y;
    
} point32s;

typedef struct 
{
    uint32_t l;
    uint32_t t;
    uint32_t r;
    uint32_t b;

} ltrb32u;

typedef struct 
{
    uint32_t l;
    uint32_t t;
    uint32_t w;
    uint32_t h;

} ltwh32u;

typedef struct
{
    uint32_t* data;
    uint32_t width;
    uint32_t height;             

} frame;

typedef struct
{
    frame*   data;
    uint32_t width;
    uint32_t height;  
    uint32_t nframes;           

} sprite;

/*****************************************************************************************************************************/
typedef struct field field;
typedef struct sector sector;
typedef struct checkbox checkbox;
typedef struct slider slider;
typedef struct momentary momentary;
typedef struct rotary rotary;
typedef struct socket socket;

struct sector {
    uint32_t    index;
    uint32_t    uid;
    ltrb32u     bounds;
    uint32_t    width;
    uint32_t    height;
    SectorType  type;
    void*       data;                   // Type dependent data
    void*       extension;
    float       value[CP_LIMIT];
    float       memory[CP_LIMIT];
    float       range[2];
    bool        visible;
    bool        hovered;
    bool        repaint;
    bool        staging;
    bool        on;
    bool        connected;
    bool        has_data;
    uint32_t    flags;
    uint32_t    nodes;
    uint32_t    capacity;
    void        (*callback[CT_LIMIT])(sector*, sector*);
    sector*     target[CT_LIMIT];
    sector*     connection;
    field*      carrier;
    sector*     root;
    sector**    node;
};

struct checkbox {
    uint32_t radio_id;  
};

struct slider {
    SubType type;
    float default_value;
    float step[CP_LIMIT];
};

struct rotary {
    float default_value;
    float step[CP_LIMIT];
};

struct socket {

};


struct momentary {

};

/*****************************************************************************************************************************/

struct field {
    ltrb32u     bounds;
    uint32_t    width;
    uint32_t    height;
    point32s    memory[SP_LIMIT];   // Saved cursor position
    frame*      layer[CC];      
    sector*     at;                 // Controls array
    sector*     pressed;
    uint32_t    current;            // HitTest return
    uint32_t    prior;              // Last HitTest
    bool        repaint;            // Repaint flag
    bool        drag;               // Sector drag flag
    bool        move;               // Window drag flag
    bool        staging;            // Draw staging layer
    bool        connecting;         // Connection pending
    uint32_t    sectors;
    uint32_t    capacity;
    uint32_t    flags;
    uint32_t    step;
};

/******************************************************************************************************************************
 * Helpers
 *****************************************************************************************************************************/ 
uint8_t extract_byte(uint32_t, uint_fast8_t);
uint32_t encode_uid(uint8_t, uint8_t, uint8_t, uint8_t);
uid32 decode_uid(uint32_t);
point32u uv_to_screen(float, float, uint32_t, uint32_t);
point screen_to_uv(uint32_t, uint32_t, uint32_t, uint32_t);


/******************************************************************************************************************************
 * Returns 8-bit value
 * Data : 0x FF FF FF FF
 * index:     3  2  1  0     MSB->LSB
 * *****************************************************************************************************************************/
uint8_t extract_byte(uint32_t value, uint_fast8_t byte) {
    return (value >> (byte * 8)) & 0xFF;
}

point32u uv_to_screen(float x, float y, uint32_t w, uint32_t h) {
    point32u s = {
        .x = (uint32_t)(x * (float)w),
        .y = (uint32_t)(y * (float)h)
    };
    return s;
}

point screen_to_uv(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    point n = {
        .x =  (float)x / (float)w,
        .y =  1.0f - (float)y / (float)h
    };
    return n;
}

/******************************************************************************************************************************
 * Returns 32-bit identifier [ AA BB CC DD  ]
 * AA : Module type
 * BB : Module id/position
 * CC : Parameter type
 * DD : Parameter id/position
 * *****************************************************************************************************************************/
uint32_t encode_uid(uint8_t mt, uint8_t mp, uint8_t pt, uint8_t pp)
{
    return ((mt << 24) | (mp << 16) | (pt << 8) | pp);
}

uid32 decode_uid(uint32_t data)
{
    uid32 uid = {
        .mt = extract_byte(data, MT),
        .mp = extract_byte(data, MP),
        .pt = extract_byte(data, PT),
        .pp = extract_byte(data, PP)
    };

    return uid;
}
/******************************************************************************************************************************
 * Primitives
 *****************************************************************************************************************************/ 
typedef enum
{
    BACKGROUND          = 0x263238FF,
    MINOR               = 0x232935FF,
    MAJOR               = 0x162228FF,
    FOREGROUND          = 0xB0BEC5FF,
    TEXT                = 0x607D8BFF,
    SELECTIONBACKGROUND = 0x546E7AFF,
    SELECTIONFOREGROUND = 0xFFFFFFFF,
    BUTTONS             = 0x2E3C43FF,
    SECONDBACKGROUND    = 0x32424AFF,
    DISABLED            = 0x415967FF,
    CONTRAST            = 0x1E272CFF,
    ACTIVE              = 0x314549FF,
    BORDER              = 0x2A373EFF,
    HIGHLIGHT           = 0x425B67FF,
    NOTIFICATIONS       = 0x1E272CFF,
    ACCENT              = 0x009688FF,
    EXCLUDED            = 0x2E3C43FF,
    GREEN               = 0xC3E88DFF,
    YELLOW              = 0xFFCB6BFF,
    BLUE                = 0x82AAFFFF,
    RED                 = 0xF07178FF,
    PURPLE              = 0xC792EAFF,
    ORANGE              = 0xF78C6CFF,
    CYAN                = 0x89DDFFFF,
    GRAY                = 0x546E7AFF,
    ERROR               = 0xFF5370FF,
    COMMENTS            = 0x546E7AFF,
    VARIABLES           = 0xEEFFFFFF,
    LINKS               = 0x80CBC4FF,
    FUNCTIONS           = 0x82AAFFFF,
    KEYWORDS            = 0xC792EAFF,
    TAGS                = 0xF07178FF,
    STRINGS             = 0xC3E88DFF,
    OPERATORS           = 0x89DDFFFF,
    ATTRIBUTES          = 0xFFCB6BFF,
    NUMBERS             = 0xF78C6CFF,
    PARAMETERS          = 0xF78C6CFF,
    SOCKET_IN           = 0x006658FF,
    SOCKET_OUT          = 0x905158FF,

} Palette;

typedef enum {
    GRIP    = 3,
    GAP     = 2,
    SOCKET_BORDER = 2,

} Constrain;

extern const uint8_t gtFont[];

static void draw_glyph      (frame*, const uint8_t*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_text_label (frame*, const uint8_t* , const char*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_rectangle  (frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_rect_f     (frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_line_v     (frame*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_line_h     (frame*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_circle_f   (frame*, uint32_t, uint32_t, uint32_t, uint32_t);
static void draw_circle_o   (frame*, uint32_t, uint32_t, uint32_t, uint32_t);
static void draw_ltrb_o     (frame*, ltrb32u*, const uint32_t);
static void draw_ltrb_f     (frame*, ltrb32u*, const uint32_t);
static uint32_t frame_get   (frame*, uint32_t, uint32_t);
static void frame_set       (frame*, uint32_t, uint32_t, uint32_t);
static void frame_fill      (frame*, uint32_t);
static void frame_clr       (frame*);
static void frame_init      (frame*, uint32_t, uint32_t);
static void frame_flush     (frame*);
static void frame_copy      (frame*, frame*);
static void frame_copy_at   (frame*, frame*, uint32_t, uint32_t);
static void sprite_init     (sprite*, uint32_t, uint32_t, uint32_t);
static void sprite_flush    (sprite*);
static void sprite_load_stripe(sprite*, frame*);

static inline void frame_set(frame*o, uint32_t x, uint32_t y, uint32_t value)
{
    if(x < o->width && y < o->height)
    o->data[x + y * o->width] = value;
}

static inline uint32_t frame_get(frame* o, uint32_t x, uint32_t y)
{
    if(x < o->width && y < o->height) return o->data[x + y * o->width];
    return o->data[0];
}

static inline void frame_clr(frame* o)
{
    memset(o->data, 0, o->height * o->width * sizeof(uint32_t));
}

static inline void frame_fill(frame* o, uint32_t value)
{
    for(uint32_t i = 0; i < (o->height * o->width); ++i) o->data[i] = value;
}

static inline void frame_init(frame* o, uint32_t x, uint32_t y)
{
    o->width  = x;
    o->height = y;
    o->data   = (uint32_t*)calloc(o->width * o->height , sizeof(uint32_t));
}

static inline void frame_flush(frame* o)
{
    free(o->data);
}

static inline void frame_copy(frame* target, frame* source) {
    for(uint32_t i = 0; i < target->height * target->width; i++) target->data[i] = source->data[i];
}

static inline void frame_copy_at(frame* target, frame* source, uint32_t xo, uint32_t yo) {
    for(uint32_t y = 0; y < source->height; y++) {
        for(uint32_t x = 0; x < source->width; x++) {
            frame_set(target, x + xo, y + yo, frame_get(source, x, y));
        }
    }
}

static inline void draw_rectangle(frame* restrict canvas, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t i = l; i <= r; i++) {
        frame_set(canvas, i, t, colour);
        frame_set(canvas, i, b, colour);
    }

    for(uint32_t i = t; i <= b; i++) {
        frame_set(canvas, l, i, colour);
        frame_set(canvas, r, i, colour);
    }
}

static inline void draw_rect_f(frame* canvas, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t y = t; y <= b; ++y) {
        for(uint32_t x = l; x <= r; ++x) {
            frame_set(canvas, x, y, colour);
        }
    }
}

static inline void draw_line_v(frame* o, uint32_t x, uint32_t yo, uint32_t ye, const uint32_t value) {
    for(uint32_t y = yo; y <= ye; ++y)
        frame_set(o, x, y, value);
}

static inline void draw_line_h(frame* o, uint32_t y, uint32_t xo, uint32_t xe, const uint32_t value) {
    for(uint32_t x = xo; x <= xe; ++x)
        frame_set(o, x, y, value);
}

void draw_circle_f(frame* o, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
    int x = 0;
    int y = r;
    int p = 1 - r;

    while (x <= y) {
        ++x;
        if (p < 0) {
            p = p + 2 * x + 1;
        } 
        else{
            --y;
            p = p + 2 * (x - y) + 1;
        }

        draw_line_h(o, yc + x, xc - y, xc + y, value);
        draw_line_h(o, yc - x, xc - y, xc + y, value);
        draw_line_h(o, yc + y, xc - x, xc + x, value);
        draw_line_h(o, yc - y, xc - x, xc + x, value);
    }

    draw_line_h(o, yc, xc - r, xc + r, value);
}

static inline void draw_circle_o(frame* o, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
    int x = 0;
    int y = r;
    int p = 1 - r;

    while (x <= y) {
        ++x;
        if (p < 0) {
            p = p + 2 * x + 1;
        } 
        else{
            --y;
            p = p + 2 * (x - y) + 1;
        }

        frame_set(o, xc + x, yc + y, value);
        frame_set(o, xc - x, yc + y, value);
        frame_set(o, xc + x, yc - y, value);
        frame_set(o, xc - x, yc - y, value);
        frame_set(o, xc + y, yc + x, value);
        frame_set(o, xc - y, yc + x, value);
        frame_set(o, xc + y, yc - x, value);
        frame_set(o, xc - y, yc - x, value); 

    }

    frame_set(o, xc, yc - r, value);
    frame_set(o, xc, yc + r, value);
    frame_set(o, xc + r, yc, value);
    frame_set(o, xc - r, yc, value);
}

static inline void draw_glyph(frame* canvas, const uint8_t* font, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour) {
    uint32_t pos = id * 7;
    u_int8_t stencil = 0b1;

    for(uint32_t y = 0; y < 8; y++) {
        for(uint32_t x = 0; x < 7; x++) {
            if(font[pos + x] & stencil) frame_set(canvas, x + l, y + t, colour);
        }
        stencil<<=1;
    }
}

static inline void draw_text_label(frame* canvas, const uint8_t* font, const char* text, uint32_t l, uint32_t t, uint32_t, uint32_t, const uint32_t colour) {
    uint32_t n = strlen(text);
    for(uint32_t i = 0; i < n; i++) {
        draw_glyph(canvas, font, text[i] - 32, l + i * 8, t, colour);
    }
}

/******************************************************************************************************************************/

static inline void sprite_init(sprite* o, uint32_t w, uint32_t h, uint32_t nframes) {
    frame temp;
    frame_init(&temp, w, h);

    o->data = (frame*)malloc(nframes * sizeof(temp));
    o->width = w;
    o->height = h;
    o->nframes = nframes;

    frame_flush(&temp);
    
    for(uint32_t i = 0; i < o->nframes; i++) {
        frame_init(&o->data[i], w, h);
    }
}

static inline void sprite_flush(sprite* restrict o) {
    for(uint32_t i = 0; i < o->nframes; i++) {
        frame_flush(&o->data[i]);
    }
    free(o->data);
}

static inline void sprite_load_stripe(sprite* restrict o, frame* restrict f) {
    for(uint32_t i = 0; i < o->nframes; i++) {
        for(uint32_t y = 0; y < o->height; y++) {
            for(uint32_t x = 0; x < o->width; x++) {
                frame_set(&o->data[i], x, y, frame_get(f, x, y + (i * o->height)));
            }
        }
    }
}

static inline void draw_ltrb_o(frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour) {
    for(uint32_t i = r->l; i <= r->r; ++i) {
        frame_set(canvas, i, r->t, colour);
        frame_set(canvas, i, r->b, colour);
    }
    for(uint32_t i = r->t; i <= r->b; i++) {
        frame_set(canvas, r->l, i, colour);
        frame_set(canvas, r->r, i, colour);
    }
}

static inline void draw_ltrb_f(frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour)
{
    for(uint32_t y = r->t; y <= r->b; ++y) {
        for(uint32_t x = r->l; x <= r->r; ++x) {
            frame_set(canvas, x, y, colour);
        }
    }
}

/******************************************************************************************************************************/
void set_text_data(sector*, const char*); 
static void value_to_textbox(sector*, sector*);

void fuse_link(sector*, sector*);
void draw_scene(field* restrict);

void add_mod_link(sector*, sector*, CallbackType, void (*)(sector*, sector*));
void link_sector(sector*, sector*);
void move_sector(sector* restrict, ltrb32u* restrict);
void erase_sector(sector* restrict);

/*****************************************************************************************************************************/

sector* createSector(field* restrict, sector* restrict, SectorType, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void initField   (field* restrict, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void destroyField(field* restrict);

/*****************************************************************************************************************************/

void hit_test_down(field* restrict, int, int, uint32_t);
void hit_test_drag(field* restrict, int, int);
void hit_test_up  (field* restrict, int, int, uint32_t);
void hit_test     (field* restrict, int, int);

/*****************************************************************************************************************************/

extern void (*init_sector[])(sector* restrict);
extern void (*set_sector[])(sector* restrict, int, int);
extern void (*draw_sector[])(sector* restrict);
extern void (*drag_sector[])(sector* restrict, int, int);
extern void (*scroll_sector[])(sector* restrict, int, int);
extern void (*enter_sector[])(sector* restrict, int, int);
extern void (*leave_sector[])(sector* restrict, int, int);
extern void (*release_sector[])(sector* restrict, int, int);

/*****************************************************************************************************************************/

sector* createSector(field* restrict o, sector* restrict node, SectorType type, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t flags) {
    auto pos = ++o->sectors;

    o->at[pos].type                     = type;
 //   o->at[pos].subtype                  = SS_A;
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
//    o->at[pos].step[CP_COARSE]          = 1.0f;
 //   o->at[pos].step[CP_FINE]            = 0.1f;
    o->at[pos].repaint                  = true;
    o->at[pos].flags                    = flags;
    o->at[pos].carrier                  = o;
//    o->at[pos].radio_id                 = 0;
//    o->at[pos].default_value            = 0.0f;

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
        o->at[pos].callback[i] = &fuse_link;
    }

    if(!(flags & TRANSPARENT)) {
        draw_ltrb_f (
            o->layer[SC],
            &o->at[pos].bounds,
            o->at[pos].index
        );
    }

    init_sector[type](&o->at[pos]);

    if(node)
        link_sector(node, &o->at[pos]);

    return &o->at[pos];
}

void initField(field* restrict o, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t size, uint32_t flags) {
    o->bounds.l     = x;
    o->bounds.t     = y;
    o->bounds.r     = x + w;
    o->bounds.b     = y + h;

    o->width        = w;
    o->height       = h;

    for(int i = 0; i < SP_LIMIT; ++i) {
        o->memory[i] = (point32s) { .x = 0, .y = 0 };
    }

    o->drag         = false;
    o->move         = false;
    o->capacity     = size + 1u;
    o->repaint      = true;
    o->sectors      = 0;
    o->flags        = flags;
    o->step         = 25;
    o->staging      = false;
    o->connecting   = false;
    o->prior        = 0;
    o->current      = 0;
    o->pressed      = nullptr;

    if(true) {
        for(uint32_t i = 0; i < CC; ++i) {
            o->layer[i] = malloc(sizeof(frame));
            frame_init(o->layer[i],  w, h);
        }

        frame_clr(o->layer[SC]);
        frame_clr(o->layer[SN]);
        frame_fill(o->layer[BG], BACKGROUND);
    }

    o->at = malloc(o->capacity * sizeof(sector));
    for(uint32_t i = 0; i < o->capacity; i++) o->at[i].index = i;

    auto canvas = &o->at[0];

    canvas->type = ST_CANVAS;
    canvas->carrier = o;
    canvas->repaint = true;

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
        canvas->callback[i] = &fuse_link;
    }
}

void destroyField(field* restrict o) {
    for(uint32_t i = 0; i < CC; ++i) {
        frame_flush(o->layer[i]);
        free(o->layer[i]);
    }
    free(o->at);
}

void empty() {}

void fuse_link(sector*, sector*) {}

void link_sector(sector* parent, sector* child) {
    assert(parent != child);
    if (parent->nodes >= parent->capacity) {
        parent->capacity = parent->capacity * 2 + 4;
        parent->node = realloc(parent->node, parent->capacity * sizeof(sector*));
    }
    parent->node[parent->nodes] = child;
    child->root = parent;
    ++parent->nodes;
}

void move_sector(sector* restrict s, ltrb32u* restrict bounds) {
    draw_ltrb_f(s->carrier->layer[FG], &s->bounds, 0x0);
    if(!(s->flags & TRANSPARENT)) {
        draw_ltrb_f(s->carrier->layer[SC], &s->bounds, 0x0);
        draw_ltrb_f(s->carrier->layer[SC], bounds, s->index);
    }
    s->bounds = *bounds;
}

void add_mod_link(sector* source, sector* target, CallbackType type, void (*fn)(sector*, sector*)) {
    source->callback[type] = fn;
    source->target[type] = target;
}

void set_text_data(sector* restrict o, const char* restrict text) {
    strncpy(o->data, text, TEXTBOX_SIZE);
    o->repaint = true;
}

static inline void value_to_textbox(sector* slider, sector* tbox) {
    int precision = 2;
    char *text = tbox->data;
    float val = slider->value[CP_COARSE] + slider->value[CP_FINE];
    snprintf(text, TEXTBOX_SIZE, "%.*f", precision, val);
    tbox->repaint = true;
}

/*****************************************************************************************************************************/

void hit_test_down(field* restrict o, int x, int y, uint32_t button) {
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
    o->prior = o->current;
        
    p->callback[CT_PRESS](p, p->target[CT_PRESS]);
}

#ifdef DEBUG_OVERLAY
static inline void draw_debug_overlay(field* o, sector* s) {
    constexpr uint32_t colour = 0xFF'FF'FF'90;
    frame_fill(o->layer[DO], 0);
    uint32_t voffset = 10;
    uint32_t vstep = 12;
    uint32_t row = 0;

    draw_text_label(o->layer[DO], gtFont,    "#", 10, voffset + vstep * row++, 100, 10, colour);

    if(!s) return;

    char buffer[128];
    snprintf(buffer, 128, "CONTROL INDEX: %u", s->index);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CONTROL UID  : 0x%X", s->uid);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "IS CONNECTED : %d", s->connected);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CONNECTION   : %p", (void*)s->connection);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "HAS DATA     : %d", s->has_data);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "VALUE COARSE : %f", s->value[0]);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "VALUE FINE   : %f", s->value[1]);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "FLAGS        : %b", s->flags);
    draw_text_label(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    atomic_store_explicit(&force_repaint, true, memory_order_release);
}
#endif

void hit_test(field* restrict o, int x, int y) {
    auto uid = frame_get(o->layer[SC], x, y);

    if(o->current != uid) {
        #ifdef DEBUG_OVERLAY
            draw_debug_overlay(o, &o->at[uid]);
        #endif

        o->prior = o->current;
        o->current = uid;
       
        auto prior = &o->at[o->prior];
        prior->hovered = false;
        prior->repaint = true;
        leave_sector[prior->type](prior, x, y);
        
        auto current = &o->at[o->current];
        current->hovered = true;
        current->repaint = true;
        enter_sector[current->type](current, x, y);
        
        o->repaint = true;
    }
}

void hit_test_drag(field* restrict o, int x, int y) {
    drag_sector[o->pressed->type](o->pressed, x, y);
    o->repaint = true;
}

void hit_test_up(field* restrict o, int x, int y, uint32_t button) {
    o->current = frame_get(o->layer[SC], x, y);

    if(o->pressed) {
        release_sector[o->pressed->type](o->pressed, x, y);
        o->pressed = nullptr;
    }
    else {
        auto s = &o->at[o->current];
        release_sector[s->type](s, x, y);
    }

    o->drag = false;
    o->move = false;
    o->connecting = false;
    o->repaint = true;

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

static void draw_checkbox(sector* restrict c) {
    auto o = c->carrier;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);
    if(c->value[CP_COARSE] > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_scene(field* restrict o) {
    for(uint32_t i = 0; i <= o->sectors; ++i) {
        auto s = &o->at[i];
        if(s->repaint) {
            draw_sector[s->type](s);
        }
    }
    o->repaint = false;
}

/*****************************************************************************************************************************/

static void set_checkbox(sector* restrict o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    auto ext = (checkbox*)o->extension;
    if(o->flags & RADIO) {
        auto id = ext->radio_id;
        if(*coarse > 0.5f) return;
        else {
            *coarse = 1.0f;
            o->repaint = true;
            auto f = o->carrier;
            for(uint32_t i = 0; i <= f->sectors; ++i) {
                auto r = &f->at[i];
                if((r->flags & RADIO) && ext->radio_id == id && r != o) {
                    r->value[CP_COARSE] = 0.0f;
                    r->repaint = true;
                    printf("Turned off : %d\n", r->index);
                }
            }
        }
    }
    else {
        if (*coarse < 0.5f) *coarse = 1.0f;
        else *coarse = 0.0f;
    }
    o->repaint = true;
}


static void set_slider(sector* restrict o, int x, int y) {
    const auto coarse = &o->value[CP_COARSE];
    const auto fine = &o->value[CP_FINE];
    auto ext = (slider*)o->extension;

    const float step = ext->step[CP_COARSE];
    const float sensivity = 0.2f;

    auto dx = o->carrier->memory[SP_CURSOR_PRIOR].x - x;
    auto dy = o->carrier->memory[SP_CURSOR_PRIOR].y - y;

    auto delta = (o->flags & VERTICAL ? dy : -dx);
    int ds = (int)roundf((float)delta * sensivity);
    float value = o->memory[CP_COARSE] + ds * step;

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

static void draw_slider(sector* restrict c) {
    auto o = c->carrier;
    auto ext = (slider*)c->extension;
    auto coarse = &c->value[CP_COARSE];
    auto fine = &c->value[CP_FINE];
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    float frac = (*coarse - c->range[0]) / (c->range[1] - c->range[0]);

    switch(ext->type) {
        case SS_A:
            if(c->flags & VERTICAL) {
                int h   = c->bounds.b - c->bounds.t - GRIP * 2 - GAP * 2;
                int pos = (int)((1.0f - frac) * (float)h) + c->bounds.t + GRIP + GAP;
                draw_rect_f(o->layer[FG], c->bounds.l + GAP, pos - GRIP,
                            c->bounds.r - GAP, pos + GRIP, HIGHLIGHT);
            } else {
                int w   = c->bounds.r - c->bounds.l - GRIP * 2 - GAP * 2;
                int pos = (int)(frac * (float)w) + c->bounds.l + GRIP + GAP;
                draw_rect_f(o->layer[FG], pos - GRIP, c->bounds.t + GAP,
                            pos + GRIP, c->bounds.b - GAP, HIGHLIGHT);
            }
            break;

        case SS_B:
            if(c->flags & VERTICAL) {
                int h   = c->bounds.b - c->bounds.t - GAP * 2;
                int pos = (int)((1.0f - frac) * (float)h) + c->bounds.t + GAP;
                draw_rect_f(o->layer[FG], c->bounds.l + GAP, pos,
                            c->bounds.r - GAP, c->bounds.b - GAP, HIGHLIGHT);
            } else {
                int w   = c->bounds.r - c->bounds.l - GAP * 2;
                int pos = (int)(frac * (float)w) + c->bounds.l + GAP;
                draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP,
                            pos, c->bounds.b - GAP, HIGHLIGHT);
            }
            break;

        case SS_LIMIT:
        default:
            break;
    }
    c->repaint = false;
}

static void scroll_slider(sector* restrict o, int, int y) {
    auto fine = &o->value[CP_FINE];
    auto coarse = &o->value[CP_COARSE];
    auto ext = (slider*)o->extension;
    auto df = (float)y * ext->step[CP_FINE];

    if((*coarse + df < o->range[1]) && (*coarse + df > o->range[0])) {
        *fine += df;
    }

    if((int)*fine != 0) {
        *coarse += (int)*fine;
        *fine = 0.0f;
    };

    o->repaint = true;
    o->callback[CT_VALUE](o, o->target[CT_VALUE]);
}

static void draw_button(sector* restrict c) {
    auto o = c->carrier;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    if(c->hovered)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACTIVE);

    if(c->value[CP_COARSE] > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACCENT);

    c->repaint = false;
}

/******************************************************************************************************************************/

static void draw_rotary(sector* restrict c) {
    auto o = c->carrier;
    auto range = c->range[1] - c->range[0];
    sprite* spr = (sprite*)c->data;
    int f = ((c->value[CP_COARSE] - c->range[0]) / range) * spr->nframes;
    frame_copy_at(o->layer[FG], &spr->data[f], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void set_rotary(sector* restrict o, int x, int y) {
    auto f = o->carrier;
    auto coarse = &o->value[CP_COARSE];
    auto ext = (rotary*)o->extension;
    int dy = roundf((f->memory[SP_CURSOR_PRIOR].y - y)/ext->step[CP_COARSE]);
    int dx = roundf((x - f->memory[SP_CURSOR_PRIOR].x)/ext->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < o->range[0]) *coarse = o->range[0];
    else if(*coarse > o->range[1]) *coarse = o->range[1];
    o->repaint = true;

    f->memory[SP_CURSOR_PRIOR].x = (float)x;
    f->memory[SP_CURSOR_PRIOR].y = (float)y;
}

/******************************************************************************************************************************/

static void set_sprite_inf_slider(sector* restrict o, int x, int y) {
    auto f = o->carrier;
    auto coarse = &o->value[CP_COARSE];
    auto ext = (rotary*)o->extension;
    int dy = roundf((y - f->memory[SP_CURSOR_PRIOR].y)/ext->step[CP_COARSE]);
    int dx = roundf((f->memory[SP_CURSOR_PRIOR].x - x)/ext->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < o->range[0]) *coarse += o->range[0];
    else if(*coarse > o->range[1]) *coarse -= o->range[1];

    // if (o->at[o->current].value <= 0.0f) o->at[o->current].value = o->at[o->current].range;
    // else if (o->at[o->current].value >= o->at[o->current].range) o->at[o->current].value = 0.0f;

    o->repaint = true;
    f->memory[SP_CURSOR_PRIOR].x = (float)x;
    f->memory[SP_CURSOR_PRIOR].y = (float)y;
}

static void set_button(sector* restrict o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    o->repaint = true;

    auto f = o->carrier;
    f->prior = f->current;
}

static void release_button(sector* restrict o, int, int) {
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

static void draw_sprite_button(sector* restrict c) {
    auto o = c->carrier;
    sprite* spr = (sprite*)c->data;

    if(c->value[CP_COARSE] > 0.5f)
        frame_copy_at(o->layer[FG], &spr->data[1], c->bounds.l, c->bounds.t);
    else
        frame_copy_at(o->layer[FG], &spr->data[0], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void init_socket(sector* restrict s) {
    s->data = (float*)calloc(SPLINE_SEGMENTS * 2, sizeof(float));
}

static void set_socket(sector* restrict o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    o->repaint = true;

    auto f = o->carrier;
    f->prior = f->current;
}

static void draw_socket(sector* restrict c) {
    auto o = c->carrier;
    auto r = c->width / 2;
    point32u center = {
        .x = c->bounds.l + r,
        .y = c->bounds.t + r
    };
    auto color = c->flags & OUTPUT ? SOCKET_OUT : SOCKET_IN;

    draw_circle_f(o->layer[FG], center.x, center.y, r, color);
    r = c->width / 4;
    draw_circle_f(o->layer[FG], center.x, center.y, r, BUTTONS);
    c->repaint = false;
}

static inline point interpolate_bezier(point a, point b, point c, point d, float t)
{
    float ti = 1.0f - t;
    float tsq = t * t;
    float isq = ti * ti;
    
    float fa = isq * ti;
    float fb = 3.0f * t * isq;
    float fc = 3.0f * tsq * ti;
    float fd = tsq * t;
    
    point o = {
        .x = fa * a.x + fb * b.x + fc * c.x + fd * d.x,
        .y = fa * a.y + fb * b.y + fc * c.y + fd * d.y
    };

    return o;
}

static void drag_socket(sector* s, int x, int y)
{
    if(s->connected) {
        s->carrier->pressed = s->connection;
        s = s->connection;
        s->carrier->current = s->index;

        if(s->has_data) {
            memset(s->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float)); 
            s->connection->has_data = false;
        }
        if(s->connection->has_data) {
            memset(s->connection->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            s->connection->has_data = false;
        }
        s->connection->connected = false;
        s->connected = false;

    }

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

    const float inc = 1.0f / (float)(SPLINE_SEGMENTS - 1) ;
    float t = 0.0;

    auto uva = screen_to_uv(a.x, a.y, o->width, o->height);
    auto uvb = screen_to_uv(b.x, b.y, o->width, o->height);
    auto uvc = screen_to_uv(c.x, c.y, o->width, o->height);
    auto uvd = screen_to_uv(d.x, d.y, o->width, o->height);

    for(uint32_t i = 0, j = 0; i < SPLINE_SEGMENTS; i++) {
        point uvp = interpolate_bezier(uva, uvb, uvc, uvd, t);

        ((float*)s->data)[j++] = uvp.x;
        ((float*)s->data)[j++] = uvp.y;

        t += inc;
    }

    s->has_data = true;
}

static void drag_cord(sector* restrict s, int x, int y)
{
    if(s->connected) {
        if(s->has_data) {
            memset(s->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }
        if(s->connection->has_data) {
            memset(s->connection->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            s->connection->has_data = false;
            s->connection->connected = false;
        }
    }

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

    const float inc = 1.0f / (float)(SPLINE_SEGMENTS - 1) ;
    float t = 0.0;

    auto uva = screen_to_uv(a.x, a.y, o->width, o->height);
    auto uvb = screen_to_uv(b.x, b.y, o->width, o->height);
    auto uvc = screen_to_uv(c.x, c.y, o->width, o->height);
    auto uvd = screen_to_uv(d.x, d.y, o->width, o->height);

    for(uint32_t i = 0, j = 0; i < SPLINE_SEGMENTS; i++) {
        point uvp = interpolate_bezier(uva, uvb, uvc, uvd, t);

        ((float*)s->data)[j++] = uvp.x;
        ((float*)s->data)[j++] = uvp.y;

        t += inc;
    }

    s->has_data = true;
}

inline static void connect(sector* restrict source, sector* restrict target) {
    if(target->connected) {
        if(target->has_data) {
            memset(target->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            target->has_data = false;
            target->connected = false;
        }
        if(target->connection->has_data) {
            memset(source->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            target->connection->has_data = false;
            target->connection->connected = false;
        }
    }
    drag_cord(source, target->bounds.l + target->width / 2, target->bounds.t + target->height / 2);
    source->connected = true;
    target->connected = true;
    source->connection = target;
    target->connection = source;
    source->hovered = false;
    target->hovered = true;
    source->carrier->connecting = false;
}

inline static void disconnect(sector* restrict o) {
    auto target = o->connection;

    if(o->has_data) {
        memset(o->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        o->has_data = false;
    }
    o->connected = false;

    if(target) {
        if(target->has_data) {
            memset(o->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }

        target->connected = false;
        target->has_data = false;
        o->connection->connection = nullptr;
        o->connection = nullptr;
    }
}

static void release_socket(sector* restrict o, int x, int y) {
    auto carrier = o->carrier;
    auto target_id = frame_get(carrier->layer[SC], x, y);
    auto target = &carrier->at[target_id];
    
    if(target->connected) disconnect(target);

    if((o->flags & OUTPUT) && (target->flags & INPUT)) {
        connect(o, target);
        return;
    }
    else if((o->flags & INPUT) && (target->flags & OUTPUT)) {
        connect(o, target);
        return;
    }
    
    disconnect(o);
}

/*****************************************************************************************************************************/

static void init_node(sector* restrict s) {
    s->data = (frame*)calloc(1, sizeof(frame));
    frame_init((frame*)s->data, s->width, s->height);
    draw_ltrb_f(s->carrier->layer[SN], &s->bounds, s->index);
}

static void set_node(sector* restrict o, int, int) {
    o->repaint = true;
    auto f = o->carrier;
    f->prior = f->current;
}

static inline bool has_overlap(const sector* restrict c) {
    const auto l = c->carrier->layer[SN];
    for(uint32_t y = c->bounds.t; y < c->bounds.b; ++y) {
        for(uint32_t x = c->bounds.l; x < c->bounds.r; ++x) {
            auto v = frame_get(l, x, y);
            if(v && v != c->index)
                return true;
        }
    }
    return false;
}

static void draw_node(sector* restrict c) {
    auto o = c->carrier;
    auto l = c->staging ? ST : NG;

    if(l == ST) {
        frame_clr(o->layer[l]);
        draw_ltrb_o(o->layer[l], &c->bounds, has_overlap(c) ? ERROR : HIGHLIGHT);
    }
    else {
        draw_ltrb_f(o->layer[l], &c->bounds, SECONDBACKGROUND);
        draw_ltrb_o(o->layer[l], &c->bounds, BORDER);
    }
    c->repaint = false;
}

static void drag_node(sector* restrict o, int x, int y)
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
    }
}

static void release_node(sector* restrict s, int, int) 
{
    auto overlap = has_overlap(s);

    ltrb32u ir = {
        .l = s->carrier->memory[SP_LT_PRESS].x, 
        .t = s->carrier->memory[SP_LT_PRESS].y, 
        .r = s->carrier->memory[SP_LT_PRESS].x + s->width, 
        .b = s->carrier->memory[SP_LT_PRESS].y + s->height
    };

    if(overlap) { 
        s->bounds = ir;
        draw_ltrb_f(s->carrier->layer[SC], &ir, s->index);
        draw_ltrb_f(s->carrier->layer[SN], &ir, s->index);
        draw_ltrb_f(s->carrier->layer[NG], &ir, 0x0);

        for(uint32_t i = 0; i < s->nodes; ++i) {
            s->node[i]->repaint = true;
        }
    }
    else {
        draw_ltrb_f(s->carrier->layer[SC], &ir, 0x0);
        draw_ltrb_f(s->carrier->layer[SN], &ir, 0x0);
        draw_ltrb_f(s->carrier->layer[SC], &s->bounds, s->index);
        draw_ltrb_f(s->carrier->layer[SN], &s->bounds, s->index);

        draw_ltrb_f(s->carrier->layer[NG], &ir, 0x0);

        int dx = s->bounds.l - s->carrier->memory[SP_LT_PRESS].x;
        int dy = s->bounds.t - s->carrier->memory[SP_LT_PRESS].y;

        for(uint32_t i = 0; i < s->nodes; ++i) {
            auto node = s->node[i];

            ltrb32u r = {
                .l = node->bounds.l + dx,
                .t = node->bounds.t + dy,
                .r = node->bounds.r + dx,
                .b = node->bounds.b + dy
            };

            move_sector(node, &r); 

            if(node->connected) {
                auto target = node->connection;
                if(node->has_data) {
                    drag_cord(node, target->bounds.l + target->width / 2, target->bounds.t + target->height / 2);
                }
                else if(target->has_data) {
                    drag_cord(target, node->bounds.l + node->width / 2, node->bounds.t + node->height / 2);
                }

            }

            node->repaint = true;
        }
    }

    frame_clr(s->carrier->layer[ST]);
    s->carrier->staging = false;
    s->staging = false;
    s->repaint = true;
    s->carrier->repaint = true;
}

/*****************************************************************************************************************************/

static void init_textbox(sector* restrict s) {
    s->data = (char*)calloc(TEXTBOX_SIZE, sizeof(char)); 
}

static void set_textbox(sector* restrict o, int, int) {
    o->repaint = true;
}

static void draw_textbox(sector* restrict c) {
    auto o = c->carrier;
    draw_ltrb_f(o->layer[FG], &c->bounds, SECONDBACKGROUND);
    char *text = c->data;
    //auto len = strlen(text);
    //auto offset_x = (c->width - 10 * len) / 2;
    auto offset_y = (c->height - 8) / 2;

    draw_text_label(
        o->layer[FG],
        gtFont,
        text,
        c->bounds.l,
        c->bounds.t + offset_y,
        0,
        0,
        TEXT
    );

    c->repaint = false;
}

/*****************************************************************************************************************************/

static void draw_canvas(sector* restrict s) 
{
    auto o = s->carrier;
    frame_fill(o->layer[BG], BACKGROUND);
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

static inline void init_none(sector* restrict) {}

void (*init_sector[])(sector* restrict)  = {
    [ST_SLIDER]                 = init_none,  
    [ST_ROTARY]                 = init_none,  
    [ST_SPRITE_INF_SLIDER]      = init_none,  
    [ST_SOCKET]                 = init_socket,
    [ST_CHECKBOX]               = init_none,  
    [ST_MOMENTARY]              = init_none,  
    [ST_SPRITE_CHECKBOX]        = init_none,  
    [ST_SPRITE_BUTTON]          = init_none,  
    [ST_CANVAS]                 = init_none,  
    [ST_TEXTBOX]                = init_textbox,  
    [ST_NODE]                   = init_node,  
};

static inline void set_none(sector* restrict, int, int) {}

void (*set_sector[])(sector* restrict, int, int) = {
    [ST_SLIDER]                 = set_slider,            
    [ST_ROTARY]                 = set_rotary,     
    [ST_SPRITE_INF_SLIDER]      = set_sprite_inf_slider, 
    [ST_SOCKET]                 = set_socket,            
    [ST_CHECKBOX]               = set_checkbox,          
    [ST_MOMENTARY]              = set_button,            
    [ST_SPRITE_CHECKBOX]        = set_checkbox,          
    [ST_SPRITE_BUTTON]          = set_button,            
    [ST_CANVAS]                 = set_none,              
    [ST_TEXTBOX]                = set_textbox,
    [ST_NODE]                   = set_node               
};

void (*draw_sector[])(sector* restrict) = {
    [ST_SLIDER]                 = draw_slider,        
    [ST_ROTARY]                 = draw_rotary, 
    [ST_SPRITE_INF_SLIDER]      = draw_rotary, 
    [ST_SOCKET]                 = draw_socket,        
    [ST_CHECKBOX]               = draw_checkbox,      
    [ST_MOMENTARY]              = draw_button,        
    [ST_SPRITE_CHECKBOX]        = draw_sprite_button, 
    [ST_SPRITE_BUTTON]          = draw_sprite_button, 
    [ST_CANVAS]                 = draw_canvas,        
    [ST_TEXTBOX]                = draw_textbox,
    [ST_NODE]                   = draw_node           
};

static inline void drag_none(sector* restrict, int, int) {}

void (*drag_sector[])(sector* restrict, int, int) = {
    [ST_SLIDER]                 = set_slider,           
    [ST_ROTARY]                 = set_rotary,    
    [ST_SPRITE_INF_SLIDER]      = set_sprite_inf_slider,
    [ST_SOCKET]                 = drag_socket,          
    [ST_CHECKBOX]               = drag_none,             
    [ST_MOMENTARY]              = drag_none,             
    [ST_SPRITE_CHECKBOX]        = drag_none,             
    [ST_SPRITE_BUTTON]          = drag_none,             
    [ST_CANVAS]                 = drag_none,             
    [ST_TEXTBOX]                = drag_none,  
    [ST_NODE]                   = drag_node             
};

static inline void scroll_none(sector* restrict, int, int) {}

void (*scroll_sector[])(sector* restrict, int, int) = {
    [ST_SLIDER]                 = scroll_slider,     
    [ST_ROTARY]                 = scroll_slider,     
    [ST_SPRITE_INF_SLIDER]      = scroll_slider,     
    [ST_SOCKET]                 = scroll_none,          
    [ST_CHECKBOX]               = scroll_none,          
    [ST_MOMENTARY]              = scroll_none,          
    [ST_SPRITE_CHECKBOX]        = scroll_none,          
    [ST_SPRITE_BUTTON]          = scroll_none,          
    [ST_CANVAS]                 = scroll_none,          
    [ST_TEXTBOX]                = scroll_none,  
    [ST_NODE]                   = scroll_none 
};

static inline void enter_none(sector* restrict, int, int) {}

void (*enter_sector[])(sector* restrict, int, int) = {
    [ST_SLIDER]                 = enter_none,         
    [ST_ROTARY]                 = enter_none,         
    [ST_SPRITE_INF_SLIDER]      = enter_none,         
    [ST_SOCKET]                 = enter_none,         
    [ST_CHECKBOX]               = enter_none,         
    [ST_MOMENTARY]              = enter_none,   
    [ST_SPRITE_CHECKBOX]        = enter_none,         
    [ST_SPRITE_BUTTON]          = enter_none,   
    [ST_CANVAS]                 = enter_none,         
    [ST_TEXTBOX]                = enter_none,  
    [ST_NODE]                   = enter_none 
};
static inline void leave_none(sector* restrict, int, int) {}

void (*leave_sector[])(sector* restrict, int, int) = {
    [ST_SLIDER]                 = leave_none,         
    [ST_ROTARY]                 = leave_none,         
    [ST_SPRITE_INF_SLIDER]      = leave_none,         
    [ST_SOCKET]                 = leave_none,         
    [ST_CHECKBOX]               = leave_none,         
    [ST_MOMENTARY]              = leave_none,   
    [ST_SPRITE_CHECKBOX]        = leave_none,         
    [ST_SPRITE_BUTTON]          = leave_none,   
    [ST_CANVAS]                 = leave_none,         
    [ST_TEXTBOX]                = leave_none,  
    [ST_NODE]                   = leave_none          
};

static inline void release_none(sector* restrict, int, int) {}

void (*release_sector[])(sector* restrict, int, int) = {
    [ST_SLIDER]                 = release_none,
    [ST_ROTARY]                 = release_none,
    [ST_SPRITE_INF_SLIDER]      = release_none,
    [ST_SOCKET]                 = release_socket,
    [ST_CHECKBOX]               = release_none,
    [ST_MOMENTARY]              = release_button,
    [ST_SPRITE_CHECKBOX]        = release_none,
    [ST_SPRITE_BUTTON]          = release_button,
    [ST_CANVAS]                 = release_none,
    [ST_TEXTBOX]                = release_none,  
    [ST_NODE]                   = release_node 
}; 

const uint8_t gtFont[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0  = space
  0x00,0x00,0x00,0x5F,0x00,0x00,0x00, // 1  = !
  0x00,0x00,0x07,0x00,0x07,0x00,0x00, // 2  = "
  0x14,0x14,0x7f,0x14,0x7f,0x14,0x14, // 3  = #
  0x2E,0x2A,0x2A,0x6B,0x2A,0x2A,0x3A, // 4  = $
  0x43,0x23,0x10,0x08,0x04,0x62,0x61, // 5  = %
  0x00,0x34,0x4A,0x52,0x24,0x40,0x00, // 6  = &
  0x00,0x00,0x00,0x07,0x00,0x00,0x00, // 7  = '
  0x00,0x00,0x1C,0x22,0x41,0x00,0x00, // 8  = (
  0x00,0x00,0x41,0x22,0x1C,0x00,0x00, // 9  = )
  0x00,0x08,0x2A,0x1C,0x2A,0x08,0x00, // 10 = *
  0x00,0x08,0x08,0x3e,0x08,0x08,0x00, // 11 = +
  0x00,0x00,0x00,0x60,0x00,0x00,0x00, // 12 = ,
  0x00,0x08,0x08,0x08,0x08,0x08,0x08, // 13 = -
  0x00,0x00,0x00,0x20,0x00,0x00,0x00, // 14 = .
  0x40,0x20,0x10,0x08,0x04,0x02,0x01, // 15 = /
  0x1c,0x22,0x41,0x41,0x41,0x22,0x1c, // 16 = 0
  0x41,0x41,0x41,0x7f,0x40,0x40,0x40, // 17 = 1
  0x71,0x49,0x49,0x49,0x49,0x49,0x46, // 18 = 2
  0x41,0x41,0x41,0x49,0x49,0x49,0x36, // 19 = 3
  0x18,0x14,0x12,0x11,0x7f,0x10,0x10, // 20 = 4
  0x4f,0x49,0x49,0x49,0x49,0x49,0x31, // 21 = 5
  0x3e,0x49,0x49,0x49,0x49,0x49,0x30, // 22 = 6
  0x01,0x41,0x21,0x11,0x09,0x05,0x03, // 23 = 7
  0x36,0x49,0x49,0x49,0x49,0x49,0x36, // 24 = 8
  0x06,0x49,0x49,0x49,0x49,0x49,0x3e, // 25 = 9
  0x00,0x00,0x00,0x22,0x00,0x00,0x00, // 26 = :
  0x00,0x00,0x00,0x62,0x00,0x00,0x00, // 27 = ;
  0x00,0x08,0x14,0x22,0x41,0x00,0x00, // 28 = <
  0x00,0x14,0x14,0x14,0x14,0x14,0x14, // 29 = =
  0x00,0x00,0x41,0x22,0x14,0x08,0x00, // 30 = >
  0x03,0x01,0x01,0x59,0x09,0x09,0x0F, // 31 = ?
  0x1C,0x22,0x49,0x55,0x55,0x55,0x5F, // 32 = @
  0x7c,0x12,0x11,0x11,0x11,0x12,0x7c, // 33 = A
  0x7f,0x49,0x49,0x49,0x49,0x49,0x36, // 34 = B
  0x1c,0x22,0x41,0x41,0x41,0x41,0x41, // 35 = C
  0x7f,0x41,0x41,0x41,0x41,0x22,0x1c, // 36 = D
  0x7f,0x49,0x49,0x49,0x41,0x41,0x41, // 37 = E
  0x7f,0x09,0x09,0x09,0x01,0x01,0x01, // 38 = F
  0x1c,0x22,0x41,0x41,0x41,0x41,0x79, // 39 = G
  0x7f,0x08,0x08,0x08,0x08,0x08,0x7f, // 40 = H
  0x41,0x41,0x41,0x7f,0x41,0x41,0x41, // 41 = I
  0x10,0x20,0x40,0x41,0x41,0x21,0x1f, // 42 = J
  0x7f,0x08,0x08,0x08,0x14,0x22,0x41, // 43 = K
  0x7f,0x40,0x40,0x40,0x40,0x40,0x40, // 44 = L
  0x7f,0x02,0x04,0x08,0x04,0x02,0x7f, // 45 = M
  0x7f,0x02,0x04,0x08,0x10,0x20,0x7f, // 46 = N
  0x1c,0x22,0x41,0x41,0x41,0x22,0x1c, // 47 = O
  0x7f,0x09,0x09,0x09,0x09,0x09,0x06, // 48 = P
  0x1c,0x22,0x41,0x41,0x51,0x22,0x5c, // 49 = Q
  0x7f,0x09,0x09,0x09,0x19,0x29,0x46, // 50 = R
  0x46,0x49,0x49,0x49,0x49,0x49,0x31, // 51 = S
  0x01,0x01,0x01,0x7f,0x01,0x01,0x01, // 52 = T
  0x1f,0x20,0x40,0x40,0x40,0x20,0x1f, // 53 = U
  0x0f,0x10,0x20,0x40,0x20,0x10,0x0f, // 54 = V
  0x7f,0x20,0x10,0x08,0x10,0x20,0x7f, // 55 = W
  0x41,0x22,0x14,0x08,0x14,0x22,0x41, // 56 = X
  0x01,0x02,0x04,0x78,0x04,0x02,0x01, // 57 = Y
  0x41,0x61,0x51,0x49,0x45,0x43,0x41, // 58 = Z
  0x00,0x00,0x7f,0x41,0x41,0x00,0x00, // 59 = [
  0x01,0x02,0x04,0x08,0x10,0x20,0x40, // 60 = backslash
  0x00,0x00,0x41,0x41,0x7f,0x00,0x00, // 61 = ]
  0x08,0x0c,0x06,0x03,0x06,0x0c,0x08, // 62 = ^
  0x80,0x80,0x80,0x80,0x80,0x80,0x80, // 63 = _
  0x00,0x00,0x03,0x07,0x04,0x00,0x00, // 64 = '
  0x00,0x64,0x54,0x54,0x54,0x78,0x00, // 65 = a
  0x00,0x7f,0x44,0x44,0x44,0x38,0x00, // 66 = b
  0x00,0x38,0x44,0x44,0x44,0x44,0x00, // 67 = c
  0x00,0x30,0x48,0x48,0x48,0x7f,0x00, // 68 = d
  0x00,0x38,0x54,0x54,0x54,0x18,0x00, // 69 = e
  0x00,0x08,0x7E,0x09,0x01,0x02,0x00, // 70 = f
  0x00,0x18,0xA4,0xA4,0xA4,0x78,0x00, // 71 = g
  0x00,0x7F,0x04,0x04,0x04,0x78,0x00, // 72 = h
  0x00,0x00,0x44,0x7D,0x44,0x00,0x00, // 73 = i
  0x00,0x20,0x40,0x40,0x44,0x3D,0x00, // 74 = j
  0x00,0x7F,0x10,0x10,0x28,0x44,0x00, // 75 = k
  0x02,0x02,0x3E,0x40,0x40,0x40,0x00, // 76 = l
  0x00,0x7C,0x08,0x10,0x08,0x7C,0x00, // 77 = m
  0x00,0x7C,0x08,0x04,0x04,0x78,0x00, // 78 = n
  0x00,0x38,0x44,0x44,0x44,0x38,0x00, // 79 = o
  0x00,0xFC,0x24,0x24,0x24,0x18,0x00, // 80 = p
  0x00,0x18,0x24,0x24,0x24,0xF8,0x00, // 81 = q
  0x00,0x7C,0x08,0x04,0x04,0x08,0x00, // 82 = r
  0x00,0x58,0x54,0x54,0x54,0x34,0x00, // 83 = s
  0x00,0x04,0x3E,0x44,0x44,0x20,0x00, // 84 = t
  0x00,0x3C,0x40,0x40,0x40,0x7C,0x00, // 85 = u
  0x00,0x1C,0x20,0x40,0x20,0x1C,0x00, // 86 = v
  0x00,0x3C,0x40,0x30,0x40,0x3C,0x00, // 87 = w
  0x00,0x44,0x28,0x10,0x28,0x44,0x00, // 88 = x
  0x00,0x0C,0x90,0x90,0x90,0x7C,0x00, // 89 = y
  0x00,0x44,0x64,0x54,0x4C,0x44,0x00, // 90 = z
  0x00,0x08,0x08,0x36,0x41,0x41,0x00, // 91 = {
  0x00,0x00,0x00,0xFF,0x00,0x00,0x00, // 92 = |
  0x00,0x41,0x41,0x36,0x08,0x08,0x00, // 93 = }
  0x02,0x03,0x01,0x03,0x02,0x03,0x01, // 94 = ~
  0x70,0x78,0x4c,0x46,0x4c,0x78,0x70, // 95 = DEL : ASCII = 127

  0xFF,0x01,0x01,0x01,0x01,0x01,0xFF, // 96 = SQUARE 1
  0x80,0x80,0x80,0x80,0x80,0x80,0xFF, // 97 = SQUARE 2
  0x40,0x20,0x10,0x08,0x04,0x02,0xFF, // 98 = RAMP
  0xFF,0x02,0x04,0x08,0x10,0x20,0x40, // 99 = SAW

  0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 100 = PROGRESSh 0 0x84
  0x7E,0x00,0x00,0x00,0x00,0x00,0x00, // 101 = PROGRESSh 1 0x85
  0x7E,0x00,0x7E,0x00,0x00,0x00,0x00, // 102 = PROGRESSh 2 0x86
  0x7E,0x00,0x7E,0x00,0x7E,0x00,0x00, // 103 = PROGRESSh 3 0x87
  0x7E,0x00,0x7E,0x00,0x7E,0x00,0x7E, // 104 = PROGRESSh 4 0x88

  0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 105 = PROGRESSv 0 0x89
  0x80,0x80,0x80,0x80,0x80,0x80,0x80, // 106 = PROGRESSv 1 0x8A
  0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0, // 107 = PROGRESSv 2 0x8B
  0xA8,0xA8,0xA8,0xA8,0xA8,0xA8,0xA8, // 108 = PROGRESSv 3 0x8C
  0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA, // 109 = PROGRESSv 4 0x8D


  0x10,0x10,0x10,0x10,0x10,0x10,0x10, // 105 H Line    0x8E
  0x10,0x10,0x10,0x1F,0x10,0x10,0x10, // 106 Inv T     0x8F
  0x10,0x10,0x10,0xFF,0x10,0x10,0x10, // 107 Cross     0x90
  0x00,0x3E,0x3E,0x3E,0x3E,0x3E,0x00, // 108 QuadMark  0x91

  0x7F,0x41,0xDD,0x1C,0xDD,0x41,0x7F, // 109 SymbolF   0x92
  0x80,0x80,0xFF,0x00,0xFF,0x80,0x80, // 110 SymbolT   0x93
  0x01,0x01,0xFF,0x00,0xFF,0x01,0x01, // 111 SymbolT   0x94

  0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00

  };
