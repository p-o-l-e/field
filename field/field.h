#pragma  once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "containers.h"
#include "primitives.h"
#include "colours.h"

constexpr uint_fast32_t SPLINE_SEGMENTS = 32;

typedef enum {
    ST_SLIDER, 
    ST_PROGRESS_BAR, 
    ST_ROTARY,
    ST_SPRITE_INF_SLIDER,
    ST_SOCKET,
    ST_CHECKBOX, 
    ST_BUTTON, 
    ST_SPRITE_CHECKBOX, 
    ST_SPRITE_BUTTON, 
    ST_CANVAS,
    ST_TEXTBOX,
    ST_NODE,

    ST_LIMIT

} SectorType;

typedef enum {
    CT_PRESS,
    CT_RELEASE,
    CT_VALUE,

    CT_LIMIT

} CallbackType;
            
typedef enum { 
    LMB,
    MMB,
    RMB

} MouseButton;

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
    CC,                         // controls count

} FieldLayer;

typedef enum: uint32_t {
    VERTICAL    = 1 << 0,
    MOVEABLE    = 1 << 1,
    INTERCON    = 1 << 2,
    INPUT       = 1 << 3,
    OUTPUT      = 1 << 4,

} SectorFlags;

typedef enum {
    ROOT        = 1 << 0,

} FieldFlags;

/*****************************************************************************************************************************/
typedef struct field field;
typedef struct sector sector;

struct sector {
    uint32_t    id;
    ltrb32u     bounds;
    uint32_t    width;
    uint32_t    height;
    SectorType  type;                   
    void*       data;                   // Type dependent data
    float       value[CP_LIMIT];
    float       memory[CP_LIMIT];
    float       range[2];
    float       step[CP_LIMIT];
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

void fuse_link(sector*, sector*);
void draw_scene(field* restrict o);

void add_mod_link(sector*, sector*, CallbackType, void (*)(sector*, sector*));
void link_sector(sector*, sector*);
void move_sector(sector* restrict, ltrb32u* restrict);
void erase_sector(sector* restrict);

/*****************************************************************************************************************************/

sector* createSector(field* restrict, sector* restrict, SectorType, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void initField   (field* restrict, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void destroyField(field* restrict);

/*****************************************************************************************************************************/

void hit_test_down(field* restrict, int, int, MouseButton);
void hit_test_drag(field* restrict, int, int);
void hit_test_up  (field* restrict, int, int, MouseButton);
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
