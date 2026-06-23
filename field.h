#pragma  once
#include <stdbool.h>
#include <stddef.h>
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

/** Primitives ***************************************************************************************************************/

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

} Palette;

typedef enum {
    GRIP    = 3,
    GAP     = 2,
    SOCKET_BORDER = 2,

} Constrain;



/** Primitives ***************************************************************************************************************/

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

} Frame;

typedef struct
{
    Frame*   data;
    uint32_t width;
    uint32_t height;  
    uint32_t nframes;           

} Sprite;

/*****************************************************************************************************************************/

typedef struct Field Field;
typedef struct Entity Entity;
typedef struct Checkbox Checkbox;
typedef struct Slider Slider;
typedef struct Momentary Momentary;
typedef struct rotary rotary;
typedef struct Socket Socket;
typedef struct Carrier Carrier;
typedef struct SectorDescriptor SectorDescriptor;

struct SectorDescriptor {
    uint32_t    id;
    uint32_t    node_id;
    ltwh32u     bounds;
    SectorType  type;
    SubType     subtype;
    float       default_value;
    float       range[2];
    float       step[CP_LIMIT];
    uint32_t    radio_id;
    char*       label;
    uint32_t    flags;
    uint32_t    output;
};

struct Entity {
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
    void        (*callback[CT_LIMIT])(Entity*, Entity*);
    Entity*     target[CT_LIMIT];
    Entity*     connection;
    Field*      parent;
    Entity*     root;
    Entity**    node;
};

struct Checkbox {
    uint32_t radio_id;  
};

struct Slider {
    SubType type;
    float default_value;
    float step[CP_LIMIT];
};

struct rotary {
    float default_value;
    float step[CP_LIMIT];
};

struct Socket {

};


struct Momentary {

};

/*****************************************************************************************************************************/

struct Carrier {
    uint32_t uid;
    uint32_t width;
    uint32_t height;
    uint32_t entities;
    uint32_t capacity;
    Entity*  at; 
};



