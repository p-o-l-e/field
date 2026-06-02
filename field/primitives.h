#pragma once

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "fonts.h"
#include "containers.h"

void draw_glyph(frame* canvas, const uint8_t* font, uint32_t id, uint32_t xo, uint32_t yo, const uint32_t colour);
void draw_text_label(frame* canvas, const uint8_t* font, const char* text, uint32_t xo, uint32_t yo, uint32_t xe, uint32_t ye, const uint32_t colour);
void draw_rectangle(frame* canvas, uint32_t xo, uint32_t yo, uint32_t xe, uint32_t ye, const uint32_t colour);
void draw_rectangle_filled(frame* canvas, uint32_t xo, uint32_t yo, uint32_t xe, uint32_t ye, const uint32_t colour);
