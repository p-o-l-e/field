#include "curves.h"
#include <math.h>

point interpolate_bezier(point a, point b, point c, point d, float t)
{
    point o;
    o.x = pow(1 - t, 3) * a.x + 3 * t * pow(1 - t, 2) * b.x + 3 * t * t * (1 - t) * c.x + pow(t, 3) * d.x;
    o.y = pow(1 - t, 3) * a.y + 3 * t * pow(1 - t, 2) * b.y + 3 * t * t * (1 - t) * c.y + pow(t, 3) * d.y;
    return o;
}