struct Field {
    ltrb32u     bounds;
    uint32_t    width;
    uint32_t    height;
    point32s    memory[SP_LIMIT];   // Saved cursor position
    Frame*      layer[CC];
    Carrier*    node;
    Entity*     at;                 // Controls array TODO: Move to Carrier
    Entity*     pressed;
    uint32_t    current;            // HitTest return
    uint32_t    prior;              // Last HitTest
    bool        repaint;            // Repaint flag
    bool        drag;               // Sector drag flag
    bool        move;               // Window drag flag
    bool        staging;            // Draw staging layer
    bool        connecting;         // Connection pending
    uint32_t    entities;           // TODO: will be renamed to nodes and mean Carriers count
    uint32_t    nodes;
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


extern const uint8_t gtFont[];

static void draw_glyph      (Frame*, const uint8_t*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_text_label (Frame*, const uint8_t* , const char*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_rect_o     (Frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_rect_f     (Frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_line_v     (Frame*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_line_h     (Frame*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void draw_circle_f   (Frame*, uint32_t, uint32_t, uint32_t, uint32_t);
static void draw_circle_o   (Frame*, uint32_t, uint32_t, uint32_t, uint32_t);
static void draw_ltrb_o     (Frame*, ltrb32u*, const uint32_t);
static void draw_ltrb_f     (Frame*, ltrb32u*, const uint32_t);
static uint32_t frame_get   (Frame*, uint32_t, uint32_t);
static void frame_set       (Frame*, uint32_t, uint32_t, uint32_t);
static void frame_fill      (Frame*, uint32_t);
static void frame_clr       (Frame*);
static void frame_init      (Frame*, uint32_t, uint32_t);
static void frame_flush     (Frame*);
static void frame_copy      (Frame*, Frame*);
static void frame_copy_at   (Frame*, Frame*, uint32_t, uint32_t);
static void sprite_init     (Sprite*, uint32_t, uint32_t, uint32_t);
static void sprite_flush    (Sprite*);
static void sprite_load_stripe(Sprite*, Frame*);

static inline void frame_set(Frame*o, uint32_t x, uint32_t y, uint32_t value)
{
    if(x < o->width && y < o->height)
    o->data[x + y * o->width] = value;
}

static inline uint32_t frame_get(Frame* o, uint32_t x, uint32_t y)
{
    if(x < o->width && y < o->height) return o->data[x + y * o->width];
    return o->data[0];
}

static inline void frame_clr(Frame* o)
{
    memset(o->data, 0, o->height * o->width * sizeof(uint32_t));
}

static inline void frame_fill(Frame* o, uint32_t value)
{
    for(uint32_t i = 0; i < (o->height * o->width); ++i) o->data[i] = value;
}

static inline void frame_init(Frame* o, uint32_t x, uint32_t y)
{
    o->width  = x;
    o->height = y;
    o->data   = (uint32_t*)calloc(o->width * o->height , sizeof(uint32_t));
}

static inline void frame_flush(Frame* o)
{
    free(o->data);
}

static inline void frame_copy(Frame* target, Frame* source) {
    for(uint32_t i = 0; i < target->height * target->width; i++) target->data[i] = source->data[i];
}

static inline void frame_copy_at(Frame* target, Frame* source, uint32_t xo, uint32_t yo) {
    for(uint32_t y = 0; y < source->height; y++) {
        for(uint32_t x = 0; x < source->width; x++) {
            frame_set(target, x + xo, y + yo, frame_get(source, x, y));
        }
    }
}

static inline void draw_rect_o(Frame* restrict frame, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t i = l; i <= r; i++) {
        frame_set(frame, i, t, colour);
        frame_set(frame, i, b, colour);
    }

    for(uint32_t i = t; i <= b; i++) {
        frame_set(frame, l, i, colour);
        frame_set(frame, r, i, colour);
    }
}

static inline void draw_rect_f(Frame* frame, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t y = t; y <= b; ++y) {
        for(uint32_t x = l; x <= r; ++x) {
            frame_set(frame, x, y, colour);
        }
    }
}

static inline void draw_line_v(Frame* frame, uint32_t x, uint32_t yo, uint32_t ye, const uint32_t value) {
    for(uint32_t y = yo; y <= ye; ++y)
        frame_set(frame, x, y, value);
}

static inline void draw_line_h(Frame* frame, uint32_t y, uint32_t xo, uint32_t xe, const uint32_t value) {
    for(uint32_t x = xo; x <= xe; ++x)
        frame_set(frame, x, y, value);
}

void draw_circle_f(Frame* frame, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
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

        draw_line_h(frame, yc + x, xc - y, xc + y, value);
        draw_line_h(frame, yc - x, xc - y, xc + y, value);
        draw_line_h(frame, yc + y, xc - x, xc + x, value);
        draw_line_h(frame, yc - y, xc - x, xc + x, value);
    }

    draw_line_h(frame, yc, xc - r, xc + r, value);
}

static inline void draw_circle_o(Frame* frame, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
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

        frame_set(frame, xc + x, yc + y, value);
        frame_set(frame, xc - x, yc + y, value);
        frame_set(frame, xc + x, yc - y, value);
        frame_set(frame, xc - x, yc - y, value);
        frame_set(frame, xc + y, yc + x, value);
        frame_set(frame, xc - y, yc + x, value);
        frame_set(frame, xc + y, yc - x, value);
        frame_set(frame, xc - y, yc - x, value); 
    }

    frame_set(frame, xc, yc - r, value);
    frame_set(frame, xc, yc + r, value);
    frame_set(frame, xc + r, yc, value);
    frame_set(frame, xc - r, yc, value);
}

static inline void draw_glyph(Frame* frame, const uint8_t* font, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour) {
    uint32_t pos = id * 7;
    u_int8_t stencil = 0b1;

    for(uint32_t y = 0; y < 8; y++) {
        for(uint32_t x = 0; x < 7; x++) {
            if(font[pos + x] & stencil) frame_set(frame, x + l, y + t, colour);
        }
        stencil<<=1;
    }
}

static inline void drawTextLabel(Frame* frame, const uint8_t* font, const char* text, uint32_t l, uint32_t t, uint32_t, uint32_t, const uint32_t colour) {
    uint32_t n = strlen(text);
    for(uint32_t i = 0; i < n; i++) {
        draw_glyph(frame, font, text[i] - 32, l + i * 8, t, colour);
    }
}

/******************************************************************************************************************************/

static inline void sprite_init(Sprite* sprite, uint32_t w, uint32_t h, uint32_t nframes) {
    Frame temp;
    frame_init(&temp, w, h);

    sprite->data = (Frame*)malloc(nframes * sizeof(temp));
    sprite->width = w;
    sprite->height = h;
    sprite->nframes = nframes;

    frame_flush(&temp);
    
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        frame_init(&sprite->data[i], w, h);
    }
}

static inline void sprite_flush(Sprite* restrict sprite) {
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        frame_flush(&sprite->data[i]);
    }
    free(sprite->data);
}

static inline void sprite_load_stripe(Sprite* restrict sprite, Frame* restrict frame) {
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        for(uint32_t y = 0; y < sprite->height; y++) {
            for(uint32_t x = 0; x < sprite->width; x++) {
                frame_set(&sprite->data[i], x, y, frame_get(frame, x, y + (i * sprite->height)));
            }
        }
    }
}

