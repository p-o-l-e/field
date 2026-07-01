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

atomic_bool trigger_stream_update = false;
constexpr uint_fast32_t SPLINE_SEGMENTS = 32;
constexpr uint_fast32_t TEXTBOX_SIZE = 32;

typedef struct Field Field;
typedef struct Entity Entity;
typedef struct Checkbox Checkbox;
typedef struct Slider Slider;
typedef struct Encoder Encoder;
typedef struct Momentary Momentary;
typedef struct rotary rotary;
typedef struct Socket Socket;
typedef struct Oscillograph Oscillograph;
typedef struct Node Node;
typedef struct SectorDescriptor SectorDescriptor;

typedef enum {
    ST_SLIDER,
    ST_ENCODER,
    ST_ROTARY,
    ST_SPRITE_INF_SLIDER,
    ST_SOCKET,
    ST_CHECKBOX, 
    ST_MOMENTARY, 
    ST_SPRITE_CHECKBOX, 
    ST_SPRITE_BUTTON, 
    ST_CANVAS,
    ST_TEXTBOX,
    ST_CRT,
    ST_NODE,

    ST_LIMIT

} SectorType;

typedef enum {
    SS_A, 
    SS_B,
    SS_C,

    SS_LIMIT

} SubType;

typedef enum: uint8_t {
    F_CT_STATIC,
    F_CT_CONTROL,
    F_CT_INPUT, 
    F_CT_OUTPUT 
} CoreType;

typedef enum {
    CT_PRESS,
    CT_RELEASE,
    CT_VALUE,

    CT_LIMIT

} CallbackType;

typedef enum {
    IP_NODE,
    IP_ENTITY,

    IP_LIMIT

} IndexPosition;
            
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
    TRANSPARENT = 1 << 3,
    RADIO       = 1 << 4,

} SectorFlags;

typedef enum {
    ROOT        = 1 << 0,

} FieldFlags;

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
    SOCKET_IN           = 0x009688FF,
    SOCKET_OUT          = 0xF07178FF,

} Palette;

typedef enum {
    GRIP    = 3,
    GAP     = 2,
    SOCKET_BORDER = 2,

} Constrain;

/** Primitives ****************************************************************************************************************/

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

/******************************************************************************************************************************/

struct SectorDescriptor {
    uint32_t    id;
    uint32_t    node_id;
    ltwh32u     bounds;
    CoreType    core_type;
    SectorType  type;
    SubType     subtype;
    float       default_value;
    float       range[2];
    float       step[CP_LIMIT];
    uint32_t    radio_id;
    char*       label;
    uint32_t    flags;
    uint32_t    output;
    uint32_t    input_x;
    uint32_t    input_y;
};

struct Entity {
    uint32_t    index;
    uint32_t    uid;
    ltrb32u     bounds;
    uint32_t    width;
    uint32_t    height;
    CoreType    core_type;
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
    Node*       parent;
    Entity*     root;
};

struct Checkbox {
    uint32_t radio_id;  
};

struct Slider {
    SubType type;
    float default_value;
    float step[CP_LIMIT];
    bool inverse;
};

struct Encoder {
    SubType type;
    float default_value;
    float step[CP_LIMIT];
    bool inverse;
};

struct rotary {
    float default_value;
    float step[CP_LIMIT];
};

struct Socket {
    float* input;
    float* output;
};

struct Oscillograph {
    float** x;
    float** y;
    void* data;
    uint32_t r_head;
    uint32_t w_head;
};

struct Momentary {

};

/*****************************************************************************************************************************/

struct Node {
    uint32_t    index;
    uint32_t    uid;
    uint32_t    width;
    uint32_t    height;
    uint32_t    entities;           // Entity count in this node
    uint32_t    capacity;
    Entity*     at;
    Field*      parent;
    Frame       buffer;
    bool        visible;
};

struct Field {
    ltrb32u     bounds;
    uint32_t    width;
    uint32_t    height;
    point32s    memory[SP_LIMIT];   // Saved cursor position
    Frame*      layer[CC];
    Node*       node;
    Entity*     pressed;
    Entity*     stream[3];
    uint32_t    current[IP_LIMIT];  // HitTest return
    uint32_t    prior[IP_LIMIT];    // Last HitTest
    bool        repaint;            // Repaint flag
    bool        drag;               // Sector drag flag
    bool        move;               // Window drag flag
    bool        staging;            // Draw staging layer
    bool        connecting;         // Connection pending
    uint32_t    nodes;
    uint32_t    capacity;
    uint32_t    flags;
    uint32_t    step;
};

extern const uint8_t gtFont[];

