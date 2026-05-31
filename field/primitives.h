#pragma once

#include <math.h>
#include <string.h>
#include "fonts.h"
#include "containers.h"


void draw_glyph(frame* canvas, uint8_t* font, int id, int xo, int yo, const uint colour);
void draw_text_label(frame* canvas, char* font, char* text, int xo, int yo, int xe, int ye, const uint colour);
void draw_rectangle(frame* canvas, int xo, int yo, int xe, int ye, const uint colour);
void draw_rectangle_filled(frame* canvas, int xo, int yo, int xe, int ye, const uint colour);
void draw_scope(frame* canvas, wavering* data, int xo, int yo, int xe, int ye, uint bg_colour, uint fg_colour, float scale);