static inline void draw_ltrb_o(Frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour) {
    for(uint32_t i = r->l; i <= r->r; ++i) {
        frame_set(canvas, i, r->t, colour);
        frame_set(canvas, i, r->b, colour);
    }
    for(uint32_t i = r->t; i <= r->b; i++) {
        frame_set(canvas, r->l, i, colour);
        frame_set(canvas, r->r, i, colour);
    }
}

static inline void draw_ltrb_f(Frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour)
{
    for(uint32_t y = r->t; y <= r->b; ++y) {
        for(uint32_t x = r->l; x <= r->r; ++x) {
            frame_set(canvas, x, y, colour);
        }
    }
}

/******************************************************************************************************************************/
void set_text_data(Entity*, const char*); 
static void value_to_textbox(Entity*, Entity*);

void fuse_link(Entity*, Entity*);
void draw_scene(Field* restrict);

void add_mod_link(Entity*, Entity*, CallbackType, void (*)(Entity*, Entity*));
void link_sector(Entity*, Entity*);
void move_sector(Entity* restrict, ltrb32u* restrict);
void erase_sector(Entity* restrict);

/*****************************************************************************************************************************/

Entity* createSector(Field* restrict, SectorDescriptor*);
void initField   (Field* restrict, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void destroyField(Field* restrict);

/*****************************************************************************************************************************/

void hit_test_down(Field* restrict, int, int, uint32_t);
void hit_test_drag(Field* restrict, int, int);
void hit_test_up  (Field* restrict, int, int, uint32_t);
void hit_test     (Field* restrict, int, int);

/*****************************************************************************************************************************/

extern void (*init_sector[])(Entity* restrict);
extern void (*set_sector[])(Entity* restrict, int, int);
extern void (*draw_sector[])(Entity* restrict);
extern void (*drag_sector[])(Entity* restrict, int, int);
extern void (*scroll_sector[])(Entity* restrict, int, int);
extern void (*enter_sector[])(Entity* restrict, int, int);
extern void (*leave_sector[])(Entity* restrict, int, int);
extern void (*release_sector[])(Entity* restrict, int, int);

/*****************************************************************************************************************************/

Entity* find_sector_by_id(Field* restrict o, uint32_t id) {
    if(!id) return nullptr;

    for(uint32_t i = 0; i <= o->entities; ++i) {
        if(o->at[i].uid == id) return &o->at[i];
    }
    return nullptr;
}

Entity* createSector(Field* restrict o, SectorDescriptor* d) {
    auto pos = ++o->entities;
    auto s = &o->at[pos];

    s->uid                      = d->id;
    s->type                     = d->type;
    s->bounds.l                 = d->bounds.l;
    s->bounds.t                 = d->bounds.t;
    s->bounds.r                 = d->bounds.l + d->bounds.w;
    s->bounds.b                 = d->bounds.t + d->bounds.h;
    s->width                    = d->bounds.w;
    s->height                   = d->bounds.h;
    s->value[CP_COARSE]         = (float)(int)d->default_value;
    s->value[CP_FINE]           = d->default_value - (float)(int)d->default_value;
    s->range[0]                 = d->range[0];
    s->range[1]                 = d->range[1];

    s->repaint                  = true;
    s->flags                    = d->flags;
    s->parent                   = o;

    printf("Created sector : %u\n", s->index);

    switch (d->type) {
        case ST_CHECKBOX: {
            s->extension = (Checkbox*)malloc(sizeof(Checkbox));
            auto ext = (Checkbox*)s->extension;
            ext->radio_id = d->radio_id;
        }
        break;

        case ST_SLIDER: {
            s->extension = (Slider*)malloc(sizeof(Slider));
            auto ext = (Slider*)s->extension;
            ext->type = d->subtype;
            ext->default_value = d->default_value; 
            ext->step[CP_COARSE] = d->step[CP_COARSE];
            ext->step[CP_FINE] = d->step[CP_FINE];
        }
        break;

        case ST_SOCKET: {
            s->extension = (Socket*)malloc(sizeof(Socket));
        }
        break;

        default: 
        break;
    
    }

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
        s->callback[i] = &fuse_link;
    }

    if(!(d->flags & TRANSPARENT)) {
        draw_ltrb_f (
            o->layer[SC],
            &s->bounds,
            s->index
        );
    }

    init_sector[d->type](s);
    if(d->type == ST_TEXTBOX) {
        strncpy(s->data, d->label, TEXTBOX_SIZE);
    }

    if(d->output) {
        printf("---- Set output : %d\n", d->output);
        auto target = find_sector_by_id(o, d->output);
        if(target) {
            add_mod_link(s, target, CT_VALUE, value_to_textbox);
            printf("---- Target found...\n");
        }
        else
            printf("---- NO Target found...\n");
    }

    auto node = find_sector_by_id(o, d->node_id);
    if(node)
        link_sector(node, s);

    return s;
}

