#define INIT_QRPNG_BUF_SZ 4096


struct qrpng_buf_s {
    u_char  *data;
    size_t   cap;
    size_t   len;
};


typedef unsigned char u_char;
typedef struct qrpng_buf_s qrpng_buf_t;


qrpng_buf_t * qrpng_init();
void qrpng_free(qrpng_buf_t *q);
int qrpng_create(qrpng_buf_t *qrpng_buf, const char *data);
