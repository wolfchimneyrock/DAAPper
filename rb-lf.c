#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include "system.h"
#include "ringbuffer.h"
#include "util.h"

#define BITS          16
#define MASK_USED     ((1 << BITS) - 1)
#define MASK_TAIL     (MASK_USED << BITS)
#define MASK_HEAD     (MASK_TAIL << BITS)

typedef struct _ringbuffer {
    void **data;
    uintptr_t head, tail_used;
    size_t capacity;
    int deleted;
    sem_t empty_sem, filled_sem;
} RB_LF;

RB_LF *rb_lf_init(size_t capacity) {
    if (capacity < 1 || capacity > (1 << BITS )) return NULL;
    LOGGER(LOG_INFO, "    rb_init()");
    RB_LF *rb = malloc(sizeof(RB_LF));
    rb->capacity = capacity;
    rb->data = calloc(capacity, sizeof(void *));
    rb->tail_used = 0;
    rb->head = 0;
    rb->deleted = 0;
    sem_init(&rb->filled_sem, 0, 0);
    sem_init(&rb->empty_sem,  0, capacity);
    return rb;
}

int rb_lf_isfull(RB_LF *rb) {
    if (rb == NULL || rb->deleted) return -1;
    return (rb->tail_used & MASK_USED) >= rb->capacity;
}

int rb_lf_isempty(RB_LF *rb) {
    if (rb == NULL || rb->deleted) return -1;
    return (rb->tail_used & MASK_USED) == 0;
}

int rb_lf_size(RB_LF *rb) {
    if (rb == NULL || rb->deleted) return -1;
    return (rb->tail_used & MASK_USED);
}

int rb_lf_pushback(RB_LF *rb, void *data) {
    if (rb == NULL || rb->deleted) return -1;
    uintptr_t current, updated, tail, used;
    int success = 0;
    int ret;
    while (rb && !rb->deleted) {
        pthread_testcancel();
        ret = sem_wait(&rb->empty_sem);
        if (ret == -1 && errno == EINTR) continue;

        do {
            current = rb->tail_used;
            tail    = (((current & MASK_TAIL) >> BITS) + 1) % rb->capacity;
            used    = ((current & MASK_USED) + 1);
            updated = (tail << BITS) | used;
        } while (!__sync_bool_compare_and_swap(&rb->tail_used, current, updated));
        // we got it
        rb->data[(current & MASK_TAIL)>>BITS] = data;
        sem_post(&rb->filled_sem);
        return 0;
    }
    return -1;
}
                
void *rb_lf_popfront(RB_LF *rb) {
    if (rb == NULL || rb->deleted) return NULL;
    void *result;
    uintptr_t current, updated, head, used, newhead;
    int success = 0;
    int ret;
    while (rb && !rb->deleted) {
        pthread_testcancel();
        ret = sem_wait(&rb->filled_sem);
        if (ret == -1 && errno == EINTR) continue;
        do {
            head = rb->head;
            updated = (head + 1) % rb->capacity;

        } while (!__sync_bool_compare_and_swap(&rb->head, head, updated));
        // we now have reserved the former head node
        result = rb->data[head];
        __sync_synchronize();
        // now its safe to release the prior node by decrementing used
        do {
            current = rb->tail_used;
            updated = current - 1;
        } while (!__sync_bool_compare_and_swap(&rb->tail_used, current, updated));

        sem_post(&rb->empty_sem);
        return result;
    }
    return NULL;
}
   
int rb_lf_drain(RB_LF *rb, void **dest, size_t max) {
    uintptr_t count, current, updated, head, used, newhead, available;
    int ret;
    while (rb && !rb->deleted) {
        pthread_testcancel();
        ret = sem_wait(&rb->filled_sem);
        if (ret == -1 && errno == EINTR) continue;
        // we got one, try for more up to max
        count = 1;
        while (count < max) {
            ret = sem_trywait(&rb->filled_sem);
            if (ret == -1 && errno == EINTR)  continue;
            if (ret == -1 && errno == EAGAIN) break;
            count++;
        }

        do {
            available = rb->tail_used & MASK_USED;
            if (available < count) count = available;
            head = rb->head;
            updated = (head + count) % rb->capacity;

        } while (!__sync_bool_compare_and_swap(&rb->head, head, updated));

        // we now have reserved count nodes at the head
        if (updated > head) {

            // not split across end of buffer - a single memcpy
            memcpy(dest, rb->data + head, count*sizeof(void *));
        } else {
            // two memcpy required
            size_t first = rb->capacity - head;
            
            memcpy(dest, rb->data + head, first*sizeof(void *));
            memcpy(dest + first, rb->data, (count - first)*sizeof(void *));
        }
        __sync_synchronize();
        // now safe to release the nodes
        do {
            current = rb->tail_used;
            updated = current - count;
        } while (!__sync_bool_compare_and_swap(&rb->tail_used, current, updated));
        for (int i = 0; i < count; i++) sem_post(&rb->empty_sem);
        return count;
    }
    return -1;
}

void rb_lf_free(RB_LF *rb) {
    if (rb == NULL) return;
    rb->deleted = 1;
    free(rb->data);
    sem_destroy(&rb->filled_sem);
    free(rb);
}


