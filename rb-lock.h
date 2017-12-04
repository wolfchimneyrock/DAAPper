#ifndef __RB_LOCK_H__
#define __RB_LOCK_H__


void       *rb_lock_init           (size_t capacity);
void        rb_lock_free           (void *rb);
int         rb_lock_isfull         (void *rb);
int         rb_lock_size           (void *rb);
int         rb_lock_isempty        (void *rb);
int         rb_lock_pushback       (void *rb, void *data);
void       *rb_lock_popfront       (void *rb);
int         rb_lock_drain          (void *rb, void **dest, size_t max);
#endif
