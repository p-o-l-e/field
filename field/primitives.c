#include <stdint.h>
#include <string.h>
#include <math.h>
#include "primitives.h"
#include "containers.h"

/**************************************************************************************************************************
 * Returns 8-bit value
 * Data : 0x FF FF FF FF
 * index:     3  2  1  0     MSB->LSB
 * ***********************************************************************************************************************/
uint8_t extract_byte(uint32_t value, uint_fast8_t byte) [[unsequenced]] {
    return (value >> (byte * 8)) & 0xFF;
}

point32u uv_to_screen(float x, float y, uint32_t w, uint32_t h) [[reproducible]] {
    point32u s = {
        .x = (uint32_t)(x * (float)w),
        .y = (uint32_t)(y * (float)h)
    };
    return s;
}

point screen_to_uv(uint32_t x, uint32_t y, uint32_t w, uint32_t h) [[reproducible]] {
    point n = {
        .x =  (float)x / (float)w,
        .y =  1.0f - (float)y / (float)h
    };
    return n;
}

/**************************************************************************************************************************
 * Returns 32-bit identifier [ AA BB CC DD  ]
 * AA : Module type
 * BB : Module id/position
 * CC : Parameter type
 * DD : Parameter id/position
 * ***********************************************************************************************************************/
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

void draw_rectangle(frame* canvas, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t i = l; i <= r; i++)
    {
        frame_set(canvas, i, t, colour);
        frame_set(canvas, i, b, colour);
    }

    for(uint32_t i = t; i <= b; i++)
    {
        frame_set(canvas, l, i, colour);
        frame_set(canvas, r, i, colour);
    }
}

void draw_ltrb_o(frame* canvas, ltrb32u* r, const uint32_t colour) {
    for(uint32_t i = r->l; i <= r->r; ++i) {
        frame_set(canvas, i, r->t, colour);
        frame_set(canvas, i, r->b, colour);
    }
    for(uint32_t i = r->t; i <= r->b; i++) {
        frame_set(canvas, r->l, i, colour);
        frame_set(canvas, r->r, i, colour);
    }
}

void draw_rect_f(frame* canvas, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t y = t; y <= b; ++y) {
        for(uint32_t x = l; x <= r; ++x) {
            frame_set(canvas, x, y, colour);
        }
    }
}

void draw_ltrb_f(frame* canvas, ltrb32u* r, const uint32_t colour)
{
    for(uint32_t y = r->t; y <= r->b; ++y) {
        for(uint32_t x = r->l; x <= r->r; ++x) {
            frame_set(canvas, x, y, colour);
        }
    }
}

inline void draw_line_v(frame* o, uint32_t x, uint32_t yo, uint32_t ye, const uint32_t value) {
    for(uint32_t y = yo; y <= ye; ++y)
        frame_set(o, x, y, value);
}

inline void draw_line_h(frame* o, uint32_t y, uint32_t xo, uint32_t xe, const uint32_t value) {
    for(uint32_t x = xo; x <= xe; ++x)
        frame_set(o, x, y, value);
}

void draw_circle_f(frame* o, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value)
{
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

void draw_circle_o(frame* o, uint32_t xc, uint32_t yc, uint32_t r, uint32_t value)
{
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

void draw_glyph(frame* canvas, const uint8_t* font, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour)
{
    uint32_t pos = id * 7;
    u_int8_t stencil = 0b1;

    for(uint32_t y = 0; y < 8; y++)
    {
        for(uint32_t x = 0; x < 7; x++)
        {
            if(font[pos + x] & stencil) frame_set(canvas, x + l, y + t, colour);

        }
        stencil<<=1;
    }
}

void draw_text_label(frame* canvas, const uint8_t* font, const char* text, uint32_t l, uint32_t t, uint32_t, uint32_t, const uint32_t colour)
{
    uint32_t n = strlen(text);

    for(uint32_t i = 0; i < n; i++)
    {
        draw_glyph(canvas, font, text[i] - 32, l + i * 8, t, colour);
    }
}