void initField(Field* restrict field, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t size, uint32_t flags) {
    field->bounds.l     = x;
    field->bounds.t     = y;
    field->bounds.r     = x + w;
    field->bounds.b     = y + h;
    field->width        = w;
    field->height       = h;

    for(int i = 0; i < SP_LIMIT; ++i) {
        field->memory[i] = (point32s) { .x = 0, .y = 0 };
    }

    field->drag         = false;
    field->move         = false;
    field->capacity     = size + 1u;
    field->repaint      = true;
    field->entities      = 0;
    field->flags        = flags;
    field->step         = 25;
    field->staging      = false;
    field->connecting   = false;
    field->prior        = 0;
    field->current      = 0;
    field->pressed      = nullptr;

    if(true) {
        for(uint32_t i = 0; i < CC; ++i) {
            field->layer[i] = malloc(sizeof(Frame));
            frame_init(field->layer[i],  w, h);
        }

        frame_clr(field->layer[SC]);
        frame_clr(field->layer[SN]);
        frame_fill(field->layer[BG], BACKGROUND);
    }

    field->at = malloc(field->capacity * sizeof(Entity));
    for(uint32_t i = 0; i < field->capacity; i++) field->at[i].index = i;

    auto canvas = &field->at[0];

    canvas->type = ST_CANVAS;
    canvas->parent = field;
    canvas->repaint = true;

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
        canvas->callback[i] = &fuse_link;
    }
}

void destroyField(Field* restrict o) {
    for(uint32_t i = 0; i < CC; ++i) {
        frame_flush(o->layer[i]);
        free(o->layer[i]);
    }
    free(o->at);
}

static inline Carrier* createCarrier(Field* restrict, uint32_t, uint32_t, size_t);

static inline Carrier* createCarrier(Field* restrict field, uint32_t w, uint32_t h, size_t size) {

    auto index = field->nodes;

    if (index >= field->capacity) {
        field->capacity = field->capacity * 2 + 4;
        field->node = realloc(field->node, field->capacity * sizeof(Carrier));
    }

    field->node[index].width = w;
    field->node[index].height = h;
    field->node[index].capacity = size;
    
    field->node[index].at = malloc(size * sizeof(Entity));
    for(size_t i = 0; i < size; ++i) field->node[index].at[i].index = i;
    
    ++field->nodes;
    return &field->node[index];
}

