/////////////////////////////////////////////////////////////////////////////////////
// https://github.com/miloyip/line/blob/master/line_sdfaabb.c ///////////////////////
// Signed distnace field (SDF) optimization with AABB ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

#include "sdfaabb.h"
#include <math.h>  

float capsule_sdf(float px, float py, float ax, float ay, float bx, float by, float r) 
{
    float pax = px - ax, pay = py - ay, bax = bx - ax, bay = by - ay;
    float h = fmaxf(fminf((pax * bax + pay * bay) / (bax * bax + bay * bay), 1.0f), 0.0f);
    float dx = pax - bax * h, dy = pay - bay * h;
    return sqrtf(dx * dx + dy * dy) - r;
}

void alphablend(frame* canvas, int x, int y, float alpha) 
{   
    frame_pset(canvas, x, y, 0x000000ff * (1.0f - alpha) + alpha);
}

void lineSDFAABB(frame* canvas, float ax, float ay, float bx, float by, float radius, float alpha) 
{
    int xo = (int)floorf(fminf(ax, bx) - radius);
    int xe = (int) ceilf(fmaxf(ax, bx) + radius);
    int yo = (int)floorf(fminf(ay, by) - radius);
    int ye = (int) ceilf(fmaxf(ay, by) + radius);
    for (int y = yo; y <= ye; y++)
        for (int x = xo; x <= xe; x++)
            alphablend(canvas, x, y, fmaxf(fminf(0.5f - capsule_sdf(x, y, ax, ay, bx, by, radius), 1.0f), alpha));
}
