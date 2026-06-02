#include <math.h>
#include <stdint.h>
#include "primitives.h"

void draw_rectangle(frame* canvas, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t i = l; i <= r; i++)
    {
        frame_pset(canvas, i, t, colour);
        frame_pset(canvas, i, b, colour);
    }

    for(uint32_t i = t; i <= b; i++)
    {
        frame_pset(canvas, l, i, colour);
        frame_pset(canvas, r, i, colour);
    }
}

void draw_rectangle_filled(frame* canvas, uint32_t l, uint32_t t, uint32_t r, uint32_t b, const uint32_t colour)
{
    for(uint32_t y = t; y <= b; y++)
    {
        for(uint32_t x = l; x <= r; x++)
        {
            frame_pset(canvas, x, y, colour);
        }
    }
}

void draw_glyph(frame* canvas, const uint8_t* font, uint32_t id, uint32_t l, uint32_t t, const uint32_t colour)
{
    uint32_t pos = id * 7;
    u_int8_t stencil = 0b1;

    for(uint32_t y = 0; y < 8; y++)
    {
        for(uint32_t x = 0; x < 7; x++)
        {
            if(font[pos + x] & stencil) frame_pset(canvas, x + l, y + t, colour);

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

