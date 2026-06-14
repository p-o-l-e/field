#include "image_load.h"

void load_png_rgba(const char* path, frame *o)
{
    upng_t* upng = upng_new_from_file(path);
    upng_decode(upng);

	if (upng_get_error(upng) != UPNG_EOK) {
		printf("error: %u %u\n", upng_get_error(upng), upng_get_error_line(upng));
		return;
	}

    auto w = upng_get_width(upng);
    auto h = upng_get_height(upng);

    auto data = upng_get_buffer(upng);

    const uint32_t channels = 4;

    for(uint32_t y = 0; y < h; y++)
    {
        for(uint32_t x = 0; x < w; x++)
        {
            uint32_t colour = 0;

            for(uint32_t c = 0; c < channels; c++)
            {
                colour <<= 8;
                colour += data[(x + y * w) * channels + c];
            }

            frame_set(o, x, y, colour);
        }
    }
}
