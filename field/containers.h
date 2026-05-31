#pragma once
#include <stdlib.h>
#include <stdint.h>
#ifndef WAVERING_LENGTH
#define WAVERING_LENGTH 1024
#endif


/******************************************************************************************************************************/

typedef struct 
{
    uint64_t l;
    uint64_t t;
    uint64_t r;
    uint64_t b;

} ltrb32;

typedef struct 
{
    uint64_t l;
    uint64_t t;
    uint64_t r;
    uint64_t b;

} ltrb64;

/******************************************************************************************************************************/

typedef struct 
{
    float x;
    float y;
    
} point;


/******************************************************************************************************************************/

typedef struct
{
    float data[WAVERING_LENGTH];
    unsigned  i; // Write Pointer
    unsigned  o; // Read Pointer

} wavering;

void  wavering_init(wavering* o);
void  wavering_set (wavering* o, float value);
float wavering_get (wavering* o);


/******************************************************************************************************************************/

#define _ftype uint32_t

typedef struct
{
    _ftype* data;
    unsigned width;
    unsigned height;             

} frame;

_ftype frame_get    (frame* o, unsigned x, unsigned y);
void frame_pset     (frame* o, unsigned x, unsigned y, _ftype value);
void frame_clr      (frame* o, _ftype value);
void frame_init     (frame* o, unsigned x, unsigned y);
void frame_flush    (frame* o);
void frame_copy     (frame* dest, frame* src);
void frame_copy_at  (frame* dest, frame* src, unsigned xo, unsigned yo);



/******************************************************************************************************************************/

typedef struct
{
    frame*   data;
    unsigned width;
    unsigned height;  
    unsigned nframes;           

} sprite;

void sprite_init (sprite* o, unsigned w, unsigned h, unsigned nframes);
void sprite_flush(sprite* o);
void sprite_load_stripe(sprite* o, frame* f);

