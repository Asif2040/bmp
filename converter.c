#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"

void negate(pixel_t *pixel) {
    *pixel = (pixel_t){255 - pixel->blue, 255 - pixel->green, 255 - pixel->red};
}

bool negate_adapter(pixel_t *pixel) {
    negate(pixel);
    return false;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Only input and output files are required\n");
        return 1;
    }
    file_header_t fhdr;
    info_header_t ihdr;
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("File error");
        return 1;
    }
    if (read_headers(file, &fhdr, &ihdr) == 1) { 
        fclose(file);
        return 1;
    }
    else {
        if (ihdr.bpp == 24) {
            pixbuf_t *pixbuf = read_pixbuf(file, &ihdr, NULL);
            for_each_pixel(pixbuf, (bool (*)())negate_adapter, NULL);
            int status = save_to_bmp24bit(argv[2], NULL, NULL, pixbuf);
            free(pixbuf);
            fclose(file);
            return status;
        } else {
            color_table_t *table = read_table(file, &ihdr);
            for (uint32_t i = 0; i < table->size; i++) {
                negate(&table->colors[i].color);
            }
            FILE *new = fopen(argv[2], "w");
            fwrite(&fhdr, sizeof(fhdr), 1, new);
            fwrite(&ihdr, sizeof(ihdr), 1, new);
            fwrite(table->colors, sizeof(padded_color_t), table->size, new);
            uint8_t buffer[4096];
            int read;
            while ((read = fread(buffer, sizeof(uint8_t), sizeof(buffer), file))) {
                fwrite(buffer, sizeof(uint8_t), read, new);
                printf("Copied %d bytes\n", read);
            }
            fclose(new);
            free(table);
        }
        fclose(file);
        return 0;
    }
}