/** Primitives ****************************************************************************************************************/
static void ff_draw_glyph      (Frame*, const uint8_t*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void ff_draw_text_label (Frame*, const uint8_t* , const char*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void ff_draw_rect_o     (Frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void ff_draw_rect_f     (Frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
static void ff_draw_line_v     (Frame*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void ff_draw_line_h     (Frame*, uint32_t, uint32_t, uint32_t, const uint32_t);
static void ff_draw_circle_f   (Frame*, uint32_t, uint32_t, uint32_t, uint32_t);
static void ff_draw_circle_o   (Frame*, uint32_t, uint32_t, uint32_t, uint32_t);
static void ff_draw_ltrb_o     (Frame*, ltrb32u*, const uint32_t);
static void ff_draw_ltrb_f     (Frame*, ltrb32u*, const uint32_t);
static uint32_t ff_frame_get   (Frame*, uint32_t, uint32_t);
static void ff_frame_set       (Frame*, uint32_t, uint32_t, uint32_t);
static void ff_frame_fill      (Frame*, uint32_t);
static void ff_frame_clr       (Frame*);
static void ff_frame_init      (Frame*, uint32_t, uint32_t);
static void ff_frame_flush     (Frame*);
static void ff_frame_copy      (Frame*, Frame*);
static void ff_frame_copy_with_alpha(Frame*, Frame*, uint8_t);
static void ff_frame_copy_at   (Frame*, Frame*, uint32_t, uint32_t);
static void ff_sprite_init     (Sprite*, uint32_t, uint32_t, uint32_t);
static void ff_sprite_flush    (Sprite*);
static void ff_sprite_load_stripe(Sprite*, Frame*);
/******************************************************************************************************************************/
void ff_set_text_data(Entity*, const char*); 
static void ff_value_to_textbox(Entity*, Entity*);

void ff_fuse_link(Entity*, Entity*);
void ff_draw_scene(Field* restrict);

void ff_add_mod_link(Entity*, Entity*, CallbackType, void (*)(Entity*, Entity*));
void ff_move_entity(Entity* restrict, ltrb32u* restrict);
void ff_erase_entity(Entity* restrict);
static void ff_disconnect(Entity* restrict);
/** Contructors ***************************************************************************************************************/

Entity* ffCreateEntity(Node* restrict, SectorDescriptor*);
static inline Node* ffCreateNode(Field* restrict, uint32_t, uint32_t, size_t);
Field* ffCreateField(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void ffDestroyField(Field* restrict);

/** Helpers *******************************************************************************************************************/

static inline void ffPlaceNode(Field* restrict, Node* restrict, uint32_t, uint32_t);

/*****************************************************************************************************************************/
Entity* ff_find_entity_by_id(Field* restrict field, uint32_t id) {
    if(!id) return nullptr;

    for(uint32_t n = 0; n < field->nodes; ++n) {
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            if(field->node[n].at[i].uid == id) return &field->node[n].at[i];
        }
    }
    return nullptr;
}

void ff_hit_test_down(Field* restrict, int, int, uint32_t);
void ff_hit_test_drag(Field* restrict, int, int);
void ff_hit_test_up  (Field* restrict, int, int, uint32_t);
void ff_hit_test     (Field* restrict, int, int);

/*****************************************************************************************************************************/

extern void (*init_entity[])(Entity* restrict);
extern void (*set_entity[])(Entity* restrict, int, int);
extern void (*draw_entity[])(Entity* restrict);
extern void (*drag_entity[])(Entity* restrict, int, int);
extern void (*scroll_entity[])(Entity* restrict, int, int);
extern void (*enter_entity[])(Entity* restrict, int, int);
extern void (*leave_entity[])(Entity* restrict, int, int);
extern void (*release_entity[])(Entity* restrict, int, int);

/** Helpers *******************************************************************************************************************/

uint8_t ff_extract_byte(uint32_t, uint_fast8_t);
point32u ff_uv_to_screen(float, float, uint32_t, uint32_t);
point ff_screen_to_uv(uint32_t, uint32_t, uint32_t, uint32_t);
static uint32_t ff_hsva_to_rgba(uint8_t, uint8_t, uint8_t, uint8_t);
uint32_t ff_index_to_hsv(uint32_t, uint8_t);

/******************************************************************************************************************************
 * Returns 8-bit value
 * Data : 0x FF FF FF FF
 * index:     3  2  1  0     MSB->LSB
 * *****************************************************************************************************************************/
uint8_t ff_extract_byte(uint32_t value, uint_fast8_t byte) {
    return (value >> (byte * 8)) & 0xFF;
}

point32u ff_uv_to_screen(float x, float y, uint32_t w, uint32_t h) {
    point32u s = {
        .x = (uint32_t)(x * (float)w),
        .y = (uint32_t)(y * (float)h)
    };
    return s;
}

point ff_screen_to_uv(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    point n = {
        .x =  (float)x / (float)w,
        .y =  1.0f - (float)y / (float)h
    };
    return n;
}

static uint32_t ff_hsva_to_rgba(uint8_t h, uint8_t s, uint8_t v, uint8_t a) {
    uint8_t region = h / 43;
    uint8_t remainder = (h - region * 43) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    uint8_t r, g, b;
    switch (region) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return (r << 24) | (g << 16) | (b << 8) | a;
}

uint32_t ff_index_to_hsv(uint32_t index, uint8_t alpha) {
    uint8_t hue = (index * 137) & 0xFF;
    uint8_t sat = 204;
    uint8_t val = 230;
    return ff_hsva_to_rgba(hue, sat, val, alpha);
}

static inline void ff_frame_set(Frame* frame, uint32_t x, uint32_t y, uint32_t value) {
    if(x < frame->width && y < frame->height)
    frame->data[x + y * frame->width] = value;
}

static inline uint32_t ff_frame_get(Frame* frame, uint32_t x, uint32_t y) {
    if(x < frame->width && y < frame->height) return frame->data[x + y * frame->width];
    return frame->data[0];
}

static inline void ff_frame_clr(Frame* frame) {
    memset(frame->data, 0, frame->height * frame->width * sizeof(uint32_t));
}

static inline void ff_frame_fill(Frame* frame, uint32_t value) {
    for(uint32_t i = 0; i < (frame->height * frame->width); ++i) frame->data[i] = value;
}

static inline void ff_frame_init(Frame* frame, uint32_t x, uint32_t y) {
    frame->width  = x;
    frame->height = y;
    frame->data   = (uint32_t*)calloc(frame->width * frame->height , sizeof(uint32_t));
}

static inline void ff_frame_flush(Frame* frame) {
    free(frame->data);
}

static inline void ff_frame_copy(Frame* target, Frame* source) {
    for(uint32_t i = 0; i < target->height * target->width; i++) target->data[i] = source->data[i];
}

static void ff_frame_copy_with_alpha(Frame* target, Frame* source, uint8_t alpha) {
    for(uint32_t i = 0; i < target->height * target->width; ++i) {
        auto data = ff_index_to_hsv(source->data[i] + 0x2, alpha);
        target->data[i] = data;
    }
}

static inline void ff_frame_copy_at(Frame* target, Frame* source, uint32_t xo, uint32_t yo) {
    for(uint32_t y = 0; y < source->height; y++) {
        for(uint32_t x = 0; x < source->width; x++) {
            auto data = ff_frame_get(source, x, y);
            if(data)
                ff_frame_set(target, x + xo, y + yo, data);
        }
    }
}

static inline void ff_draw_rect_o(Frame* restrict frame, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour) {
    for(uint32_t i = l; i <= r; i++) {
        ff_frame_set(frame, i, t, colour);
        ff_frame_set(frame, i, b, colour);
    }

    for(uint32_t i = t; i <= b; i++) {
        ff_frame_set(frame, l, i, colour);
        ff_frame_set(frame, r, i, colour);
    }
}

static inline void ff_draw_rect_f(Frame* frame, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour) {
    for(uint32_t y = t; y <= b; ++y) {
        for(uint32_t x = l; x <= r; ++x) {
            ff_frame_set(frame, x, y, colour);
        }
    }
}

static inline void ff_draw_line_v(Frame* frame, uint32_t x, uint32_t yo, uint32_t ye, const uint32_t value) {
    for(uint32_t y = yo; y <= ye; ++y)
        ff_frame_set(frame, x, y, value);
}

static inline void ff_draw_line_h(Frame* frame, uint32_t y, uint32_t xo, uint32_t xe, const uint32_t value) {
    for(uint32_t x = xo; x <= xe; ++x)
        ff_frame_set(frame, x, y, value);
}

void ff_draw_circle_f(Frame* frame, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
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

        ff_draw_line_h(frame, yc + x, xc - y, xc + y, value);
        ff_draw_line_h(frame, yc - x, xc - y, xc + y, value);
        ff_draw_line_h(frame, yc + y, xc - x, xc + x, value);
        ff_draw_line_h(frame, yc - y, xc - x, xc + x, value);
    }

    ff_draw_line_h(frame, yc, xc - r, xc + r, value);
}

static inline void ff_draw_circle_o(Frame* frame, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value) {
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

        ff_frame_set(frame, xc + x, yc + y, value);
        ff_frame_set(frame, xc - x, yc + y, value);
        ff_frame_set(frame, xc + x, yc - y, value);
        ff_frame_set(frame, xc - x, yc - y, value);
        ff_frame_set(frame, xc + y, yc + x, value);
        ff_frame_set(frame, xc - y, yc + x, value);
        ff_frame_set(frame, xc + y, yc - x, value);
        ff_frame_set(frame, xc - y, yc - x, value); 
    }

    ff_frame_set(frame, xc, yc - r, value);
    ff_frame_set(frame, xc, yc + r, value);
    ff_frame_set(frame, xc + r, yc, value);
    ff_frame_set(frame, xc - r, yc, value);
}

static inline void ff_draw_glyph(Frame* frame, const uint8_t* font, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour) {
    uint32_t pos = id * 7;
    u_int8_t stencil = 0b1;

    for(uint32_t y = 0; y < 8; y++) {
        for(uint32_t x = 0; x < 7; x++) {
            if(font[pos + x] & stencil) ff_frame_set(frame, x + l, y + t, colour);
        }
        stencil<<=1;
    }
}

static inline void ffDrawTextLabel(Frame* frame, const uint8_t* font, const char* text, uint32_t l, uint32_t t, uint32_t, uint32_t, const uint32_t colour) {
    uint32_t n = strlen(text);
    for(uint32_t i = 0; i < n; i++) {
        ff_draw_glyph(frame, font, text[i] - 32, l + i * 8, t, colour);
    }
}

/******************************************************************************************************************************/

static inline void ff_sprite_init(Sprite* sprite, uint32_t w, uint32_t h, uint32_t nframes) {
    Frame temp;
    ff_frame_init(&temp, w, h);

    sprite->data = (Frame*)malloc(nframes * sizeof(temp));
    sprite->width = w;
    sprite->height = h;
    sprite->nframes = nframes;

    ff_frame_flush(&temp);
    
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        ff_frame_init(&sprite->data[i], w, h);
    }
}

static inline void ff_sprite_flush(Sprite* restrict sprite) {
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        ff_frame_flush(&sprite->data[i]);
    }
    free(sprite->data);
}

static inline void ff_sprite_load_stripe(Sprite* restrict sprite, Frame* restrict frame) {
    for(uint32_t i = 0; i < sprite->nframes; i++) {
        for(uint32_t y = 0; y < sprite->height; y++) {
            for(uint32_t x = 0; x < sprite->width; x++) {
                ff_frame_set(&sprite->data[i], x, y, ff_frame_get(frame, x, y + (i * sprite->height)));
            }
        }
    }
}

static inline void ff_draw_ltrb_o(Frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour) {
    for(uint32_t i = r->l; i <= r->r; ++i) {
        ff_frame_set(canvas, i, r->t, colour);
        ff_frame_set(canvas, i, r->b, colour);
    }
    for(uint32_t i = r->t; i <= r->b; i++) {
        ff_frame_set(canvas, r->l, i, colour);
        ff_frame_set(canvas, r->r, i, colour);
    }
}

static inline void ff_draw_ltrb_f(Frame* restrict canvas, ltrb32u* restrict r, const uint32_t colour)
{
    for(uint32_t y = r->t; y <= r->b; ++y) {
        for(uint32_t x = r->l; x <= r->r; ++x) {
            ff_frame_set(canvas, x, y, colour);
        }
    }
}

Entity* ffCreateEntity(Node* restrict node, SectorDescriptor* descriptor) {
    if (node->entities >= node->capacity) {
        node->capacity = node->capacity * 2 + 4;
        node->at = realloc(node->at, node->capacity * sizeof(Entity));
    }
    auto pos = node->entities++;
    auto entity = &node->at[pos];

    entity->index                    = pos;
    entity->uid                      = descriptor->id;
    entity->type                     = descriptor->type;
    entity->core_type                = descriptor->core_type;
    entity->bounds.l                 = descriptor->bounds.l;
    entity->bounds.t                 = descriptor->bounds.t;
    entity->bounds.r                 = descriptor->bounds.l + descriptor->bounds.w;
    entity->bounds.b                 = descriptor->bounds.t + descriptor->bounds.h;
    entity->width                    = descriptor->bounds.w;
    entity->height                   = descriptor->bounds.h;
    entity->value[CP_COARSE]         = (float)(int)descriptor->default_value;
    entity->value[CP_FINE]           = descriptor->default_value - (float)(int)descriptor->default_value;
    entity->range[0]                 = descriptor->range[0];
    entity->range[1]                 = descriptor->range[1];
    entity->repaint                  = true;
    entity->flags                    = descriptor->flags;
    entity->visible                  = true;
    entity->parent                   = node;

    //printf("Created entity : %u\n", entity->index);

    switch (descriptor->type) {
        case ST_CHECKBOX: {
            entity->extension = (Checkbox*)malloc(sizeof(Checkbox));
            auto ext = (Checkbox*)entity->extension;
            ext->radio_id = descriptor->radio_id;
        }
        break;

        case ST_SLIDER: {
            entity->extension = (Slider*)malloc(sizeof(Slider));
            auto ext = (Slider*)entity->extension;
            ext->type = descriptor->subtype;
            ext->default_value = descriptor->default_value; 
            ext->step[CP_COARSE] = descriptor->step[CP_COARSE];
            ext->step[CP_FINE] = descriptor->step[CP_FINE];
            ext->inverse = false;
        }
        break;

        case ST_ENCODER: {
            entity->extension = (Encoder*)malloc(sizeof(Encoder));
            auto ext = (Encoder*)entity->extension;
            ext->type = descriptor->subtype;
            ext->default_value = descriptor->default_value; 
            ext->step[CP_COARSE] = descriptor->step[CP_COARSE];
            ext->step[CP_FINE] = descriptor->step[CP_FINE];
            ext->inverse = false;
        }
        break;

        case ST_SOCKET: {
            entity->extension = (Socket*)malloc(sizeof(Socket));
            auto ext = (Socket*)entity->extension;
            ext->input = nullptr;
            ext->output = nullptr;
        }
        break;

        case ST_CRT: {
            entity->extension = (Oscillograph*)malloc(sizeof(Oscillograph));
            auto ext = (Oscillograph*)entity->extension;
            auto xi = ff_find_entity_by_id(node->parent, descriptor->input_x);
            auto yi = ff_find_entity_by_id(node->parent, descriptor->input_y);
                
            ext->x = &((Socket*)xi->extension)->input;
            ext->y = &((Socket*)yi->extension)->input;
        }
        break;

        case ST_TEXTBOX: {

        }
        break;

        default: 
        break;
    
    }

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
        entity->callback[i] = &ff_fuse_link;
    }

    if(!(descriptor->flags & TRANSPARENT)) {
        ff_draw_ltrb_f (
            &node->buffer,
            &entity->bounds,
            entity->index
        );
    }

    init_entity[entity->type](entity);
    if(descriptor->type == ST_TEXTBOX) {
        strncpy(entity->data, descriptor->label, TEXTBOX_SIZE);
    }

    if(descriptor->output) {
        //printf("---- Set output : %d\n", descriptor->output);
        auto target = ff_find_entity_by_id(node->parent, descriptor->output);
        if(target) {
            ff_add_mod_link(entity, target, CT_VALUE, ff_value_to_textbox);
            //printf("---- Target found...\n");
        }
        // else
        //     printf("---- NO Target found...\n");
    }

    return entity;
}

