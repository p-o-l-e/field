#pragma once
#include <stdlib.h>
#include <stdint.h>
#ifndef WAVERING_LENGTH
#define WAVERING_LENGTH 1024
#endif

/******************************************************************************************************************************/

typedef struct 
{
    uint32_t l;
    uint32_t t;
    uint32_t r;
    uint32_t b;

} ltrb32u;

typedef struct 
{
    int32_t l;
    int32_t t;
    int32_t r;
    int32_t b;

} ltrb32s;

typedef struct 
{
    uint64_t l;
    uint64_t t;
    uint64_t r;
    uint64_t b;

} ltrb64u;

/******************************************************************************************************************************/

typedef struct 
{
    float x;
    float y;
    
} point;

typedef struct 
{
    uint32_t x;
    uint32_t y;
    
} point32u;

typedef struct 
{
    int32_t x;
    int32_t y;
    
} point32s;

/******************************************************************************************************************************/

typedef struct
{
    float data[WAVERING_LENGTH];
    uint32_t i;             // In
    uint32_t o;             // Out

} wavering;

void  wavering_init(wavering* o);
void  wavering_set (wavering* o, float value);
float wavering_get (wavering* o);

/******************************************************************************************************************************/

typedef struct
{
    uint32_t* data;
    uint32_t width;
    uint32_t height;             

} frame;

uint32_t frame_get  (frame* o, uint32_t x, uint32_t y);
void frame_pset     (frame* o, uint32_t x, uint32_t y, uint32_t value);
void frame_clr      (frame* o, uint32_t value);
void frame_init     (frame* o, uint32_t x, uint32_t y);
void frame_flush    (frame* o);
void frame_copy     (frame* target, frame* source);
void frame_copy_at  (frame* target, frame* source, uint32_t x, uint32_t y);

/******************************************************************************************************************************/

typedef struct
{
    frame*   data;
    uint32_t width;
    uint32_t height;  
    uint32_t nframes;           

} sprite;

void sprite_init (sprite* o, uint32_t w, uint32_t h, uint32_t nframes);
void sprite_flush(sprite* o);
void sprite_load_stripe(sprite* o, frame* f);

