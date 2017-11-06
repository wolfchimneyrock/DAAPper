#ifndef __CACHE_H__
#define __CACHE_H__

typedef struct _cache CACHE;


CACHE *cache_init(int capacity, int stripes, void *(*create)(int, void *), int (*destroy)(void *));

void *cache_set_and_get(CACHE *c, int key, void *a);

#endif
