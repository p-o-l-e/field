////////////////////////////////////////////////////////////////////////////////////////
// MIT License
// Copyright (c) 2022 unmanned
////////////////////////////////////////////////////////////////////////////////////////
#include "blur.h"
////////////////////////////////////////////////////////////////////////////////////////
void blur_h(Frame* data, uint scale)
{
    int range = scale * 2 + 1;
    for(uint y = 0; y < data->height; y++)
    {
        for(uint x = 0; x < data->width-range; x++)
        {
            float car = 0.0f;
            for(int i = 0; i < range; i++)
            {   
                car += frame_get(data, x + i, y);
            }
            car /= (float)range;
            frame_pset(data, x + scale, y, car);
        }
    }
}

void blur_v(Frame* data, uint scale)
{
    int range = scale * 2 + 1;  
    for(uint x = 0; x < data->width; x++)
    {
        for(uint y = 0; y < data->height-range; y++)
        {
            float car = 0.0f;
            for(int i = 0; i < range; i++)
            {   
                car += frame_get(data, x, y + i);
            }
            car /= (float)range;
            frame_pset(data, x, y + scale, car);
        }
    }
}

void blur_box(frame* data, uint scale)
{
    int range = scale * 2 + 1;
    for(uint y = 0; y < data->height; y++)
    {
        for(uint x = 0; x < data->width-range; x++)
        {
            float car = 0.0f;
            for(int i = 0; i < range; i++)
            {   
                car += frame_get(data, x + i, y);
            }
            car /= (float)(range);
            frame_pset(data, x + scale, y, car);
        }
    }
    
    for(uint x = 0; x < data->width; x++)
    {
        for(uint y = 0; y < data->height-range; y++)
        {
            float car = 0.0f;
            for(int i = 0; i < range; i++)
            {   
                car += frame_get(data, x, y + i);
            }
            car /= (float)(range);
            frame_pset(data, x, y + scale, car);
        }
    }
}
