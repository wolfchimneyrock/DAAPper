#include <stdlib.h>
#include <string.h>
#include "scratch.h"

struct _scratch {
    char *scratch;
    char *free_scratch;
    size_t available_scratch;
    size_t capacity;
};

SCRATCH *scratch_new(size_t capacity) {
    SCRATCH *s  = malloc(sizeof(SCRATCH));
    s->capacity = capacity;
    s->scratch  = malloc(capacity);
    scratch_reset(s);
    return s;
}

void *scratch_detach(SCRATCH *s) {
    if (s) {
        void *result = s->scratch;
        s->scratch   = malloc(s->capacity);
        scratch_reset(s);
        return result;
    } else 
        return NULL;
}

void *scratch_head(SCRATCH *s) {
    if   (s) return s->scratch;
    else     return NULL;
}

void *scratch_get(SCRATCH *s, size_t size) {
    void *result;   
    if (s && s->available_scratch >= size) {
        result = s->free_scratch;
        s->free_scratch      += size;
        s->available_scratch -= size;
    } else result = NULL;
    return result;
}

void scratch_reset(SCRATCH *s) {
    if (s) {
        memset(s->scratch, 0, s->capacity);
        s->free_scratch      = s->scratch;
        s->available_scratch = s->capacity;
    }
}

void scratch_free(SCRATCH *s, scratch_keep keep) {
    if (s) {
        if (keep == SCRATCH_FREE) free(s->scratch);
        free(s);
    }
}
