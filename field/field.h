#pragma  once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "containers.h"
#include "primitives.h"
#include "colours.h"

typedef enum {
    SLIDER_H,       
    SLIDER_V, 
    STEP_SLIDER_H, 
    STEP_SLIDER_V, 
    PROGRESS_BAR_H, 
    PROGRESS_BAR_V, 
    SPRITE_SLIDER,
    SPRITE_INF_SLIDER,
    SOCKET,
    TITLE_BAR, 
    CHECKBOX, 
    BUTTON, 
    SPRITE_CHECKBOX, 
    SPRITE_BUTTON, 
    CANVAS

} sector_type;
            
typedef enum { 
    LMB,
    MMB,
    RMB

} mouse_button;

typedef enum {
    VERTICAL = 1 << 0,
    DRAGABLE = 1 << 1,

} sector_flags;

/*****************************************************************************************************************************/

typedef struct
{
    ltrb32          bounds;     // Rectangle area
    sector_type     type;       // Defined in sector_type 
    void*           data;       // Type dependent data
    float           value;
    float           range;
    float           step;
    point           lp;         // Last cursor position
    bool            visible;
    bool            hovered;
    bool            repaint;
    bool            dragable;
    bool            on;       
    uint32_t        id;         // Unique identifier
    sector_flags    flags;
    void (*callback)();         // Callback
    struct sector*  to;         // Linked node

} sector;


/*****************************************************************************************************************************/

#define _ftype uint32_t
typedef struct 
{
    ltrb32      bounds;
    frame       canvas;  // Foreground
    frame       stencil; // HitTest data
    frame       bg;      // Backgroung
    sector*     at;      // Controls array
    uint32_t    current; // HitTest return
    uint32_t    prior;   // Last HitTest
    uint32_t    count;   // Controls quantity
    bool        repaint; // Repaint flag
    bool        refresh; // Swap buffer flag
    bool        drag;    // Sector drag flag
    bool        move;    // Window drag flag

} field;

void empty();
void draw_scene   (field* o);


/*****************************************************************************************************************************/

void init_sector  (field* o, int position, sector_type type, int x, int y, int w, int h);
void init_field   (field* o, uint32_t x, uint32_t y, uint32_t w, uint32_t h, unsigned size);
void destroy_field(field* o);

/*****************************************************************************************************************************/

void hit_test_down(field* o, int x, int y, mouse_button button);
void hit_test_drag(field* o, int x, int y);
void hit_test_up  (field* o, int x, int y, mouse_button button);
void hit_test     (field* o, int x, int y);

/*****************************************************************************************************************************/

void draw_h_progress_bar(field* o, sector* c);
void draw_v_progress_bar(field* o, sector* c);

/*****************************************************************************************************************************/

void draw_h_slider (field* o, sector* c);
void set_h_slider  (field* o, int x, int y);
void drag_h_slider (field* o, int x, int y);

/*****************************************************************************************************************************/

void draw_v_slider (field* o, sector* c);
void set_v_slider  (field* o, int x, int y);
void drag_v_slider (field* o, int x, int y);
void scroll_slider (field* o, int x, int y);

/*****************************************************************************************************************************/

void draw_checkbox (field* o, sector* c);
void set_checkbox  (field* o, int x, int y);

/*****************************************************************************************************************************/

void set_h_step_slider (field* o, int x, int y);
void drag_h_step_lider (field* o, int x, int y);

/*****************************************************************************************************************************/

void set_v_step_slider (field* o, int x, int y);
void drag_v_step_slider(field* o, int x, int y);
void scroll_step_slider(field* o, int x, int y);

/*****************************************************************************************************************************/

void drag_title_bar  (field* o, int x, int y);
void draw_title_bar  (field* o, sector* c);

/*****************************************************************************************************************************/

void draw_button   (field* o, sector* c);
void release_button(field* o, int x, int y);
void set_button    (field* o, int x, int y);

/*****************************************************************************************************************************/

void draw_sprite_button(field* o, sector* c);
void draw_sprite_slider(field* o, sector* c);
void set_sprite_slider(field* o, int x, int y);
void set_sprite_inf_slider(field* o, int x, int y);

void draw_socket(field* o, sector* c);
void drag_socket(field* o, int x, int y);




void set_none   (field* o, int x, int y);
void draw_canvas(field* o, sector* c);

extern void (*set_sector[])   (field*, int, int);
extern void (*draw_sector[])  (field*, sector*);
extern void (*drag_sector[])  (field*, int, int);
extern void (*scroll_sector[])(field*, int, int);
extern void (*leave_sector[]) (field*, int, int);
