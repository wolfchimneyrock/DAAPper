#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "ringbuffer.h"

struct _ringbuffer { 
    void **data;
    size_t head, tail, used, cap;
    sem_t empty_sem, filled_sem;
    pthread_mutex_t buffer_mutex;
    pthread_cond_t  buffer_ready;
};

RINGBUFFER *rb_init(size_t capacity) {
    if (capacity < 1) return NULL;
    RINGBUFFER *rb = malloc(sizeof(RINGBUFFER));
    rb->data = malloc(capacity * sizeof(void *));
    rb->cap  = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->used = 0;
    pthread_mutex_init(&rb->buffer_mutex, NULL);
    pthread_cond_init (&rb->buffer_ready, NULL);
    sem_init(&rb->empty_sem,  0, capacity);
    sem_init(&rb->filled_sem, 0, 0);
    pthread_cond_broadcast(&rb->buffer_ready);
    return rb;
}

void rb_free(RINGBUFFER *rb) {
    if (rb == NULL) return;
    free(rb->data);
    sem_destroy(&rb->empty_sem);
    sem_destroy(&rb->filled_sem);
    pthread_mutex_destroy(&rb->buffer_mutex);
    pthread_cond_destroy (&rb->buffer_ready);
    free(rb);
}

int rb_isfull(RINGBUFFER *rb) {
    if (rb == NULL) return -1;
    return rb->used == rb->cap;
}

int rb_isempty(RINGBUFFER *rb) {
    if (rb == NULL) return -1;
    return rb->used == 0;
}

int rb_pushback(RINGBUFFER *rb, void *data) {
    int ret;
    if (rb == NULL) return -1;
    while(1) {
        pthread_testcancel();
        ret = sem_wait(&rb->empty_sem);
            if (ret == -1 && errno == EINTR) continue;
            pthread_mutex_lock(&rb->buffer_mutex);
                if (!rb_isfull(rb)) {
                    *(rb->data + rb->tail) = (void *)data;
                    rb->tail = (rb->tail + 1) % rb->cap;
                    rb->used++;
                    ret = 0;
                } else ret = -1;
            pthread_mutex_unlock(&rb->buffer_mutex);
        sem_post(&rb->filled_sem);
        return ret;
    }
}

void *rb_popfront(RINGBUFFER *rb) {
    int ret;
    if (rb == NULL) return NULL;
    void *result = NULL;
    while (1) {
        pthread_testcancel();
        ret = sem_wait(&rb->filled_sem);
            if (ret == -1 && errno == EINTR) continue;
            pthread_mutex_lock(&rb->buffer_mutex);
                if (!rb_isempty(rb)) {
                    result = (rb->data)[rb->head];
                    rb->head = (rb->head + 1) % rb->cap;
                    rb->used--;
                }
            pthread_mutex_unlock(&rb->buffer_mutex);
        sem_post(&rb->empty_sem);
        return result;
    }
}
