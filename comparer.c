#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"

typedef struct {
    pixbuf_t *other;
    uint64_t count;
} comp_data_t;

bool compare(pixel_t *pixel, uint32_t x, uint32_t y, void *userdata) {
    comp_data_t *data = userdata;
    pixel_t *other = pixel_at(data->other, x, y);
    if (pixel->red == other->red && pixel->green == other->green && pixel->blue == other->blue) {
        return false;
    }
    if (data->count == 0) {
        fprintf(stderr, "Next pixels are different:\n");
    }
    fprintf(stderr, "x%-6d y%-6d\n", x, y);
    data->count++;
    if (data->count == 100) {
        return true;
    }
    return false;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Only two files are required\n");
        return 1;
    }
    file_header_t fhdr;
    info_header_t ihdr;
    color_table_t *table;
    pixbuf_t *pixbuf1 = load_from_bmp(argv[1], &fhdr, &ihdr, &table);
    pixbuf_t *pixbuf2 = load_from_bmp(argv[2], &fhdr, &ihdr, &table);
    if (pixbuf1 == NULL || pixbuf2 == NULL) {
        free(pixbuf1);
        free(pixbuf2);
        return 1;
    }
    if (pixbuf1->width != pixbuf2->width || pixbuf1->height != pixbuf2->height) {
        fprintf(stderr, "Dimensions differ: %s is %ux%u while %s is %ux%u\n", argv[1], pixbuf1->width, pixbuf1->height, argv[2], pixbuf2->width, pixbuf2->height);
        return 1;
    }
    comp_data_t data = {pixbuf2, 0};
    for_each_pixel(pixbuf1, compare, &data);
    if (data.count == 0) printf("These pictures are equal\n");
    free(pixbuf1);
    free(pixbuf2);
    return 0;
}
