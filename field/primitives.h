#pragma once

#include <stdint.h>
#include "fonts.h"
#include "containers.h"

typedef enum { PP = 0, PT = 8, MP = 16, MT = 24 } shift;

typedef struct 
{
    uint32_t mt: 8;
    uint32_t mp: 8;
    uint32_t pt: 8;
    uint32_t pp: 8;

} uid32;

uint8_t extract_byte(uint32_t, uint_fast8_t);
point32u uv_to_screen(float, float, uint32_t, uint32_t);
point screen_to_uv(uint32_t, uint32_t, uint32_t, uint32_t);

uint32_t encode_uid(uint8_t, uint8_t, uint8_t, uint8_t);
uid32 decode_uid(uint32_t);

void draw_glyph(frame*, const uint8_t*, uint32_t, uint32_t, uint32_t, const uint32_t);
void draw_text_label(frame*, const uint8_t* , const char*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
void draw_rectangle(frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
void draw_ltrb_o(frame*, ltrb32u*, const uint32_t);
void draw_rect_f(frame*, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t);
void draw_ltrb_f(frame*, ltrb32u*, const uint32_t);