void empty() {}

void fuse_link(Entity*, Entity*) {}

void link_sector(Entity* parent, Entity* child) {
    assert(parent != child);
    if (parent->nodes >= parent->capacity) {
        parent->capacity = parent->capacity * 2 + 4;
        parent->node = realloc(parent->node, parent->capacity * sizeof(Entity*));
    }
    parent->node[parent->nodes] = child;
    child->root = parent;
    ++parent->nodes;
}

void move_sector(Entity* restrict s, ltrb32u* restrict bounds) {
    draw_ltrb_f(s->parent->layer[FG], &s->bounds, 0x0);
    if(!(s->flags & TRANSPARENT)) {
        draw_ltrb_f(s->parent->layer[SC], &s->bounds, 0x0);
        draw_ltrb_f(s->parent->layer[SC], bounds, s->index);
    }
    s->bounds = *bounds;
}

void add_mod_link(Entity* source, Entity* target, CallbackType type, void (*fn)(Entity*, Entity*)) {
    source->callback[type] = fn;
    source->target[type] = target;
}

void set_text_data(Entity* restrict o, const char* restrict text) {
    strncpy(o->data, text, TEXTBOX_SIZE);
    o->repaint = true;
}

static inline void value_to_textbox(Entity* slider, Entity* tbox) {
    int precision = 2;
    char *text = tbox->data;
    float val = slider->value[CP_COARSE] + slider->value[CP_FINE];
    snprintf(text, TEXTBOX_SIZE, "%.*f", precision, val);
    tbox->repaint = true;
}

/*****************************************************************************************************************************/

void hit_test_down(Field* restrict o, int x, int y, uint32_t button) {
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
            p->parent->layer[SN],
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
static inline void draw_debug_overlay(Field* o, Entity* s) {
    constexpr uint32_t colour = 0xFF'FF'FF'90;
    frame_fill(o->layer[DO], 0);
    uint32_t voffset = 10;
    uint32_t vstep = 12;
    uint32_t row = 0;

    drawTextLabel(o->layer[DO], gtFont,    "#", 10, voffset + vstep * row++, 100, 10, colour);

    if(!s) return;

    char buffer[128];
    snprintf(buffer, 128, "CONTROL INDEX: %u", s->index);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CONTROL UID  : %u", s->uid);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "IS CONNECTED : %d", s->connected);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CONNECTION   : %p", (void*)s->connection);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "HAS DATA     : %d", s->has_data);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "VALUE COARSE : %f", s->value[0]);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "VALUE FINE   : %f", s->value[1]);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "FLAGS        : %b", s->flags);
    drawTextLabel(o->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    atomic_store_explicit(&force_repaint, true, memory_order_release);
}
#endif

