/////////////////////////////////////////////////////////////////////////////////////
// https://github.com/miloyip/line/blob/master/line_sdfaabb.c ///////////////////////
// Signed distnace field (SDF) optimization with AABB ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <math.h>  
#include "containers.h"

float capsule_sdf(float px, float py, float ax, float ay, float bx, float by, float r);
void alphablend(frame* canvas, int x, int y, float alpha);
inline void lineSDFAABB(frame* canvas, float ax, float ay, float bx, float by, float radius, float alpha);
