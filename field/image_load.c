#include "image_load.h"


void load_png_rgba(const char* path, frame *o, int xo, int yo)
{
    upng_t* upng = upng_new_from_file(path);
    upng_decode(upng);
	if (upng_get_error(upng) != UPNG_EOK) {
		printf("error: %u %u\n", upng_get_error(upng), upng_get_error_line(upng));
		return 0;
	}

    auto w = upng_get_width(upng);
    auto h = upng_get_height(upng);
    unsigned char* data = upng_get_buffer(upng);

    int channels = 4;



    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            uint colour = 0;
            for(int c = 0; c < channels; c++)
            {
                colour<<=8;
                colour += data[(x + y*w)*channels + c];
            }
            frame_pset(o, x + xo, y + yo, colour);
        }
    }
}
