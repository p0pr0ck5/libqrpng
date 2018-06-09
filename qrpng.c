/*
 * Copyright 2018 Robert Paprocki
 */


#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "png.h"
#include "qrencode.h"


static int qr_hint        = QR_MODE_8;
static int qr_size        = 4;
static int qr_margin      = 4;
static int qr_version     = 0;
static QRecLevel qr_level = QR_ECLEVEL_L;


int
create_qr_png(const char *data, const char *path)
{
    int             x, y, xx, yy, bit, realwidth;
    FILE           *fp;
    QRcode         *qrcode;
    png_infop       info_ptr; 
    png_structp     png_ptr;
    unsigned char  *row, *p, *q;


    fp = fopen(path, "w");
    if (fp == NULL) {
        return 1;
    }


    qrcode = QRcode_encodeString(data, qr_version, qr_level, qr_hint, 1);


    realwidth = (qrcode->width + qr_margin * 2) * qr_size;
    printf("%d bytes", (realwidth + 7) / 8);
    row = malloc((realwidth + 7) / 8);
    if (row == NULL) {
        return 2;
    }   
 
   
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        return 3;
    }   

    
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        return 4;
    }   

    
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fprintf(stderr, "Failed to write PNG image.\n");
        exit(EXIT_FAILURE);
    }   
    

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr,
                 realwidth, realwidth,
                 1,
                 PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    

    /* top margin */
    memset(row, 0xff, (realwidth + 7) / 8);
    for (y = 0; y < qr_margin * qr_size; y++) {
        png_write_row(png_ptr, row);
    }   

    
    /* data */
    p = qrcode->data;
    for (y = 0; y < qrcode->width; y++) {
        bit = 7;
        memset(row, 0xff, (realwidth + 7) / 8);
        q = row;
        q += qr_margin * qr_size / 8;
        bit = 7 - (qr_margin * qr_size % 8); 
        for (x = 0; x < qrcode->width; x++) {
            for (xx = 0; xx < qr_size; xx++) {
                *q ^= (*p & 1) << bit;
                bit--;
                if (bit < 0) {
                    q++;
                    bit = 7;
                }
            }
            p++;
        }
        for (yy = 0; yy < qr_size; yy++) {
            png_write_row(png_ptr, row);
        }
    }


    /* bottom margin */
    memset(row, 0xff, (realwidth + 7) / 8);
    for (y = 0; y < qr_margin * qr_size; y++) {
        png_write_row(png_ptr, row);
    }


    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);


    QRcode_free(qrcode);


    free(row);
    fflush(fp);
    fclose(fp);


    return 0;
}
