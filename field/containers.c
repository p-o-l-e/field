////////////////////////////////////////////////////////////////////////////////////////
// Containers
// V.0.1.2 2022-06-20 (C) Unmanned
////////////////////////////////////////////////////////////////////////////////////////
#include "containers.h"

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void frame_pset(frame* o, unsigned x, unsigned y, _ftype value)
{
    if(((x >= 0) && (x < o->width)) && ((y >= 0) && (y < o->height)))
    o->data[x + y * o->width] = value;
}

_ftype frame_get(frame* o, unsigned x, unsigned y)
{
    if(((x >= 0) && (x < o->width)) && ((y >= 0) && (y < o->height))) return o->data[x + y * o->width];
    return o->data[0];
}

void frame_clr(frame* o, _ftype value)
{
    for(unsigned i = 0; i < (o->height * o->width); ++i) o->data[i] = value;
}

void frame_init(frame* o, unsigned x, unsigned y)
{
    o->width  = x;
    o->height = y;
    o->data   = (_ftype*)calloc(o->width * o->height , sizeof(_ftype));
}

void frame_flush(frame* o)
{
    free(o->data);
}

void frame_copy (frame* dest, frame* src)
{
    for(int i = 0; i < dest->height * dest->width; i++) dest->data[i] = src->data[i];
}

void frame_copy_at(frame* dest, frame* src, unsigned xo, unsigned yo)
{
    for(int y = 0; y < src->height; y++)
    {
        for(int x = 0; x < src->width; x++)
        {
            frame_pset(dest, x + xo, y + yo, frame_get(src, x, y));
        }
    }
}

/******************************************************************************************************************************/

void sprite_init(sprite* o, unsigned w, unsigned h, unsigned nframes)
{
    frame temp;
    frame_init(&temp, w, h);
    o->data = (sprite*)malloc(nframes * sizeof(temp));
    o->width = w;
    o->height = h;
    o->nframes = nframes;
    frame_flush(&temp);
    for(int i = 0; i < o->nframes; i++)
    {
        frame_init(&o->data[i], w, h);
    }
}

void sprite_flush(sprite* o)
{
    for(int i = 0; i < o->nframes; i++)
    {
        frame_flush(&o->data[i]);
    }
    free(o->data);
}

void sprite_load_stripe(sprite* o, frame* f)
{
    for(int i = 0; i < o->nframes; i++)
    {
        for(int y = 0; y < o->height; y++)
        {
            for(int x = 0; x < o->width; x++)
            {
                frame_pset(&o->data[i], x, y, frame_get(f, x, y + (i * o->height)));
            }
        }
    }
}
