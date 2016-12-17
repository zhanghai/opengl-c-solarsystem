/*
 * Copyright (c) 2016 Zhang Hai <Dreaming.in.Code.ZH@Gmail.com>
 * All Rights Reserved.
 */

#include "tga.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Data type 2: Unmapped RGB.
static GLubyte TGA_TYPE_HEADER[8] = {0, 0, 2, 0, 0, 0, 0, 0};

bool load_tga(const char *path, texture_t *texture) {

    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("fopen");
        return false;
    }

    GLubyte type_header[8];
    if (fread(type_header, 1, sizeof(type_header), file) != sizeof(type_header)) {
        perror("fread: TGA header type specification");
        fclose(file);
        return false;
    }

    if (memcmp(TGA_TYPE_HEADER, type_header, sizeof(type_header)) != 0) {
        fprintf(stderr, "load_tga: Unsupported TGA type:");
        for (size_t i = 0; i < sizeof(type_header) / sizeof(type_header[0]); ++i) {
            fprintf(stderr, " %02X", type_header[i]);
        }
        fprintf(stderr, "\n");
        fclose(file);
        return false;
    }

    GLubyte image_header[10];
    if (fread(image_header, 1, sizeof(image_header), file) != sizeof(image_header)) {
        perror("fread: TGA header image specification");
        fclose(file);
        return false;
    }

    texture->width = image_header[5] * 256u + image_header[4];
    texture->height = image_header[7] * 256u + image_header[6];

    if ((texture->width <= 0) || (texture->height <= 0)) {
        fprintf(stderr, "load_tga: Invalid image size\n");
        fclose(file);
        return false;
    }

    GLubyte depth = image_header[8];
    switch (depth) {
        case 24:
            texture->format = GL_RGB;
            break;
        case 32:
            texture->format = GL_RGBA;
            break;
        default:
            fprintf(stderr, "load_tga: Invalid color depth\n");
            fclose(file);
            return false;
    }

    size_t pixel_size = depth / 8u;
    size_t data_size = texture->width * texture->height * pixel_size;
    texture->data  = malloc(data_size);
    if (!texture->data) {
        perror("malloc");
        fclose(file);
        return false;
    }

    if (fread(texture->data, 1, data_size, file) != data_size) {
        perror("fread: TGA image data");
        free(texture->data);
        fclose(file);
        return false;
    }

    // TGA stores image data as BGR(A).
    for (size_t i = 0; i < data_size; i += pixel_size) {
        // See https://en.wikipedia.org/wiki/XOR_swap_algorithm#Reasons_for_avoidance_in_practice .
        GLubyte temp = texture->data[i];
        texture->data[i] = texture->data[i + 2];
        texture->data[i + 2] = temp;
    }

    fclose(file);
    return true;
}
