#include <math.h>
#include "primitives.h"


void draw_rectangle(frame* canvas, int xo, int yo, int xe, int ye, const uint colour)
{
    for(int i = xo; i <= xe; i++)
    {
        frame_pset(canvas, i, yo, colour);
        frame_pset(canvas, i, ye, colour);
    }
    for(int i = yo; i <= ye; i++)
    {
        frame_pset(canvas, xo, i, colour);
        frame_pset(canvas, xe, i, colour);
    }
}


void draw_rectangle_filled(frame* canvas, int xo, int yo, int xe, int ye, const uint colour)
{
    for(int y = yo; y <= ye; y++)
    {
        for(int x = xo; x <= xe; x++)
        {
            frame_pset(canvas, x, y, colour);
        }
    }
}


void draw_glyph(frame* canvas, uint8_t* font, int id, int xo, int yo, const uint colour)
{
    int pos = id*7;
    u_int8_t stencil = 0b1;
    for(int y = 0; y < 8; y++)
    {
        for(int x = 0; x < 7; x++)
        {
            if(font[pos + x]&stencil) frame_pset(canvas, x + xo, y + yo, colour);

        }
        stencil<<=1;
    }
}

void draw_text_label(frame* canvas, char* font, char* text, int xo, int yo, int xe, int ye, const uint colour)
{
    int n = strlen(text);
    for(int i = 0; i < n; i++)
    {
        draw_glyph(canvas, font, text[i] - 32, xo + i*8, yo, colour);
    }
}

void draw_scope(frame* canvas, wavering* data, int xo, int yo, int xe, int ye, uint bg_colour, uint fg_colour, float scale)
{
    int w = xe - xo;
    int h = (ye - yo)/2 + yo;

    draw_rectangle_filled(canvas, xo, yo, xe, ye, bg_colour);
    for(int i = xo; i <= xe; i++)
    {
        frame_pset(canvas, i, wavering_get(data) * scale + h, fg_colour);
    }
}

// /////////////////////////////////////////////////////////////////////////////////////
// // https://github.com/miloyip/line/blob/master/line_sdfaabb.c ///////////////////////
// // Signed distnace field (SDF) optimization with AABB ///////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////

// float capsuleSDF(float px, float py, float ax, float ay, float bx, float by, float r) 
// {
//     float pax = px - ax; 
//     float pay = py - ay;
//     float bax = bx - ax;
//     float bay = by - ay;
//     float h   = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
//     float dx  = pax - bax * h;
//     float dy  = pay - bay * h;
//     return sqrtf(dx * dx + dy * dy) - r;
// }


// void drawLineSDFAABB(frame *canvas, float xo, float yo, float xe, float ye, float r, uint colour) 
// {
//     int xa = (int)floorf(fminf(xo, xe) - r);
//     int xb = (int) ceilf(fmaxf(xo, xe) + r);
//     int ya = (int)floorf(fminf(yo, ye) - r);
//     int yb = (int) ceilf(fmaxf(yo, ye) + r);
//     for (int y = ya; y <= yb; y++)
//         for (int x = xa; x <= xb; x++)
//         PSetAlpha(canvas, x, y, DIM_ALPHA(colour, fmaxf(fminf(0.5f - capsuleSDF(x, y, xo, yo, xe, ye, r), 1.0f), 0.0f)));
// }


///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
