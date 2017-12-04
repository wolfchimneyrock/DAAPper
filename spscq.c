#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "spscq.h"
#include "util.h"

#define BITS          16
#define MASK_USED     ((1 << BITS) - 1)
#define MASK_TAIL     (MASK_USED << BITS)

struct _spscq {
    void **data;
    uintptr_t head, tail_used;
    size_t capacity;
    int deleted;
    sem_t empty_sem, filled_sem;
};

SPSCQ *spscq_init(size_t capacity) {
    if (capacity < 1) return NULL;
    SPSCQ *s = malloc(sizeof(SPSCQ));
    s->data = calloc(capacity, sizeof(void *));
    s->capacity = capacity;
    s->head = 0;
    s->tail_used = 0;
    return s;
}

int spscq_isfull(SPSCQ *s) {
    if (s == NULL || s->deleted) return 1;
    return (s->tail_used & MASK_USED) >= s->capacity;
}

int spscq_isempty(SPSCQ *s) {
    if (s == NULL || s->deleted) return 1;
    return (s->tail_used & MASK_USED) == 0;
}

void *spscq_pop(SPSCQ *s) {
    uintptr_t current, updated;
    if (s && !spscq_isempty(s)) {
        do {
            current = s->head;
            updated = (current + 1) % s->capacity;
        } while (!__sync_bool_compare_and_swap(&s->head, current, updated));
        void *result = s->data[current];
        __sync_synchronize();
        do {
            current = s->tail_used;
            updated = current - 1;
        } while (s && !__sync_bool_compare_and_swap(&s->tail_used, current, updated));
        return result;
    } else
        return NULL;
}

int spscq_push(SPSCQ *s, void *data) {
    uintptr_t current, newtail, updated;
    if (s && !spscq_isfull(s)) {
        do {
            current = s->tail_used;
            newtail = (((current & MASK_TAIL) >> BITS) + 1) % s->capacity;

            updated = ((current & MASK_USED) + 1) | (newtail << BITS);
        } while (s && !__sync_bool_compare_and_swap(&s->tail_used, current, updated));
        __sync_synchronize();
        s->data[newtail] = data;
        return 0;
    }
    return -1;
}

void spscq_free(SPSCQ *s) {
    if (s == NULL) return;
    uintptr_t current, updated;
    s->deleted = 1;
    __sync_synchronize();
    do {
        current = s->tail_used;
        updated = 0;
    } while (s && !__sync_bool_compare_and_swap(&s->tail_used, current, updated));
    __sync_synchronize();
    free(s->data);
    free(s);
}
