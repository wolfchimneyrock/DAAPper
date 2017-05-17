#include <stdlib.h>
#include <errno.h>
#include "vector.h"

// These vector functions adapted from hw1
int vector_new(vector *d, long initial) {
    if (initial == 0) return -1;
    d->used = 0;
    d->data = malloc(initial * sizeof(void *));
	if (d->data == NULL) { // allocation failed - return error
        d->capacity = 0;
        return -1;
    }
    d->capacity = initial;
    d->used = 0;
    return initial;
}

void vector_free(vector *d) {
    free(d->data);
	d->capacity = 0;
    d->used     = 0;
}

int vector_pushback(vector *d, const void *x) {
    if (d->used == d->capacity) {
        void **block = realloc(d->data, 2 * d->capacity * sizeof(void *));
		if (block == NULL) { // original bbuffer still intact
            errno = ENOMEM;
            return -1;
        } 
        d->capacity = d->capacity * 2;
        d->data = block;
    }
	d->data[d->used++] = (void *)x;
    return 0;
}

void *vector_peekback(vector *d) {
	if (d->used > 0) return d->data[d->used - 1];
	return NULL;
}

void vector_popback(vector *d) {
	if ((d->used > 0)) {
		d->used--;
	}
}