Field* ffCreateField(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t size, uint32_t flags) {
    Field *field = malloc(sizeof(Field));

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
    field->flags        = flags;
    field->step         = 25;
    field->staging      = false;
    field->connecting   = false;
    field->prior[IP_NODE] = 0;
    field->prior[IP_ENTITY] = 0;
    field->current[IP_NODE] = 0;
    field->current[IP_ENTITY] = 0;
    field->pressed      = nullptr;

    if(true) {
        for(uint32_t i = 0; i < CC; ++i) {
            field->layer[i] = malloc(sizeof(Frame));
            ff_frame_init(field->layer[i],  w, h);
        }

        ff_frame_clr(field->layer[SC]);
        ff_frame_clr(field->layer[SN]);
        ff_frame_fill(field->layer[BG], BACKGROUND);
    }

    field->node = malloc(size * sizeof(Node));
    ff_frame_init(&field->node->buffer, w, h);
    field->nodes = 1;
    field->node[0].entities = 1;
    field->node[0].capacity = size;
    field->node[0].at = malloc(field->node[0].capacity * sizeof(Entity));
    field->node[0].parent = field;
    for(uint32_t i = 0; i < field->node[0].capacity; i++) field->node[0].at[i].index = i;
    field->node[0].index = 0;
    field->node[0].visible = true;

    auto canvas = &field->node[0].at[0];

    canvas->type = ST_CANVAS;
    canvas->parent = &field->node[0];
    canvas->repaint = true;

    for(uint32_t i = 0; i < CT_LIMIT; ++i) {
        canvas->callback[i] = &ff_fuse_link;
    }

    return field;
}

void ffDestroyField(Field* restrict field) {
    for(uint32_t i = 0; i < CC; ++i) {
        ff_frame_flush(field->layer[i]);
        free(field->layer[i]);
    }
    for(uint32_t n = 0; n < field->nodes; ++n) {
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            auto s = &field->node[n].at[i];
            if(s->extension) free(s->extension);
            if(s->data) {
                if(s->type == ST_NODE) {
                    ff_frame_flush((Frame*)s->data);
                }
                free(s->data);
            }
        }
        free(field->node[n].at);
    }
    free(field->node);
}


static inline Node* ffCreateNode(Field* restrict field, uint32_t w, uint32_t h, size_t size) {

    auto index = field->nodes;

    if (index >= field->capacity) {
        field->capacity = field->capacity * 2 + 4;
        field->node = realloc(field->node, field->capacity * sizeof(Node));
    }

    field->node[index].width = w;
    field->node[index].height = h;
    field->node[index].capacity = size;
    field->node[index].entities = 0;
    field->node[index].parent = field;
    field->node[index].index = index;
    field->node[index].visible = false;
    
    field->node[index].at = malloc(size * sizeof(Entity));
    ff_frame_init(&field->node[index].buffer,  w, h);
    
    ++field->nodes;
    return &field->node[index];
}

static inline void ffPlaceNode(Field* restrict field, Node* restrict node, uint32_t x, uint32_t y) {
    node->visible = true;
    ff_draw_ltrb_f(node->parent->layer[SN], &(ltrb32u) { .l = x, .t = y, x + node->width, y + node->height }, node->index);
    ff_frame_copy_at(node->parent->layer[SC], &node->buffer, x, y);

    for(uint32_t i = 0; i < node->entities; ++i) {
        auto bounds = &node->at[i].bounds;
        bounds->l += x;
        bounds->t += y;
        bounds->r += x;
        bounds->b += y;

        node->at[i].repaint = true;
    }
    field->repaint = true;
}

void ff_empty() {}

void ff_fuse_link(Entity*, Entity*) {}

void ff_move_entity(Entity* restrict entity, ltrb32u* restrict bounds) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, 0x0);
    if(!(entity->flags & TRANSPARENT)) {
        ff_draw_ltrb_f(field->layer[SC], &entity->bounds, 0x0);
        ff_draw_ltrb_f(field->layer[SC], bounds, entity->index);
    }
    entity->bounds = *bounds;
}

void ff_add_mod_link(Entity* source, Entity* target, CallbackType type, void (*fn)(Entity*, Entity*)) {
    source->callback[type] = fn;
    source->target[type] = target;
}

void ff_set_text_data(Entity* restrict entity, const char* restrict text) {
    strncpy(entity->data, text, TEXTBOX_SIZE);
    entity->repaint = true;
}

static inline void ff_value_to_textbox(Entity* slider, Entity* tbox) {
    int precision = 2;
    char *text = tbox->data;
    float val = slider->value[CP_COARSE] + slider->value[CP_FINE];
    snprintf(text, TEXTBOX_SIZE, "%.*f", precision, val);
    tbox->repaint = true;
}

void ff_hit_test_down(Field* restrict field, int x, int y, uint32_t button) {
    auto uide = ff_frame_get(field->layer[SC], x, y);
    auto uidn = ff_frame_get(field->layer[SN], x, y);
    field->current[IP_NODE] = uidn;
    field->current[IP_ENTITY] = uide;
    if(uidn >= field->nodes || uide >= field->node[uidn].entities) return;
    auto p = &field->node[uidn].at[uide];
    if(!p) return;

    field->pressed = p;
    field->memory[SP_LT_PRESS].x = p->bounds.l;
    field->memory[SP_LT_PRESS].y = p->bounds.t;

    field->memory[SP_CURSOR_PRESS].x = x;
    field->memory[SP_CURSOR_PRESS].y = y;

    field->memory[SP_CURSOR_PRIOR].x = x;
    field->memory[SP_CURSOR_PRIOR].y = y;

    p->memory[CP_COARSE] = p->value[CP_COARSE];
    p->memory[CP_FINE] = p->value[CP_FINE];

    if(p->flags & INTERCON) {
        if(button == RMB) {
            ff_disconnect(field->pressed);
        }
        else
            field->connecting = true;
    }

    if (p->flags & MOVEABLE) {
        if (button == LMB) field->drag = true;
        field->staging = true;
        p->staging = true;

        if(p->type == ST_NODE) {
            ff_draw_ltrb_f(
                p->parent->parent->layer[SN],
                &p->bounds, 
                0x0
            );
        }

    }
    else {
        set_entity[p->type](field->pressed, x, y);
    }
    field->repaint = true;
    field->prior[IP_NODE] = field->current[IP_NODE];
    field->prior[IP_ENTITY] = field->current[IP_ENTITY];
        
    p->callback[CT_PRESS](p, p->target[CT_PRESS]);
}

