#pragma once
#include <stdint.h>
#include <stdio.h>
#include "containers.h"
#include "upng/upng.h"

void load_png_rgba(const char* path, frame *o, uint32_t xo, uint32_t yo);
