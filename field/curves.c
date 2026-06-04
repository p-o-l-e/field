#include "curves.h"

point interpolate_bezier(point a, point b, point c, point d, float t)
{
    /*
    float ti = 1.0f - t;
    float t2 = t * t;
    float mt2 = ti * ti;
    
    float fa = mt2 * ti;           // (1-t)^3
    float fb = 3.0f * t * mt2;     // 3t(1-t)^2
    float fc = 3.0f * t2 * ti;     // 3t^2(1-t)
    float fd = t2 * t;             // t^3
    
    point o = {
        .x = fa * a.x + fb * b.x + fc * c.x + fd * d.x,
        .y = fa * a.y + fb * b.y + fc * c.y + fd * d.y
    };*/


    
    float mt = 1.0f - t;
    //De Casteljau's Algorithm 
    // Level 1 interpolation
    point p01 = { .x = mt * a.x + t * b.x, .y = mt * a.y + t * b.y };
    point p12 = { .x = mt * b.x + t * c.x, .y = mt * b.y + t * c.y };
    point p23 = { .x = mt * c.x + t * d.x, .y = mt * c.y + t * d.y };
    
    // Level 2 interpolation
    point p012 = { .x = mt * p01.x + t * p12.x, .y = mt * p01.y + t * p12.y };
    point p123 = { .x = mt * p12.x + t * p23.x, .y = mt * p12.y + t * p23.y };
    
    // Level 3 (final point)
    point p = { .x = mt * p012.x + t * p123.x, .y = mt * p012.y + t * p123.y };
    
    return p;
 
}