#ifdef DEBUG_OVERLAY
static inline void ff_draw_debug_overlay(Field* field, Entity* entity) {
    constexpr uint32_t colour = 0xFF'FF'FF'90;
    ff_frame_fill(field->layer[DO], 0);
    uint32_t voffset = 10;
    uint32_t vstep = 12;
    uint32_t row = 0;

    ffDrawTextLabel(field->layer[DO], gtFont,    "#", 10, voffset + vstep * row++, 100, 10, colour);

    if(!entity) return;

    // ff_frame_copy_with_alpha(field->layer[DO], field->layer[SC], 0x30);

    ff_draw_rect_o(field->layer[DO], 
            entity->bounds.l,
            entity->bounds.t, 
            entity->bounds.r,
            entity->bounds.b, 
            ERROR);

    char buffer[128];
    snprintf(buffer, 128, "CONTROL INDEX: %u", entity->index);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CORE TYPE    : %u", entity->core_type);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CONTROL UID  : %x", entity->uid);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "NODE INDEX   : %u", entity->parent->index);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "NODE UID     : %x", entity->parent->uid);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "IS CONNECTED : %d", entity->connected);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "CONNECTION   : %p", (void*)entity->connection);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "HAS DATA     : %d", entity->has_data);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "VALUE COARSE : %f", entity->value[0]);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "VALUE FINE   : %f", entity->value[1]);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    snprintf(buffer, 128, "FLAGS        : %b", entity->flags);
    ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
    if(entity->type == ST_SOCKET) {
        auto ext = (Socket*)entity->extension;
        if(ext->input) {
            snprintf(buffer, 128, "INPUT        : %f", *ext->input);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
        if(ext->output) {
            snprintf(buffer, 128, "OUTPUT       : %f", *ext->output);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
    }
    else if(entity->type == ST_CRT) {
        auto ext = (Oscillograph*)entity->extension;
        if(ext->x && *ext->x) {
            snprintf(buffer, 128, "X            : %f", **ext->x);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
        if(ext->y && *ext->y) {
            snprintf(buffer, 128, "Y            : %f", **ext->y);
            ffDrawTextLabel(field->layer[DO], gtFont, buffer, 10, voffset + vstep * row++, 100, 10, colour);
        }
    }
    atomic_store_explicit(&force_repaint, true, memory_order_release);
}
#endif

void ff_hit_test(Field* restrict field, int x, int y) {
    auto uid_e = ff_frame_get(field->layer[SC], x, y);
    auto uid_n = ff_frame_get(field->layer[SN], x, y);

    if(field->current[IP_ENTITY] != uid_e || field->current[IP_NODE] != uid_n) {
        auto prior_node = field->prior[IP_NODE];
        auto prior_uid = field->prior[IP_ENTITY];
        if(prior_node >= field->nodes || prior_uid >= field->node[prior_node].entities) return;
        if(uid_n >= field->nodes || uid_e >= field->node[uid_n].entities) return;
        auto prior = &field->node[prior_node].at[prior_uid];
        auto current = &field->node[uid_n].at[uid_e];

        #ifdef DEBUG_OVERLAY
            ff_draw_debug_overlay(field, current);
        #endif

        field->prior[IP_ENTITY] = field->current[IP_ENTITY];
        field->prior[IP_NODE] = field->current[IP_NODE];
        field->current[IP_ENTITY] = uid_e;
        field->current[IP_NODE] = uid_n;
       
        prior->hovered = false;
        prior->repaint = true;
        leave_entity[prior->type](prior, x, y);
        
        current->hovered = true;
        current->repaint = true;
        enter_entity[current->type](current, x, y);
        
        field->repaint = true;
    }
}

void ff_hit_test_drag(Field* restrict field, int x, int y) {
    drag_entity[field->pressed->type](field->pressed, x, y);
    field->repaint = true;
}

void ff_hit_test_up(Field* restrict field, int x, int y, uint32_t button) {
    auto uid_e = ff_frame_get(field->layer[SC], x, y);
    auto uid_n = ff_frame_get(field->layer[SN], x, y);
    field->current[IP_NODE] = uid_n;
    field->current[IP_ENTITY] = uid_e;

    if(field->pressed) {
        release_entity[field->pressed->type](field->pressed, x, y);
        field->pressed = nullptr;
    }
    else {
 //       if(uid_n >= field->nodes || uid_e >= field->node[uid_n].entities) return;
        auto s = &field->node[uid_n].at[uid_e];
        if(!s) return;
        release_entity[s->type](s, x, y);
    }

    field->drag = false;
    field->move = false;
    field->connecting = false;
    field->repaint = true;

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

static void ff_draw_checkbox(Entity* restrict entity) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);
    if(entity->value[CP_COARSE] > 0.5f)
        ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP, entity->bounds.r - GAP, entity->bounds.b - GAP, SELECTIONBACKGROUND);
    entity->repaint = false;
}

void ff_draw_scene(Field* restrict field) {
    for(uint32_t n = 0; n < field->nodes; ++n) {
        if(!field->node[n].visible) continue;
        for(uint32_t i = 0; i < field->node[n].entities; ++i) {
            auto s = &field->node[n].at[i];
            if(s->repaint) {
                draw_entity[s->type](s);
            }
        }
    }
    field->repaint = false;
}

/*****************************************************************************************************************************/

static void ff_set_checkbox(Entity* restrict entity, int, int) {
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (Checkbox*)entity->extension;
    if(entity->flags & RADIO) {
        auto id = ext->radio_id;
        if(*coarse > 0.5f) return;
        else {
            *coarse = 1.0f;
            entity->repaint = true;
            auto field = entity->parent->parent;
            for(uint32_t n = 0; n < field->nodes; ++n) {
                for(uint32_t i = 0; i < field->node[n].entities; ++i) {
                    auto r = &field->node[n].at[i];
                    if((r->flags & RADIO) && ext->radio_id == id && r != entity) {
                        r->value[CP_COARSE] = 0.0f;
                        r->repaint = true;
                        printf("Turned off : %d\n", r->index);
                    }
                }
            }
        }
    }
    else {
        if (*coarse < 0.5f) *coarse = 1.0f;
        else *coarse = 0.0f;
    }
    entity->repaint = true;
}

/*** Slider *******************************************************************************************************************/

static void ff_set_slider(Entity* restrict entity, int x, int y) {
    const float v = 0.1f;
    const auto coarse = &entity->value[CP_COARSE];
    const auto fine = &entity->value[CP_FINE];
    auto ext = (Slider*)entity->extension;
    auto field = entity->parent->parent;

    float value = {};

    if(entity->flags & VERTICAL) {
        const int dy = field->memory[SP_CURSOR_PRIOR].y - y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse; 
        value = step * roundf((float)dy * v) + entity->memory[CP_COARSE];
    }
    else {
        const int dx = field->memory[SP_CURSOR_PRIOR].x - x;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse; 
        value = - step * roundf((float)dx * v) + entity->memory[CP_COARSE];
    }

    if(value < entity->range[0]) {
        *coarse = entity->range[0];
        *fine = 0.0f;
    }
    else if(value > entity->range[1]) {
        *coarse = entity->range[1];
        *fine = 0.0f;
    }
    else *coarse = value;

    entity->repaint = true;
    entity->callback[CT_VALUE](entity, entity->target[CT_VALUE]);
}

