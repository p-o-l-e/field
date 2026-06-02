#pragma once
#include <math.h>
#include "containers.h"

point interpolate_bezier(point a, point b, point c, point d, float t);
