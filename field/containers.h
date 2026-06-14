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
    uint32_t i;
    uint32_t o;

} wavering;

void  wavering_init(wavering*);
void  wavering_set (wavering*, float);
float wavering_get (wavering*);

/******************************************************************************************************************************/

typedef struct
{
    uint32_t* data;
    uint32_t width;
    uint32_t height;             

} frame;

uint32_t frame_get  (frame*, uint32_t, uint32_t);
void frame_set      (frame*, uint32_t, uint32_t, uint32_t);
void frame_fill     (frame*, uint32_t);
void frame_clr      (frame*);
void frame_init     (frame*, uint32_t, uint32_t);
void frame_flush    (frame*);
void frame_copy     (frame*, frame*);
void frame_copy_at  (frame*, frame*, uint32_t, uint32_t);

/******************************************************************************************************************************/

typedef struct
{
    frame*   data;
    uint32_t width;
    uint32_t height;  
    uint32_t nframes;           

} sprite;

void sprite_init (sprite*, uint32_t, uint32_t, uint32_t);
void sprite_flush(sprite*);
void sprite_load_stripe(sprite*, frame*);

