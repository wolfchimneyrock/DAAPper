#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

typedef struct _meta_ringbuffer RINGBUFFER; 

RINGBUFFER *rb_init           (size_t capacity, const char *type);
void        rb_free           (RINGBUFFER *rb);
int         rb_isfull         (RINGBUFFER *rb);
int         rb_size           (RINGBUFFER *rb);
int         rb_isempty        (RINGBUFFER *rb);
int         rb_pushback       (RINGBUFFER *rb, void *data);
void       *rb_popfront       (RINGBUFFER *rb);
int         rb_drain          (RINGBUFFER *rb, void **dest, size_t max);
#endif