static void ff_draw_slider(Entity* restrict entity) {
    auto ext = (Slider*)entity->extension;
    auto coarse = &entity->value[CP_COARSE];
    auto fine = &entity->value[CP_FINE];
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);

    float frac = (*coarse - entity->range[0]) / (entity->range[1] - entity->range[0]);

    switch(ext->type) {
        case SS_A:
            if(entity->flags & VERTICAL) {
                int h   = entity->bounds.b - entity->bounds.t - GRIP * 2 - GAP * 2;
                int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GRIP + GAP;
                ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, pos - GRIP,
                            entity->bounds.r - GAP, pos + GRIP, HIGHLIGHT);
            } else {
                int w   = entity->bounds.r - entity->bounds.l - GRIP * 2 - GAP * 2;
                int pos = (int)(frac * (float)w) + entity->bounds.l + GRIP + GAP;
                ff_draw_rect_f(field->layer[FG], pos - GRIP, entity->bounds.t + GAP,
                            pos + GRIP, entity->bounds.b - GAP, HIGHLIGHT);
            }
            break;

        case SS_B:
            if(entity->flags & VERTICAL) {
                int h   = entity->bounds.b - entity->bounds.t - GAP * 2;
                int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GAP;
                ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, pos,
                            entity->bounds.r - GAP, entity->bounds.b - GAP, HIGHLIGHT);
            } else {
                int w   = entity->bounds.r - entity->bounds.l - GAP * 2;
                int pos = (int)(frac * (float)w) + entity->bounds.l + GAP;
                ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP,
                            pos, entity->bounds.b - GAP, HIGHLIGHT);
            }
            break;

        case SS_C: {
            float range_span = entity->range[1] - entity->range[0];
            float zero_frac = (0.0f - entity->range[0]) / range_span;
            float frac = (*coarse - entity->range[0]) / range_span;
            if(entity->flags & VERTICAL) {
                int h   = entity->bounds.b - entity->bounds.t - GAP * 2;
                int cy = entity->bounds.t + GAP + (int)((1.0f - zero_frac) * (float)h);
                int vy = entity->bounds.t + GAP + (int)((1.0f - frac) * (float)h);
                if(vy < cy) {
                    ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, vy,
                                entity->bounds.r - GAP, cy, HIGHLIGHT);
                } 
                else if(vy > cy) {
                    ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, cy,
                                entity->bounds.r - GAP, vy, HIGHLIGHT);
                }
            } 
            else {
                int w   = entity->bounds.r - entity->bounds.l - GAP * 2;
                int cx = entity->bounds.l + GAP + (int)(zero_frac * (float)w);
                int vx = entity->bounds.l + GAP + (int)(frac * (float)w);
                if(vx > cx) {
                    ff_draw_rect_f(field->layer[FG], cx + 1, entity->bounds.t + GAP,
                                vx, entity->bounds.b - GAP, HIGHLIGHT);
                } 
                else if(vx < cx) {
                    ff_draw_rect_f(field->layer[FG], vx, entity->bounds.t + GAP,
                                cx - 1, entity->bounds.b - GAP, HIGHLIGHT);
                }
            }
            break;
        }

        case SS_LIMIT:
        default:
            break;
    }
    entity->repaint = false;
}

static void ff_scroll_slider(Entity* restrict entity, int x, int y) {
    auto fine = &entity->value[CP_FINE];
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (Slider*)entity->extension;
    float inverse = ext->inverse ? -1.0f : 1.0f;
    auto df = (float)y * ext->step[CP_FINE] * inverse;

    if((*coarse + df < entity->range[1]) && (*coarse + df > entity->range[0])) {
        *fine += df;
    }

    if((int)*fine != 0) {
        *coarse += (int)*fine;
        *fine = 0.0f;
    };

    entity->repaint = true;
    entity->callback[CT_VALUE](entity, entity->target[CT_VALUE]);
}

/*** Slider *******************************************************************************************************************/

static void ff_set_encoder(Entity* restrict entity, int x, int y) {
    const float v = 0.1f;
    const auto coarse = &entity->value[CP_COARSE];
    const auto fine = &entity->value[CP_FINE];
    auto ext = (Slider*)entity->extension;
    auto field = entity->parent->parent;

    float value = {};

    if(entity->flags & VERTICAL) {
        const int dy = field->memory[SP_CURSOR_PRIOR].y - y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse; 
        value = step * roundf((float)dy * v) + entity->memory[CP_COARSE];
    }
    else {
        const int dx = field->memory[SP_CURSOR_PRIOR].x - x;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_COARSE] * inverse; 
        value = - step * roundf((float)dx * v) + entity->memory[CP_COARSE];
    }

    if(value < entity->range[0]) {
        ext->inverse = !ext->inverse;
        entity->memory[CP_COARSE] = entity->range[0];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = entity->range[0];
    }
    else if(value > entity->range[1]) {
        ext->inverse = !ext->inverse;
        entity->memory[CP_COARSE] = entity->range[1];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = entity->range[1];
    }
    else *coarse = value;

    entity->repaint = true;
    entity->callback[CT_VALUE](entity, entity->target[CT_VALUE]);
}

static void ff_draw_encoder(Entity* restrict entity) {
    auto ext = (Slider*)entity->extension;
    auto coarse = &entity->value[CP_COARSE];
    auto fine = &entity->value[CP_FINE];
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);

    float frac = (*coarse - entity->range[0]) / (entity->range[1] - entity->range[0]);

    if(entity->flags & VERTICAL) {
        int h   = entity->bounds.b - entity->bounds.t - GAP * 2;
        if(ext->inverse) {
            frac = 1.0f - frac;
            int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GAP;
            ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP,
                        entity->bounds.r - GAP, pos, HIGHLIGHT);
        } else {
            int pos = (int)((1.0f - frac) * (float)h) + entity->bounds.t + GAP;
            ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, pos,
                        entity->bounds.r - GAP, entity->bounds.b - GAP, HIGHLIGHT);
        }
    } 
    else {
        int w   = entity->bounds.r - entity->bounds.l - GAP * 2;
        int pos = (int)(frac * (float)w) + entity->bounds.l + GAP;
        if(ext->inverse) {
            frac = 1.0f - frac;
            pos = (int)(frac * (float)w) + entity->bounds.l + GAP;
            ff_draw_rect_f(field->layer[FG], 
                        pos, 
                        entity->bounds.t + GAP,
                        entity->bounds.r - GAP, 
                        entity->bounds.b - GAP,
                        HIGHLIGHT);
        }
        else {
            ff_draw_rect_f(field->layer[FG], 
                        entity->bounds.l + GAP, 
                        entity->bounds.t + GAP,
                        pos, 
                        entity->bounds.b - GAP,
                        HIGHLIGHT);
        }
    }

    entity->repaint = false;
}

static void ff_scroll_encoder(Entity* restrict entity, int x, int y) {
    const auto coarse = &entity->value[CP_COARSE];
    const auto fine = &entity->value[CP_FINE];
    auto ext = (Slider*)entity->extension;
    auto field = entity->parent->parent;

    float value = {};

    if(entity->flags & VERTICAL) {
        const int dy = y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_FINE] * inverse; 
        value = step * dy + *coarse;
    }
    else {
        const int dx = y;
        float inverse = ext->inverse ? -1.0f : 1.0f;
        const auto step = ext->step[CP_FINE] * inverse; 
        value = step * dx + *coarse;
    }

    if(value < entity->range[0]) {
        ext->inverse = !ext->inverse;
        entity->memory[CP_COARSE] = entity->range[0];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = entity->range[0];
    }
    else if(value > entity->range[1]) {
        ext->inverse = !ext->inverse;
        entity->memory[CP_COARSE] = entity->range[1];
        field->memory[SP_CURSOR_PRIOR].x = x;
        field->memory[SP_CURSOR_PRIOR].y = y;
        *coarse = entity->range[1];
    }
    else *coarse = value;

    entity->repaint = true;
    entity->callback[CT_VALUE](entity, entity->target[CT_VALUE]);
}

static void ff_draw_button(Entity* restrict entity) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_draw_ltrb_o(field->layer[FG], &entity->bounds, BORDER);

    if(entity->hovered)
        ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP, entity->bounds.r - GAP, entity->bounds.b - GAP, ACTIVE);

    if(entity->value[CP_COARSE] > 0.5f)
        ff_draw_rect_f(field->layer[FG], entity->bounds.l + GAP, entity->bounds.t + GAP, entity->bounds.r - GAP, entity->bounds.b - GAP, ACCENT);

    entity->repaint = false;
}

/******************************************************************************************************************************/

static void ff_draw_rotary(Entity* restrict entity) {
    auto field = entity->parent->parent;
    auto range = entity->range[1] - entity->range[0];
    Sprite* spr = (Sprite*)entity->data;
    int f = ((entity->value[CP_COARSE] - entity->range[0]) / range) * spr->nframes;
    ff_frame_copy_at(field->layer[FG], &spr->data[f], entity->bounds.l, entity->bounds.t);

    entity->repaint = false;
}

static void ff_set_rotary(Entity* restrict entity, int x, int y) {
    auto field = entity->parent->parent;
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (rotary*)entity->extension;
    int dy = roundf((field->memory[SP_CURSOR_PRIOR].y - y)/ext->step[CP_COARSE]);
    int dx = roundf((x - field->memory[SP_CURSOR_PRIOR].x)/ext->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < entity->range[0]) *coarse = entity->range[0];
    else if(*coarse > entity->range[1]) *coarse = entity->range[1];
    entity->repaint = true;

    field->memory[SP_CURSOR_PRIOR].x = (float)x;
    field->memory[SP_CURSOR_PRIOR].y = (float)y;
}

/******************************************************************************************************************************/

