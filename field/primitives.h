#pragma once

#include <stdint.h>
#include "fonts.h"
#include "containers.h"

point32u uv_to_screen(float, float, uint32_t, uint32_t);
point screen_to_uv(uint32_t, uint32_t, uint32_t, uint32_t);

void draw_glyph(frame*, const uint8_t*, uint32_t, uint32_t, uint32_t, const uint32_t);
void draw_text_label(frame*, const uint8_t* , const char*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
void draw_rectangle(frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
void draw_rectangle_filled(frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);

