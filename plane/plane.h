#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct PlaneHeader PlaneHeader;

#pragma pack(push, 1)
struct PlaneHeader {
    uint8_t  bpc;
    uint8_t  channels;
    uint32_t width;
    uint32_t height;
    char     order[4];
};
#pragma pack(pop)

static inline int plane_load(const char *path, uint8_t **data, PlaneHeader *header)
{
    if(!path || !data || !header) return -1;

    FILE *f = fopen(path, "rb");
    if(!f) return -1;

    PlaneHeader h;
    if(fread(&h, 1, sizeof(h), f) != sizeof(h)) {
        fclose(f);
        return -1;
    }

    if(h.bpc == 0 || h.channels == 0 || h.width == 0 || h.height == 0) {
        fclose(f);
        return -1;
    }

    size_t pixel_count = (size_t)h.width * h.height;
    size_t data_size = pixel_count * h.channels * h.bpc;

    uint8_t *raw = (uint8_t *)malloc(data_size);
    if(!raw) {
        fclose(f);
        return -1;
    }

    if(fread(raw, 1, data_size, f) != data_size) {
        free(raw);
        fclose(f);
        return -1;
    }

    fclose(f);

    *data = raw;
    *header = h;
    return 0;
}

static inline int plane_save(const char *path, const uint8_t *data, const PlaneHeader *header)
{
    if(!path || !data || !header) {
        return -1;
    }

    if(header->bpc == 0 || header->channels == 0 ||
        header->width == 0 || header->height == 0) {
        return -1;
    }

    FILE *f = fopen(path, "wb");
    if(!f) {
        return -1;
    }

    if(fwrite(header, 1, sizeof(*header), f) != sizeof(*header)) {
        fclose(f);
        return -1;
    }

    size_t pixel_count = (size_t)header->width * header->height;
    size_t data_size = pixel_count * header->channels * header->bpc;

    if(fwrite(data, 1, data_size, f) != data_size) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}
