#ifndef __RB_LF_H__
#define __RB_LF_H__


void       *rb_lf_init           (size_t capacity);
void        rb_lf_free           (void *rb);
int         rb_lf_isfull         (void *rb);
int         rb_lf_size           (void *rb);
int         rb_lf_isempty        (void *rb);
int         rb_lf_pushback       (void *rb, void *data);
void       *rb_lf_popfront       (void *rb);
int         rb_lf_drain          (void *rb, void **dest, size_t max);
#endif
