#include "containers.h"
#include <stdint.h>
#include <string.h>

/******************************************************************************************************************************/

void wavering_init(wavering* o)
{
    o->i = 0;
    o->o = 0;
}

void wavering_set(wavering* o, float value)
{
    o->i++;
    if (o->i >= WAVERING_LENGTH) o->i = 0;
    o->data[o->i] = value;
}

float wavering_get(wavering* o)
{
    o->o++;
    if (o->o >= WAVERING_LENGTH) o->o = 0;
    return o->data[o->o];
}

/******************************************************************************************************************************/

void frame_set(frame* o, uint32_t x, uint32_t y, uint32_t value)
{
    if(x < o->width && y < o->height)
    o->data[x + y * o->width] = value;
}

uint32_t frame_get(frame* o, uint32_t x, uint32_t y)
{
    if(x < o->width && y < o->height) return o->data[x + y * o->width];
    return o->data[0];
}

void frame_clr(frame* o)
{
    memset(o->data, 0, o->height * o->width * sizeof(uint32_t));
}

void frame_fill(frame* o, uint32_t value)
{
    for(uint32_t i = 0; i < (o->height * o->width); ++i) o->data[i] = value;
}

void frame_init(frame* o, uint32_t x, uint32_t y)
{
    o->width  = x;
    o->height = y;
    o->data   = (uint32_t*)calloc(o->width * o->height , sizeof(uint32_t));
}

void frame_flush(frame* o)
{
    free(o->data);
}

void frame_copy (frame* target, frame* source)
{
    for(uint32_t i = 0; i < target->height * target->width; i++) target->data[i] = source->data[i];
}

void frame_copy_at(frame* target, frame* source, uint32_t xo, uint32_t yo)
{
    for(uint32_t y = 0; y < source->height; y++)
    {
        for(uint32_t x = 0; x < source->width; x++)
        {
            frame_set(target, x + xo, y + yo, frame_get(source, x, y));
        }
    }
}

/******************************************************************************************************************************/

void sprite_init(sprite* o, uint32_t w, uint32_t h, uint32_t nframes)
{
    frame temp;
    frame_init(&temp, w, h);

    o->data = (frame*)malloc(nframes * sizeof(temp));
    o->width = w;
    o->height = h;
    o->nframes = nframes;

    frame_flush(&temp);
    
    for(uint32_t i = 0; i < o->nframes; i++)
    {
        frame_init(&o->data[i], w, h);
    }
}

void sprite_flush(sprite* o)
{
    for(uint32_t i = 0; i < o->nframes; i++)
    {
        frame_flush(&o->data[i]);
    }

    free(o->data);
}

void sprite_load_stripe(sprite* o, frame* f)
{
    for(uint32_t i = 0; i < o->nframes; i++)
    {
        for(uint32_t y = 0; y < o->height; y++)
        {
            for(uint32_t x = 0; x < o->width; x++)
            {
                frame_set(&o->data[i], x, y, frame_get(f, x, y + (i * o->height)));
            }
        }
    }
}
