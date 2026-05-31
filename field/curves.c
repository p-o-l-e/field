#include "curves.h"

point interpolate_bezier(point pa, point pb, point pc, point pd, float t)
{
    point o;
    o.x = pow(1-t, 3) * pa.x + 3*t*pow(1-t,2) * pb.x + 3*t*t*(1-t) * pc.x + pow(t,3) * pd.x;
    o.y = pow(1-t, 3) * pa.y + 3*t*pow(1-t,2) * pb.y + 3*t*t*(1-t) * pc.y + pow(t,3) * pd.y;
    return o;
}