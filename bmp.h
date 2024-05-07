#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* 
 * the file_header_t and info_header_t structs match the physical layout
 * of a BMP header, so that header can be easily read and written
 * __attribute__((packed)) means that struct is packed (internal alignment is disabled)
 */

// represents BITMAPFILEHEADER
typedef struct {
    uint8_t signature[2];
    uint32_t file_size;
    uint32_t reserved;
    uint32_t raster_data;
} __attribute__((packed)) file_header_t;

// represents BITMAPINFOHEADER
typedef struct {
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t image_size;
    uint32_t xppm;
    uint32_t yppm;
    uint32_t colors_used;
    uint32_t colors_important;
} __attribute__((packed)) info_header_t;

// In file, the order is BGR and not RGB
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} pixel_t;


// Color table has the reserved byte for each color
typedef struct {
    pixel_t color;
    uint8_t reserved;
} padded_color_t;

typedef struct {
    uint32_t size;
    padded_color_t colors[];
} color_table_t;

// Raw pixel data
typedef struct {
    uint32_t width;
    uint32_t height;
    pixel_t pixels[];
} pixbuf_t;


static inline pixel_t *pixel_at(pixbuf_t *image, uintptr_t x, uintptr_t y) {
    return &image->pixels[y * image->width + x];
}

pixbuf_t *make_pixbuf(uint32_t width, uint32_t height);
int read_headers(FILE* file, file_header_t *fhdr, info_header_t *ihdr);
color_table_t *read_table(FILE *file, info_header_t *ihdr);
pixbuf_t *read_pixbuf(FILE *file, info_header_t *ihdr, color_table_t *table);
pixbuf_t *load_from_bmp(const char *filepath, file_header_t *fhdr, info_header_t *ihdr, color_table_t **table);
// Saves raw pixbuf as 24bit BMP
int save_to_bmp24bit(const char *filepath, file_header_t *fhdr, info_header_t *ihdr, pixbuf_t *pixbuf);
/**
 * @param action - a function which has the following signature
 *      bool (pixel_t *current_pixel, uint32_t x, uint32_t y, void *userdata)
 *
 * @param userdata - can be anything user passes and it will be passed to each action call
 */
void for_each_pixel(pixbuf_t *pixbuf, bool (*action)(pixel_t *, uint32_t, uint32_t, void *), void *userdata);
