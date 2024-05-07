#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "bmp.h"

static inline uintptr_t get_row_size(uint32_t bpp, uint32_t width) {
    return ceil(bpp * width / 32.0) * 4;
}

pixbuf_t *make_pixbuf(uint32_t width, uint32_t height) {
    return calloc(2 * sizeof(uint32_t) + width * height * sizeof(pixel_t), 1);
}

int read_headers(FILE* file, file_header_t *fhdr, info_header_t *ihdr) {
    fread(fhdr, sizeof(file_header_t), 1, file);
    if (memcmp(fhdr->signature, "BM", 2) != 0) {
        fprintf(stderr, "The file is not a BMP file.\n");
        return 1;
    }
    fread(ihdr, sizeof(info_header_t), 1, file);
    if (ihdr->size != 40) {
        fprintf(stderr, "Only BITMAPINFOHEADER is supported.\n");
        return 1;
    }
    if (ihdr->bpp != 24 && ihdr->bpp != 8) {
        fprintf(stderr, "Bpp of %u is not supported, only 8 and 24 are.\n", ihdr->bpp);
        return 1;
    }
#ifdef DEBUG
    printf("Signature: %c%c\n", fhdr->signature[0], fhdr->signature[1]);
    printf("File size: %u\n", fhdr->file_size);
    printf("Raster data offset: %u\n", fhdr->raster_data);

    printf("Header size: %u\n", ihdr->size);
    printf("Compression: %u\n", ihdr->compression);

    printf("Image size: %ux%u (%u)\n", ihdr->width, ihdr->height, ihdr->image_size);
    printf("Bits per pixel: %u\n", ihdr->bpp);
    printf("Colors used: %u\n", ihdr->colors_used);
    printf("PPM: %ux%u\n", ihdr->xppm, ihdr->yppm);
#endif
    return 0;
}

color_table_t *read_table(FILE *file, info_header_t *ihdr) {
    uint32_t num_colors = 0;
    if (ihdr->bpp <= 8) {
        num_colors = ihdr->colors_used;
        if (num_colors == 0) {
            num_colors = 1 << ihdr->bpp;
        }
    }
    color_table_t *table = malloc(sizeof(uint32_t) + num_colors * sizeof(padded_color_t));
    table->size = num_colors;
    fread(table->colors, sizeof(padded_color_t), num_colors, file);
    return table;
}

pixbuf_t *read_pixbuf(FILE *file, info_header_t *ihdr, color_table_t *table) {
    uint32_t row_size = get_row_size(ihdr->bpp, ihdr->width);
    uint8_t row[row_size];
    pixbuf_t *buffer = make_pixbuf(ihdr->width, ihdr->height);
    *buffer = (pixbuf_t){ihdr->width, ihdr->height};
    for (size_t y = 0; y < ihdr->height; y++) {
        size_t buf_y = ihdr->height - y - 1;
        fread(&row, sizeof(uint8_t), row_size, file);
        if (ihdr->bpp == 8) {
            for (size_t x = 0; x < ihdr->width; x++) {
                *pixel_at(buffer, x, buf_y) = table->colors[row[x]].color;
            }
        } else {
            memcpy(&buffer->pixels[buf_y * ihdr->width], &row, sizeof(pixel_t) * ihdr->width);
        }
    }
    return buffer;
}

pixbuf_t *load_from_bmp(const char *filepath, file_header_t *fhdr, info_header_t *ihdr, color_table_t **table) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("File error");
        return NULL;
    }

    file_header_t local_fhdr;
    info_header_t local_ihdr;
    if (fhdr == NULL) {
        fhdr = &local_fhdr;
    }
    if (ihdr == NULL) {
        ihdr = &local_ihdr;
    }
    if (read_headers(file, fhdr, ihdr)) {
        fclose(file);
        return NULL;
    }

    if (ihdr->bpp == 8) {
        *table = read_table(file, ihdr);
    }

    pixbuf_t *buffer = read_pixbuf(file, ihdr, *table);
    fclose(file);
    return buffer;
}

int save_to_bmp24bit(const char *filepath, file_header_t *fhdr, info_header_t *ihdr, pixbuf_t *pixbuf) {
    FILE *file = fopen(filepath, "w");
    if (file == NULL) {
        perror("File error");
        return 1;
    }
    file_header_t local_fhdr = {
        .signature = {'B', 'M'}, 
        .raster_data = 54
    };
    info_header_t local_ihdr = {
        .width = pixbuf->width,
        .height = pixbuf->height,
        .size = 40,
        .bpp = 24,
        .planes = 1
    };
    if (ihdr == NULL) {
        ihdr = &local_ihdr;
    }
    uint32_t row_size = get_row_size(ihdr->bpp, pixbuf->width);
    if (fhdr == NULL) {
        local_fhdr.file_size = local_fhdr.raster_data + row_size * pixbuf->height;
        fhdr = &local_fhdr;
    }
    uint32_t pixel_size = sizeof(pixel_t);
    uint8_t padding[row_size - pixbuf->width * pixel_size];
    memset(&padding, 0, row_size - pixbuf->width * pixel_size);
    fwrite(fhdr, sizeof(file_header_t), 1, file);
    fwrite(ihdr, sizeof(info_header_t), 1, file);
    for (size_t y = 0; y < pixbuf->height; y++) {
        fwrite(&pixbuf->pixels[(pixbuf->height - y - 1) * pixbuf->width], sizeof(pixel_t), pixbuf->width, file);
        fwrite(&padding, sizeof(uint8_t), row_size - pixbuf->width * pixel_size, file);
    }
    fclose(file);
    return 0;
}

void for_each_pixel(pixbuf_t *pixbuf, bool (*action)(pixel_t *, uint32_t, uint32_t, void *), void *userdata) {
    for (size_t y = 0; y < pixbuf->height; y++) {
        for (size_t x = 0; x < pixbuf->width; x++) {
            if (action(pixel_at(pixbuf, x, y), x, y, userdata)) {
                return;
            }
        }
    }
}
