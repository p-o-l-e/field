#include "curves.h"

point interpolate_bezier(point a, point b, point c, point d, float t)
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
