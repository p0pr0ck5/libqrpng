/*
 * Copyright 2018 Robert Paprocki
 */


#include <stdlib.h>
#include <string.h>
#include "png.h"
#include "qrencode.h"
#include "qrpng.h"


static int qr_hint        = QR_MODE_8;
static int qr_size        = 8;
static int qr_margin      = 1;
static int qr_version     = 0;
static QRecLevel qr_level = QR_ECLEVEL_L;


static void user_write_data(png_structp png_ptr, png_bytep data,
                            png_size_t length)
{
    qrpng_buf_t *qrpng_buf = (qrpng_buf_t *)png_get_io_ptr(png_ptr);

    /* increase the buffer size if libpng wants to write more data that we hold
     * by default we double in size, but that may not be sufficient */
    if (qrpng_buf->len + length > qrpng_buf->cap) {
        size_t new_cap = qrpng_buf->cap * 2;

        while (qrpng_buf->len + length > new_cap) {
            new_cap *= 2;
        }

        qrpng_buf->cap = new_cap;
        qrpng_buf->data = realloc(qrpng_buf->data, new_cap);
    }

    memcpy(qrpng_buf->data + qrpng_buf->len, data, length);
    qrpng_buf->len += length;
}


/* needed but not used */
static void user_flush_data(png_structp png_ptr)
{
}


qrpng_buf_t *
qrpng_init()
{
    qrpng_buf_t  *qrpng_buf;

    qrpng_buf = malloc(sizeof(qrpng_buf_t));
    if (qrpng_buf == NULL) {
        return NULL;
    }

    qrpng_buf->len  = 0;
    qrpng_buf->cap  = INIT_QRPNG_BUF_SZ;
    qrpng_buf->data = malloc(INIT_QRPNG_BUF_SZ);
    if (qrpng_buf->data == NULL) {
        return NULL;
    }

    return qrpng_buf;
}


void qrpng_free(qrpng_buf_t *q)
{
    if (q->data != NULL) {
        free(q->data);
    }
    if (q != NULL) {
        free(q);
    }
}


int
qrpng_create(qrpng_buf_t *qrpng_buf, const char *data)
{
    int             x, y, xx, yy, bit, realwidth;
    QRcode         *qrcode;
    png_infop       info_ptr; 
    png_structp     png_ptr;
    unsigned char  *row, *p, *q;


    qrcode = QRcode_encodeString(data, qr_version, qr_level, qr_hint, 1);


    realwidth = (qrcode->width + qr_margin * 2) * qr_size;
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
    

    png_set_write_fn(png_ptr, qrpng_buf, user_write_data, user_flush_data);
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


    return 0;
}
