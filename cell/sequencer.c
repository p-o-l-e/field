#include "sequencer.h"


void init_sequence(sequencer* o, int l)
{
    o->departed = 0;
    o->current  = 0;
    o->length   = l;
}

void loop_forward(sequencer* o)
{
    o->departed++;
    if(o->departed >= o->length) 
    {
        o->current++;
        if(o->current >= _steps_) o->current = 0;
        o->departed = 0;
    }
}

void loop_backward(sequencer* o)
{
    o->departed++;
    if(o->departed >= o->length) 
    {
        if(o->current > 0)
        o->current--;
        else o->current = _steps_ - 1;
        o->departed = 0;
    }
}

void loop_pingpong(sequencer* o)
{
    static bool f;
    o->departed++;
    if(o->departed >= o->length) 
    {
        if(f)
        {
            o->current++;
            if(o->current >= _steps_) 
            {
                o->current = _steps_ - 2;
                f = false;
            }
        }
        else
        {
            if(o->current > 0)
            o->current--;
            else 
            {
                f = true;
                o->current = 1;
            }
        }
        o->departed = 0;
    }
}

void loop_random(sequencer* o)
{
    o->departed++;
    if(o->departed >= o->length) 
    {
        o->current  = 0;//rand_in_range(0, _steps_ - 1);
        o->departed = 0;
    }
}