void hit_test(Field* restrict o, int x, int y) {
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

void hit_test_drag(Field* restrict o, int x, int y) {
    drag_sector[o->pressed->type](o->pressed, x, y);
    o->repaint = true;
}

void hit_test_up(Field* restrict o, int x, int y, uint32_t button) {
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

static void draw_checkbox(Entity* restrict c) {
    auto o = c->parent;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);
    if(c->value[CP_COARSE] > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, SELECTIONBACKGROUND);
    c->repaint = false;
}

void draw_scene(Field* restrict o) {
    for(uint32_t i = 0; i <= o->entities; ++i) {
        auto s = &o->at[i];
        if(s->repaint) {
            draw_sector[s->type](s);
        }
    }
    o->repaint = false;
}

/*****************************************************************************************************************************/

static void set_checkbox(Entity* restrict o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    auto ext = (Checkbox*)o->extension;
    if(o->flags & RADIO) {
        auto id = ext->radio_id;
        if(*coarse > 0.5f) return;
        else {
            *coarse = 1.0f;
            o->repaint = true;
            auto f = o->parent;
            for(uint32_t i = 0; i <= f->entities; ++i) {
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


static void set_slider(Entity* restrict o, int x, int y) {
    const float v = 0.1f;
    const auto coarse = &o->value[CP_COARSE];
    const auto fine = &o->value[CP_FINE];
    auto ext = (Slider*)o->extension;

    float value = {};

    if(o->flags & VERTICAL) {
        const int dy = o->parent->memory[SP_CURSOR_PRIOR].y - y;
        const auto step = ext->step[CP_COARSE]; 
        value = step * roundf((float)dy * v) + o->memory[CP_COARSE];
    }
    else {
        const int dx = o->parent->memory[SP_CURSOR_PRIOR].x - x;
        const auto step = ext->step[CP_COARSE]; 
        value = - step * roundf((float)dx * v) + o->memory[CP_COARSE];
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

static void draw_slider(Entity* restrict c) {
    auto o = c->parent;
    auto ext = (Slider*)c->extension;
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

static void scroll_slider(Entity* restrict o, int, int y) {
    auto fine = &o->value[CP_FINE];
    auto coarse = &o->value[CP_COARSE];
    auto ext = (Slider*)o->extension;
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

static void draw_button(Entity* restrict c) {
    auto o = c->parent;
    draw_ltrb_f(o->layer[FG], &c->bounds, BUTTONS);
    draw_ltrb_o(o->layer[FG], &c->bounds, BORDER);

    if(c->hovered)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACTIVE);

    if(c->value[CP_COARSE] > 0.5f)
        draw_rect_f(o->layer[FG], c->bounds.l + GAP, c->bounds.t + GAP, c->bounds.r - GAP, c->bounds.b - GAP, ACCENT);

    c->repaint = false;
}

/******************************************************************************************************************************/

static void draw_rotary(Entity* restrict c) {
    auto o = c->parent;
    auto range = c->range[1] - c->range[0];
    Sprite* spr = (Sprite*)c->data;
    int f = ((c->value[CP_COARSE] - c->range[0]) / range) * spr->nframes;
    frame_copy_at(o->layer[FG], &spr->data[f], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void set_rotary(Entity* restrict o, int x, int y) {
    auto f = o->parent;
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

static void set_sprite_inf_slider(Entity* restrict o, int x, int y) {
    auto f = o->parent;
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

static void set_button(Entity* restrict o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    o->repaint = true;

    auto f = o->parent;
    f->prior = f->current;
}

static void release_button(Entity* restrict o, int, int) {
    auto p = o->parent->pressed; 
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

static void draw_sprite_button(Entity* restrict c) {
    auto o = c->parent;
    Sprite* spr = (Sprite*)c->data;

    if(c->value[CP_COARSE] > 0.5f)
        frame_copy_at(o->layer[FG], &spr->data[1], c->bounds.l, c->bounds.t);
    else
        frame_copy_at(o->layer[FG], &spr->data[0], c->bounds.l, c->bounds.t);

    c->repaint = false;
}

static void init_socket(Entity* restrict s) {
    s->data = (float*)calloc(SPLINE_SEGMENTS * 2, sizeof(float));
}

static void set_socket(Entity* restrict o, int, int) {
    auto coarse = &o->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    o->repaint = true;

    auto f = o->parent;
    f->prior = f->current;
}

static void draw_socket(Entity* restrict c) {
    auto o = c->parent;
    auto r = c->width / 2;
    point32u center = {
        .x = c->bounds.l + r,
        .y = c->bounds.t + r
    };
    auto color = c->flags & OUTPUT ? RED : PURPLE;

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

static void drag_socket(Entity* s, int x, int y)
{
    if(s->connected) {
        s->parent->pressed = s->connection;
        s = s->connection;
        s->parent->current = s->index;

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

    auto o = s->parent;

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

static void drag_cord(Entity* restrict s, int x, int y)
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

    auto o = s->parent;

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

inline static void connect(Entity* restrict source, Entity* restrict target) {
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
    source->parent->connecting = false;
}

inline static void disconnect(Entity* restrict o) {
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

static void release_socket(Entity* restrict o, int x, int y) {
    auto parent = o->parent;
    auto target_id = frame_get(parent->layer[SC], x, y);
    auto target = &parent->at[target_id];
    
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

static void init_node(Entity* restrict s) {
    s->data = (Frame*)calloc(1, sizeof(Frame));
    frame_init((Frame*)s->data, s->width, s->height);
    draw_ltrb_f(s->parent->layer[SN], &s->bounds, s->index);
}

static void set_node(Entity* restrict o, int, int) {
    o->repaint = true;
    auto f = o->parent;
    f->prior = f->current;
}

static inline bool has_overlap(const Entity* restrict c) {
    const auto l = c->parent->layer[SN];
    for(uint32_t y = c->bounds.t; y < c->bounds.b; ++y) {
        for(uint32_t x = c->bounds.l; x < c->bounds.r; ++x) {
            auto v = frame_get(l, x, y);
            if(v && v != c->index)
                return true;
        }
    }
    return false;
}

static void draw_node(Entity* restrict c) {
    auto o = c->parent;
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

static void drag_node(Entity* restrict o, int x, int y)
{
    auto f = o->parent;
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

static void release_node(Entity* restrict s, int, int) 
{
    auto overlap = has_overlap(s);

    ltrb32u ir = {
        .l = s->parent->memory[SP_LT_PRESS].x, 
        .t = s->parent->memory[SP_LT_PRESS].y, 
        .r = s->parent->memory[SP_LT_PRESS].x + s->width, 
        .b = s->parent->memory[SP_LT_PRESS].y + s->height
    };

    if(overlap) { 
        s->bounds = ir;
        draw_ltrb_f(s->parent->layer[SC], &ir, s->index);
        draw_ltrb_f(s->parent->layer[SN], &ir, s->index);
        draw_ltrb_f(s->parent->layer[NG], &ir, 0x0);

        for(uint32_t i = 0; i < s->nodes; ++i) {
            s->node[i]->repaint = true;
        }
    }
    else {
        draw_ltrb_f(s->parent->layer[SC], &ir, 0x0);
        draw_ltrb_f(s->parent->layer[SN], &ir, 0x0);
        draw_ltrb_f(s->parent->layer[SC], &s->bounds, s->index);
        draw_ltrb_f(s->parent->layer[SN], &s->bounds, s->index);

        draw_ltrb_f(s->parent->layer[NG], &ir, 0x0);

        int dx = s->bounds.l - s->parent->memory[SP_LT_PRESS].x;
        int dy = s->bounds.t - s->parent->memory[SP_LT_PRESS].y;

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

    frame_clr(s->parent->layer[ST]);
    s->parent->staging = false;
    s->staging = false;
    s->repaint = true;
    s->parent->repaint = true;
}

/*****************************************************************************************************************************/

static void init_textbox(Entity* restrict s) {
    s->data = (char*)calloc(TEXTBOX_SIZE, sizeof(char)); 
}

static void set_textbox(Entity* restrict o, int, int) {
    o->repaint = true;
}

static void draw_textbox(Entity* restrict c) {
    auto o = c->parent;
    draw_ltrb_f(o->layer[FG], &c->bounds, SECONDBACKGROUND);
    char *text = c->data;
    //auto len = strlen(text);
    //auto offset_x = (c->width - 10 * len) / 2;
    auto offset_y = (c->height - 8) / 2;

    drawTextLabel(
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

static void draw_canvas(Entity* restrict s) 
{
    auto o = s->parent;
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

static inline void init_none(Entity* restrict) {}

void (*init_sector[])(Entity* restrict)  = {
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

static inline void set_none(Entity* restrict, int, int) {}

void (*set_sector[])(Entity* restrict, int, int) = {
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

void (*draw_sector[])(Entity* restrict) = {
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

static inline void drag_none(Entity* restrict, int, int) {}

void (*drag_sector[])(Entity* restrict, int, int) = {
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

static inline void scroll_none(Entity* restrict, int, int) {}

void (*scroll_sector[])(Entity* restrict, int, int) = {
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

static inline void enter_none(Entity* restrict, int, int) {}

void (*enter_sector[])(Entity* restrict, int, int) = {
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
static inline void leave_none(Entity* restrict, int, int) {}

void (*leave_sector[])(Entity* restrict, int, int) = {
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

static inline void release_none(Entity* restrict, int, int) {}

void (*release_sector[])(Entity* restrict, int, int) = {
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
