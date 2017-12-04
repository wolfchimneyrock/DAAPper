#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include "ringbuffer.h"
#include "rb-lock.h"
#include "rb-lf.h"

struct _meta_ringbuffer { 
    void   *rb;
    void  (*free)(void *);
    int   (*isfull)(void *);
    int   (*isempty)(void *);
    int   (*size)(void *);
    int   (*pushback)(void *, void *);
    void *(*popfront)(void *);
    int   (*drain)(void *, void **, size_t);
};



RINGBUFFER *rb_init(size_t capacity, const char *type) {
    RINGBUFFER *rb = malloc(sizeof(RINGBUFFER));
    if (strcmp(type, "lock") == 0) {
        // use locking
        rb->rb       = rb_lock_init(capacity);
        rb->free     = &rb_lock_free;
        rb->isfull   = &rb_lock_isfull;
        rb->isempty  = &rb_lock_isempty;
        rb->size     = &rb_lock_size;
        rb->pushback = &rb_lock_pushback;
        rb->popfront = &rb_lock_popfront;
        rb->drain    = &rb_lock_drain;
    } else
    if (strcmp(type, "lf") == 0) {
        // use lock-free
        rb->rb       = rb_lf_init(capacity);
        rb->free     = &rb_lf_free;
        rb->isfull   = &rb_lf_isfull;
        rb->isempty  = &rb_lf_isempty;
        rb->size     = &rb_lf_size;
        rb->pushback = &rb_lf_pushback;
        rb->popfront = &rb_lf_popfront;
        rb->drain    = &rb_lf_drain;
    
    } else
    if (strcmp(type, "mc") == 0) {
        // use multi-queue
    
    } else {
        // use default

    }    

    return rb;
}

void rb_free(RINGBUFFER *rb) {
    if (rb == NULL || rb->rb == NULL) return;
    (rb->free)(rb->rb);
    free(rb);
}

int rb_isfull(RINGBUFFER *rb) {
    if (rb == NULL || rb->rb == NULL) return -1;
    return (rb->isfull)(rb->rb);
}

int rb_isempty(RINGBUFFER *rb) {
    if (rb == NULL || rb->rb == NULL) return -1;
    return (rb->isempty)(rb->rb);
}

int rb_size(RINGBUFFER *rb) {
    if (rb == NULL || rb->rb == NULL) return -1;
    return (rb->size)(rb->rb);
}

int rb_pushback(RINGBUFFER *rb, void *data) {
    if (rb == NULL || rb->rb == NULL) return -1;
    return (rb->pushback)(rb->rb, data);
}

void *rb_popfront(RINGBUFFER *rb) {
    if (rb == NULL || rb->rb == NULL) return NULL;
    return (rb->popfront)(rb->rb);
}

int rb_drain(RINGBUFFER *rb, void **dest, size_t max) {
    if (rb == NULL || rb->rb == NULL) return -1;
    return (rb->drain)(rb->rb, dest, max);
}		
