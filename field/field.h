#pragma  once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "containers.h"
#include "primitives.h"
#include "colours.h"

typedef enum {
    ST_SLIDER, 
    ST_PROGRESS_BAR, 
    ST_SPRITE_SLIDER,
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
    uint32_t    flags;
    uint32_t    nodes;
    uint32_t    capacity;
    void        (*callback[CT_LIMIT])(sector*, sector*);
    sector*     target[CT_LIMIT];
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
    bool        refresh;            // Swap buffer flag
    bool        drag;               // Sector drag flag
    bool        move;               // Window drag flag
    bool        staging;            // Draw staging layer
    bool        connecting;
    uint32_t    sectors;
    uint32_t    capacity;
    uint32_t    flags;
    uint32_t    step;
};

void fuse_link(sector*, sector*);
void draw_scene(field* o);

void add_mod_link(sector*, sector*, CallbackType, void (*)(sector*, sector*));
void link_sector(sector*, sector*);
void move_sector(sector*, ltrb32u*);
void erase_sector(sector*);

/*****************************************************************************************************************************/

sector* createSector(field*, sector*, SectorType, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void initField   (field*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
void destroyField(field*);

/*****************************************************************************************************************************/

void hit_test_down(field*, int, int, MouseButton);
void hit_test_drag(field*, int, int);
void hit_test_up  (field*, int, int, MouseButton);
void hit_test     (field*, int, int);

/*****************************************************************************************************************************/

extern void (*init_sector[])(sector*);
extern void (*set_sector[])(sector*, int, int);
extern void (*draw_sector[])(sector*);
extern void (*drag_sector[])(sector*, int, int);
extern void (*scroll_sector[])(sector*, int, int);
extern void (*leave_sector[])(sector*, int, int);
extern void (*release_sector[])(sector*, int, int);