static void ff_set_sprite_inf_slider(Entity* restrict entity, int x, int y) {
    auto field = entity->parent->parent;
    auto coarse = &entity->value[CP_COARSE];
    auto ext = (rotary*)entity->extension;
    int dy = roundf((y - field->memory[SP_CURSOR_PRIOR].y)/ext->step[CP_COARSE]);
    int dx = roundf((field->memory[SP_CURSOR_PRIOR].x - x)/ext->step[CP_COARSE]);

    *coarse += (dy + dx);

    if(*coarse < entity->range[0]) *coarse += entity->range[0];
    else if(*coarse > entity->range[1]) *coarse -= entity->range[1];

    // if (o->at[o->current].value <= 0.0f) o->at[o->current].value = o->at[o->current].range;
    // else if (o->at[o->current].value >= o->at[o->current].range) o->at[o->current].value = 0.0f;

    entity->repaint = true;
    field->memory[SP_CURSOR_PRIOR].x = (float)x;
    field->memory[SP_CURSOR_PRIOR].y = (float)y;
}

static void ff_set_button(Entity* restrict entity, int, int) {
    auto field = entity->parent->parent;
    auto coarse = &entity->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    entity->repaint = true;

    field->prior[IP_ENTITY] = field->current[IP_ENTITY];
    field->prior[IP_NODE] = field->current[IP_NODE];
}

static void ff_release_button(Entity* restrict entity, int, int) {
    auto field = entity->parent->parent;
    auto pressed = field->pressed; 
    auto coarse = &pressed->value[CP_COARSE];
    if(pressed) {
        if(pressed == entity) {
            if(*coarse < 0.5f) *coarse = 1.0f;
            else *coarse = 0.0f;
        }
        else *coarse = 0.0f;

        pressed->repaint = true;
    }
}

static void ff_draw_sprite_button(Entity* restrict entity) {
    auto field = entity->parent->parent;
    Sprite* spr = (Sprite*)entity->data;

    if(entity->value[CP_COARSE] > 0.5f)
        ff_frame_copy_at(field->layer[FG], &spr->data[1], entity->bounds.l, entity->bounds.t);
    else
        ff_frame_copy_at(field->layer[FG], &spr->data[0], entity->bounds.l, entity->bounds.t);

    entity->repaint = false;
}

static void ff_init_socket(Entity* restrict entity) {
    entity->data = (float*)calloc(SPLINE_SEGMENTS * 2, sizeof(float));
}

static void ff_set_socket(Entity* restrict entity, int, int) {
    auto field = entity->parent->parent;
    auto coarse = &entity->value[CP_COARSE];
    if(*coarse < 0.5f) *coarse = 1.0f;
    else *coarse = 0.0f;
    entity->repaint = true;

    field->prior[IP_ENTITY] = field->current[IP_ENTITY];
    field->prior[IP_NODE] = field->current[IP_NODE];
}

static void ff_draw_socket(Entity* restrict entity) {
    auto field = entity->parent->parent;
    auto r = entity->width / 2;
    point32u center = {
        .x = entity->bounds.l + r,
        .y = entity->bounds.t + r
    };
    auto color = entity->core_type == F_CT_OUTPUT ? SOCKET_OUT : SOCKET_IN;

    ff_draw_circle_f(field->layer[FG], center.x, center.y, r, color);
    r = entity->width / 4;
    ff_draw_circle_f(field->layer[FG], center.x, center.y, r, BUTTONS);
    entity->repaint = false;
}

static inline point ff_interpolate_bezier(point a, point b, point c, point d, float t)
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

static void ff_drag_socket(Entity* entity, int x, int y)
{
    auto field = entity->parent->parent;
    if(entity->connected) {
        entity = entity->connection;
        field->pressed = entity;
        field->current[IP_ENTITY] = entity->index;
        field->current[IP_NODE] = entity->parent->index;

        auto entity_has_data = entity->has_data;
        auto conn_has_data = entity->connection->has_data;

        if(entity_has_data) {
            memset(entity->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }
        if(conn_has_data) {
            memset(entity->connection->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }
        entity->has_data = false;
        entity->connection->has_data = false;
        entity->connection->connected = false;
        entity->connected = false;

    }

    auto o = entity->parent->parent;

    float xe = (float)x;
    float ye = (float)y;

    if(xe > o->width) xe = (float)o->width;
    else if(xe < 0.0f) xe = 0.0f;

    if(ye > o->height) ye = (float)o->height;
    else if(ye < 0.0f) ye = 0.0f;

    point a, b, c, d;

    float xo = (float)entity->bounds.l + 0.5f * (float)entity->width;
    float yo = (float)entity->bounds.t + 0.5f * (float)entity->height;

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

    auto uva = ff_screen_to_uv(a.x, a.y, o->width, o->height);
    auto uvb = ff_screen_to_uv(b.x, b.y, o->width, o->height);
    auto uvc = ff_screen_to_uv(c.x, c.y, o->width, o->height);
    auto uvd = ff_screen_to_uv(d.x, d.y, o->width, o->height);

    for(uint32_t i = 0, j = 0; i < SPLINE_SEGMENTS; i++) {
        point uvp = ff_interpolate_bezier(uva, uvb, uvc, uvd, t);

        ((float*)entity->data)[j++] = uvp.x;
        ((float*)entity->data)[j++] = uvp.y;

        t += inc;
    }

    entity->has_data = true;
}

static void ff_drag_cord(Entity* restrict entity, int x, int y)
{
    if(entity->connected) {
        if(entity->has_data) {
            memset(entity->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }
        if(entity->connection->has_data) {
            memset(entity->connection->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
            entity->connection->has_data = false;
            entity->connection->connected = false;
        }
    }

    auto o = entity->parent->parent;

    float xe = (float)x;
    float ye = (float)y;

    if(xe > o->width) xe = (float)o->width;
    else if(xe < 0.0f) xe = 0.0f;

    if(ye > o->height) ye = (float)o->height;
    else if(ye < 0.0f) ye = 0.0f;

    point a, b, c, d;

    float xo = (float)entity->bounds.l + 0.5f * (float)entity->width;
    float yo = (float)entity->bounds.t + 0.5f * (float)entity->height;

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

    auto uva = ff_screen_to_uv(a.x, a.y, o->width, o->height);
    auto uvb = ff_screen_to_uv(b.x, b.y, o->width, o->height);
    auto uvc = ff_screen_to_uv(c.x, c.y, o->width, o->height);
    auto uvd = ff_screen_to_uv(d.x, d.y, o->width, o->height);

    for(uint32_t i = 0, j = 0; i < SPLINE_SEGMENTS; i++) {
        point uvp = ff_interpolate_bezier(uva, uvb, uvc, uvd, t);

        ((float*)entity->data)[j++] = uvp.x;
        ((float*)entity->data)[j++] = uvp.y;

        t += inc;
    }

    entity->has_data = true;
}

inline static bool ff_connect(Entity* restrict source, Entity* restrict target) {
    auto ext_s = (Socket*)source->extension;
    auto ext_t = (Socket*)target->extension;

    if(source->core_type == F_CT_INPUT && target->core_type == F_CT_OUTPUT) {
        ext_s->input = ext_t->output;
    }
    else if(source->core_type == F_CT_OUTPUT && target->core_type == F_CT_INPUT) {
        ext_t->input = ext_s->output;
    }
    else return false;

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
    ff_drag_cord(source, target->bounds.l + target->width / 2, target->bounds.t + target->height / 2);
    source->connected = true;
    target->connected = true;
    source->connection = target;
    target->connection = source;
    source->hovered = false;
    target->hovered = true;
    source->parent->parent->connecting = false;

    return true;
}

inline static void ff_disconnect(Entity* restrict entity) {
    auto target = entity->connection;

    if(entity->core_type == F_CT_INPUT) {
        auto ext = (Socket*)entity->extension;
        ext->input = nullptr;
    }
    else if(target && target->core_type == F_CT_INPUT) {
        auto ext = (Socket*)target->extension;
        ext->input = nullptr;
    }

    if(entity->has_data) {
        memset(entity->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        entity->has_data = false;
    }
    entity->connected = false;

    if(target) {
        if(target->has_data) {
            memset(target->data, 0, SPLINE_SEGMENTS * 2 * sizeof(float));
        }

        target->connected = false;
        target->has_data = false;
        entity->connection->connection = nullptr;
        entity->connection = nullptr;
    }
}

static void ff_release_socket(Entity* restrict entity, int x, int y) {
    auto field = entity->parent->parent;
    auto target_uid = ff_frame_get(field->layer[SC], x, y);
    auto target_nid = ff_frame_get(field->layer[SN], x, y);
    if(target_nid >= field->nodes || target_uid >= field->node[target_nid].entities) return;
    auto target = &field->node[target_nid].at[target_uid];
    
    if(target->connected) ff_disconnect(target);
    if(ff_connect(entity, target)) return;
    ff_disconnect(entity);
}

/*****************************************************************************************************************************/

static void ff_init_node(Entity* restrict entity) {
    entity->data = (Frame*)calloc(1, sizeof(Frame));
    ff_frame_init((Frame*)entity->data, entity->width, entity->height);
}

static void ff_set_node(Entity* restrict entity, int, int) {
    entity->repaint = true;
    auto field = entity->parent->parent;
    field->prior[IP_ENTITY] = field->current[IP_ENTITY];
    field->prior[IP_NODE] = field->current[IP_NODE];
}

static inline bool ff_has_overlap(const Entity* restrict entity) {
    auto field = entity->parent->parent;
    const auto l = field->layer[SN];
    for(uint32_t y = entity->bounds.t; y < entity->bounds.b; ++y) {
        for(uint32_t x = entity->bounds.l; x < entity->bounds.r; ++x) {
            auto v = ff_frame_get(l, x, y);
            if(v && v != entity->parent->index)
                return true;
        }
    }
    return false;
}

static void ff_draw_node(Entity* restrict entity) {
    if(entity->visible) {
        auto field = entity->parent->parent;
        auto l = entity->staging ? ST : NG;

        if(l == ST) {
            ff_frame_clr(field->layer[l]);
            ff_draw_ltrb_o(field->layer[l], &entity->bounds, ff_has_overlap(entity) ? ERROR : HIGHLIGHT);
        }
        else {
            ff_draw_ltrb_f(field->layer[l], &entity->bounds, SECONDBACKGROUND);
            ff_draw_ltrb_o(field->layer[l], &entity->bounds, BORDER);
        }
        entity->repaint = false;
    }
}

static void ff_drag_node(Entity* restrict entity, int x, int y)
{
    auto field = entity->parent->parent;
    bool moved = false;

    int dx = ((x - field->memory[SP_CURSOR_PRIOR].x) / field->step) * field->step;
    {
        int l = ((entity->bounds.l + dx) / field->step) * field->step;
        if((l >= 0) && (l + entity->width <= field->width)) {
            entity->bounds.l = l;
            entity->bounds.r = l + entity->width;
            moved = true;
        }
    }

    int dy = ((y - field->memory[SP_CURSOR_PRIOR].y) / field->step) * field->step;
    {
        int t = ((entity->bounds.t + dy) / field->step) * field->step;
        if((t >= 0) && (t + entity->height <= field->height)) {
            entity->bounds.t = t; 
            entity->bounds.b = t + entity->height;
            moved = true;
        }
    }

    if (moved) {
        
        field->memory[SP_CURSOR_PRIOR].x += dx;
        field->memory[SP_CURSOR_PRIOR].y += dy;
        
        entity->repaint = true;
        field->repaint = true;
    }
}

static void ff_release_node(Entity* restrict entity, int, int) 
{
    auto field = entity->parent->parent;
    auto overlap = ff_has_overlap(entity);

    ltrb32u ir = {
        .l = field->memory[SP_LT_PRESS].x, 
        .t = field->memory[SP_LT_PRESS].y, 
        .r = field->memory[SP_LT_PRESS].x + entity->width, 
        .b = field->memory[SP_LT_PRESS].y + entity->height
    };

    if(overlap) { 
        entity->bounds = ir;
        ff_draw_ltrb_f(field->layer[SN], &ir, entity->parent->index);
        ff_draw_ltrb_f(field->layer[NG], &ir, 0x0);

        for(uint32_t i = 0; i < entity->nodes; ++i) {
            entity->parent->at[i].repaint = true;
        }
    }
    else {
        ff_draw_ltrb_f(field->layer[SN], &ir, 0x0);
        ff_draw_ltrb_f(field->layer[SN], &entity->bounds, entity->parent->index);

        ff_draw_ltrb_f(field->layer[NG], &ir, 0x0);

        int dx = entity->bounds.l - field->memory[SP_LT_PRESS].x;
        int dy = entity->bounds.t - field->memory[SP_LT_PRESS].y;

        for(uint32_t i = 0; i < entity->parent->entities; ++i) {
            auto child = &entity->parent->at[i];
            if(child == entity) continue;

            ltrb32u r = {
                .l = child->bounds.l + dx,
                .t = child->bounds.t + dy,
                .r = child->bounds.r + dx,
                .b = child->bounds.b + dy
            };

            ff_move_entity(child, &r); 

            if(child->connected) {
                auto target = child->connection;
                if(child->has_data) {
                    ff_drag_cord(child, target->bounds.l + target->width / 2, target->bounds.t + target->height / 2);
                }
                else if(target->has_data) {
                    ff_drag_cord(target, child->bounds.l + child->width / 2, child->bounds.t + child->height / 2);
                }
            }

            child->repaint = true;
        }
    }

    ff_frame_clr(field->layer[ST]);
    field->staging = false;
    entity->staging = false;
    entity->repaint = true;
    field->repaint = true;
}

/*****************************************************************************************************************************/

static void ff_init_textbox(Entity* restrict entity) {
    entity->data = (char*)calloc(TEXTBOX_SIZE, sizeof(char)); 
}

static void ff_set_textbox(Entity* restrict entity, int, int) {
    entity->repaint = true;
}

static void ff_draw_textbox(Entity* restrict entity) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, SECONDBACKGROUND);
    char *text = entity->data;
    //auto len = strlen(text);
    //auto offset_x = (entity->width - 10 * len) / 2;
    auto offset_y = (entity->height - 8) / 2;

    ffDrawTextLabel(
        field->layer[FG],
        gtFont,
        text,
        entity->bounds.l,
        entity->bounds.t + offset_y,
        0,
        0,
        TEXT
    );

    entity->repaint = false;
}

/*****************************************************************************************************************************/

static void ff_draw_graticule(Frame* frame, uint32_t colour, uint32_t vdiv, uint32_t hdiv) {
    const float W = (float)frame->width;
    const float H = (float)frame->height;

    const float stepX = W / (float)vdiv;
    const float stepY = H / (float)hdiv;
    const float step  = stepX < stepY ? stepX : stepY;

    const float gridW = step * (float)vdiv;
    const float gridH = step * (float)hdiv;

    const float hgap = (W - gridW) * 0.5f;
    const float vgap = (H - gridH) * 0.5f;

    const float cx = hgap + gridW * 0.5f;
    const float cy = vgap + gridH * 0.5f;

    const float sub = step / 5.0f;

    // Major vertical lines
    const uint32_t y0 = (uint32_t)(vgap + 0.5f);
    const uint32_t y1 = (uint32_t)(vgap + gridH - 0.5f);
    for (uint32_t i = 0; i <= vdiv; ++i) {
        auto x = (uint32_t)(hgap + (float)i * step + 0.5f);
        if (x >= frame->width) x = frame->width - 1;
        ff_draw_line_v(frame, x, y0, y1, colour);
    }

    // Major horizontal lines
    const uint32_t x0 = (uint32_t)(hgap + 0.5f);
    const uint32_t x1 = (uint32_t)(hgap + gridW - 0.5f);
    for (uint32_t i = 0; i <= hdiv; ++i) {
        auto y = (uint32_t)(vgap + (float)i * step + 0.5f);
        if (y >= frame->height) y = frame->height - 1;
        ff_draw_line_h(frame, y, x0, x1, colour);
    }

    // Minor vertical ticks
    const float tickH = step * 0.2f;
    const uint32_t ty0 = (uint32_t)(cy - tickH + 0.5f);
    const uint32_t ty1 = (uint32_t)(cy + tickH - 0.5f);
    for (uint32_t i = 0; i < vdiv * 5; ++i) {
        auto x = (uint32_t)(hgap + (float)i * sub + 0.5f);
        if (x >= frame->width) x = frame->width - 1;
        ff_draw_line_v(frame, x, ty0, ty1, colour);
    }

    // Minor horizontal ticks
    const float tickW = step * 0.2f;
    const uint32_t tx0 = (uint32_t)(cx - tickW + 0.5f);
    const uint32_t tx1 = (uint32_t)(cx + tickW - 0.5f);
    for (uint32_t i = 0; i < hdiv * 5; ++i) {
        auto y = (uint32_t)(vgap + (float)i * sub + 0.5f);
        if (y >= frame->height) y = frame->height - 1;
        ff_draw_line_h(frame, y, tx0, tx1, colour);
    }

    // Dotted lines at ±2.4 divisions
    const float dottedOffset = step * 2.4f;
    const auto yt = (size_t)(cy - dottedOffset + 0.5f);
    const auto yb = (size_t)(cy + dottedOffset + 0.5f);

    for (uint32_t i = 0; i < vdiv * 5; ++i) {
        auto x = (size_t)(hgap + (float)i * sub + 0.5f);
        if (x >= frame->width) x = frame->width - 1;
        ff_frame_set(frame, x, yt, colour);
        ff_frame_set(frame, x, yb, colour);
    }
}

static void ff_init_crt(Entity* restrict entity) {
    entity->data = malloc(sizeof(Frame));
    ff_frame_init((Frame*)entity->data, entity->width, entity->height);
    ff_draw_graticule((Frame*)entity->data, BACKGROUND, 10, 8);
    
    auto ext = (Oscillograph*)entity->extension;
    ext->data = (float*)calloc(SPLINE_SEGMENTS * 2, sizeof(float));
    ext->r_head = 0;
    ext->w_head = 0;

    entity->parent->parent->stream[0] = entity;
}

static void ff_set_crt(Entity* restrict entity, int, int) {
    entity->repaint = true;
}

static void ff_draw_crt(Entity* restrict entity) {
    auto field = entity->parent->parent;
    ff_draw_ltrb_f(field->layer[FG], &entity->bounds, BUTTONS);
    ff_frame_copy_at(field->layer[FG], (Frame*)entity->data, entity->bounds.l, entity->bounds.t);

    entity->repaint = false;
}

/*****************************************************************************************************************************/

static void ff_draw_canvas(Entity* restrict entity) 
{
    auto field = entity->parent->parent;
    ff_frame_fill(field->layer[BG], BACKGROUND);
    uint32_t major = 4;
    if(true) {
            
        for(uint32_t x = field->step, l = 1; x < field->layer[BG]->width; x += field->step) {
            for(uint32_t y = 0; y < field->layer[BG]->height; ++y) {
                if(!ff_frame_get(field->layer[SN], x, y))
                    ff_frame_set(field->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
   
        for(uint32_t y = field->step, l = 1; y < field->layer[BG]->height; y += field->step) {
            for(uint32_t x = 0; x < field->layer[BG]->width; ++x) {
                if(!ff_frame_get(field->layer[SN], x, y))
                    ff_frame_set(field->layer[BG], x, y, l ? MINOR : MAJOR);
            }
            if(++l >= major) l = 0;
        }
    }
}

/*****************************************************************************************************************************/

static inline void ff_init_none(Entity* restrict) {}

void (*init_entity[])(Entity* restrict)  = {
    [ST_SLIDER]                 = ff_init_none,
    [ST_ENCODER]                = ff_init_none,
    [ST_ROTARY]                 = ff_init_none,  
    [ST_SPRITE_INF_SLIDER]      = ff_init_none,  
    [ST_SOCKET]                 = ff_init_socket,
    [ST_CHECKBOX]               = ff_init_none,  
    [ST_MOMENTARY]              = ff_init_none,  
    [ST_SPRITE_CHECKBOX]        = ff_init_none,  
    [ST_SPRITE_BUTTON]          = ff_init_none,  
    [ST_CANVAS]                 = ff_init_none,  
    [ST_TEXTBOX]                = ff_init_textbox,  
    [ST_NODE]                   = ff_init_node,  
    [ST_CRT]                    = ff_init_crt,  
};

static inline void ff_set_none(Entity* restrict, int, int) {}

void (*set_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_set_slider,   
    [ST_ENCODER]                = ff_set_encoder,   
    [ST_ROTARY]                 = ff_set_rotary,     
    [ST_SPRITE_INF_SLIDER]      = ff_set_sprite_inf_slider, 
    [ST_SOCKET]                 = ff_set_socket,            
    [ST_CHECKBOX]               = ff_set_checkbox,          
    [ST_MOMENTARY]              = ff_set_button,            
    [ST_SPRITE_CHECKBOX]        = ff_set_checkbox,          
    [ST_SPRITE_BUTTON]          = ff_set_button,            
    [ST_CANVAS]                 = ff_set_none,              
    [ST_TEXTBOX]                = ff_set_textbox,
    [ST_NODE]                   = ff_set_node,       
    [ST_CRT]                    = ff_set_crt        
};

void (*draw_entity[])(Entity* restrict) = {
    [ST_SLIDER]                 = ff_draw_slider,        
    [ST_ENCODER]                = ff_draw_encoder,        
    [ST_ROTARY]                 = ff_draw_rotary, 
    [ST_SPRITE_INF_SLIDER]      = ff_draw_rotary, 
    [ST_SOCKET]                 = ff_draw_socket,        
    [ST_CHECKBOX]               = ff_draw_checkbox,      
    [ST_MOMENTARY]              = ff_draw_button,        
    [ST_SPRITE_CHECKBOX]        = ff_draw_sprite_button, 
    [ST_SPRITE_BUTTON]          = ff_draw_sprite_button, 
    [ST_CANVAS]                 = ff_draw_canvas,        
    [ST_TEXTBOX]                = ff_draw_textbox,
    [ST_NODE]                   = ff_draw_node,
    [ST_CRT]                    = ff_draw_crt
};

static inline void ff_drag_none(Entity* restrict, int, int) {}

void (*drag_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_set_slider,           
    [ST_ENCODER]                = ff_set_encoder,           
    [ST_ROTARY]                 = ff_set_rotary,    
    [ST_SPRITE_INF_SLIDER]      = ff_set_sprite_inf_slider,
    [ST_SOCKET]                 = ff_drag_socket,          
    [ST_CHECKBOX]               = ff_drag_none,             
    [ST_MOMENTARY]              = ff_drag_none,             
    [ST_SPRITE_CHECKBOX]        = ff_drag_none,             
    [ST_SPRITE_BUTTON]          = ff_drag_none,             
    [ST_CANVAS]                 = ff_drag_none,             
    [ST_TEXTBOX]                = ff_drag_none,  
    [ST_NODE]                   = ff_drag_node,          
    [ST_CRT]                    = ff_drag_none,  
};

static inline void ff_scroll_none(Entity* restrict, int, int) {}

void (*scroll_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_scroll_slider,     
    [ST_ENCODER]                = ff_scroll_encoder,     
    [ST_ROTARY]                 = ff_scroll_slider,     
    [ST_SPRITE_INF_SLIDER]      = ff_scroll_slider,     
    [ST_SOCKET]                 = ff_scroll_none,          
    [ST_CHECKBOX]               = ff_scroll_none,          
    [ST_MOMENTARY]              = ff_scroll_none,          
    [ST_SPRITE_CHECKBOX]        = ff_scroll_none,          
    [ST_SPRITE_BUTTON]          = ff_scroll_none,          
    [ST_CANVAS]                 = ff_scroll_none,          
    [ST_TEXTBOX]                = ff_scroll_none,  
    [ST_NODE]                   = ff_scroll_none,
    [ST_CRT]                    = ff_scroll_none,
};

static inline void ff_enter_none(Entity* restrict, int, int) {}

void (*enter_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_enter_none,         
    [ST_ENCODER]                = ff_enter_none,         
    [ST_ROTARY]                 = ff_enter_none,         
    [ST_SPRITE_INF_SLIDER]      = ff_enter_none,         
    [ST_SOCKET]                 = ff_enter_none,         
    [ST_CHECKBOX]               = ff_enter_none,         
    [ST_MOMENTARY]              = ff_enter_none,   
    [ST_SPRITE_CHECKBOX]        = ff_enter_none,         
    [ST_SPRITE_BUTTON]          = ff_enter_none,   
    [ST_CANVAS]                 = ff_enter_none,         
    [ST_TEXTBOX]                = ff_enter_none,  
    [ST_NODE]                   = ff_enter_none,
    [ST_CRT]                    = ff_enter_none,
};

static inline void ff_leave_none(Entity* restrict, int, int) {}

void (*leave_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_leave_none,         
    [ST_ENCODER]                = ff_leave_none,         
    [ST_ROTARY]                 = ff_leave_none,         
    [ST_SPRITE_INF_SLIDER]      = ff_leave_none,         
    [ST_SOCKET]                 = ff_leave_none,         
    [ST_CHECKBOX]               = ff_leave_none,         
    [ST_MOMENTARY]              = ff_leave_none,   
    [ST_SPRITE_CHECKBOX]        = ff_leave_none,         
    [ST_SPRITE_BUTTON]          = ff_leave_none,   
    [ST_CANVAS]                 = ff_leave_none,         
    [ST_TEXTBOX]                = ff_leave_none,  
    [ST_NODE]                   = ff_leave_none,
    [ST_CRT]                    = ff_leave_none  
};

static inline void ff_release_none(Entity* restrict, int, int) {}

void (*release_entity[])(Entity* restrict, int, int) = {
    [ST_SLIDER]                 = ff_release_none,
    [ST_ENCODER]                = ff_release_none,
    [ST_ROTARY]                 = ff_release_none,
    [ST_SPRITE_INF_SLIDER]      = ff_release_none,
    [ST_SOCKET]                 = ff_release_socket,
    [ST_CHECKBOX]               = ff_release_none,
    [ST_MOMENTARY]              = ff_release_button,
    [ST_SPRITE_CHECKBOX]        = ff_release_none,
    [ST_SPRITE_BUTTON]          = ff_release_button,
    [ST_CANVAS]                 = ff_release_none,
    [ST_TEXTBOX]                = ff_release_none,  
    [ST_NODE]                   = ff_release_node,
    [ST_CRT]                    = ff_release_node,
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
