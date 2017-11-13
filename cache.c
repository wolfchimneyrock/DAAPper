// A thread-safe cache using traditional locking with N partitions, or
// a lock-free cache if N = 0 with exponential back-off waiting during contention

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include "system.h"
#include "cache.h"
#include "util.h"

#define INITIAL_BACKOFF 50000

const static void *_UPDATING = (void *)-1;

struct _lock {
    pthread_mutex_t mutex;
    pthread_cond_t cond_writer;
    pthread_cond_t cond_reader;
    int readers;
    int writers;
    int updating;
};

struct _cache {
    void **map;
    int capacity;
    int used;
    int stripes;
    void *(*create)(int, void *);
    int (*destroy)(void *);

    struct _lock *locks;
};

CACHE *cache_init(int capacity, int stripes, void *(*create)(int, void *), int (*destroy)(void *)) {
    LOGGER(LOG_INFO, "cache capacity %d with %d stripes", capacity, stripes);
    if (capacity < 1) {
        LOGGER(LOG_ERR, "cache capacity must be > 0");
        exit(1);
    }
    CACHE *c = malloc(sizeof(CACHE));
    c->map = calloc(capacity, sizeof(void *));
    c->capacity = capacity;
    c->used = 0;
    c->stripes = stripes;
    c->create = create;
    c->destroy = destroy;

    if (stripes > 0) {
        c->locks = calloc(stripes, sizeof(struct _lock));

        
        for (int i = 0; i < stripes; i++) {
            pthread_mutex_init(&c->locks[i].mutex, NULL);
            pthread_cond_init (&c->locks[i].cond_writer,  NULL);
            pthread_cond_init (&c->locks[i].cond_reader,  NULL);
            c->locks[i].writers  = 0;
            c->locks[i].readers  = 0;
            c->locks[i].updating = 0;
            pthread_cond_signal(&c->locks[i].cond_writer);
            pthread_cond_signal(&c->locks[i].cond_reader);
        }
    } else c->locks = NULL;
    return c;
}

static int _stripe(CACHE *c, int key) {
    return (key % c->stripes);
}

static void _read_lock(CACHE *c, int stripe) {
    struct _lock *l;
    l = &c->locks[stripe];

    pthread_mutex_lock(&l->mutex);
    while (l->updating || l->writers)
        pthread_cond_wait(&l->cond_reader, &l->mutex);
    l->readers++;
    pthread_mutex_unlock(&l->mutex);
}

static void _read_unlock(CACHE *c, int stripe) {
    struct _lock *l;
    l = &c->locks[stripe];

    pthread_mutex_lock(&l->mutex);
    l->readers--;
    if (l->readers == 0)
        pthread_cond_signal(&l->cond_writer);
    pthread_mutex_unlock(&l->mutex);
}

static void _write_lock(CACHE *c, int stripe) {
    struct _lock *l;
    l = &c->locks[stripe];
    pthread_mutex_lock(&l->mutex);
    l->writers++;
    while (l->readers || l->updating)
        pthread_cond_wait(&l->cond_writer, &l->mutex);
    
    l->updating = 1;
    pthread_mutex_unlock(&l->mutex);
}

static void _write_unlock(CACHE *c, int stripe) {
    struct _lock *l;
    l = &c->locks[stripe];
    pthread_mutex_lock(&l->mutex);
    l->updating = 0;
    l->writers--;
    pthread_cond_signal(&l->cond_writer);
    pthread_cond_broadcast(&l->cond_reader);
    pthread_mutex_unlock(&l->mutex);
}

static void _write_lock_all(CACHE *c) {
    for (int i = 0; i < c->stripes; i++)
        _write_lock(c, i);
}

static void _write_unlock_all(CACHE *c) {
    for (int i = 0; i < c->stripes; i++)
        _write_unlock(c, i);
}


static void _exponential_backoff(void **mem, const void *value) {
    int usecs = INITIAL_BACKOFF;
    while (*mem == value) {
        LOGGER(LOG_INFO, "   waiting %d ... ", usecs);
        _spin_pause();
        usleep(usecs);
        usecs *= 2;
    }
}

void *cache_set_and_get(CACHE *c, int key, void *a) {
    if (c == NULL) return NULL; 
    if (c->stripes) { // locking version
        void *result;
        int stripe = _stripe(c, key);
        _read_lock(c, stripe);

        {
            if (key < c->capacity) {
                // no need to grow
                if (NULL == (result = c->map[key])) {
                    // cache miss
                    _read_unlock(c, stripe);
                    _write_lock(c, stripe);
                    result = c->map[key] = (*c->create)(key, a);
                    _write_unlock(c, stripe);
                } else {
                    _read_unlock(c, stripe);
                }
                    
            } else {
                // we need to grow, and its a cache miss
                _write_lock_all(c);
                while (key > c->capacity)
                    c->capacity *= 2;
                c->map = realloc(c->map, c->capacity);
                c->map[key] = (*c->create)(key, a);
                _write_unlock_all(c);
            }
        }
        return result;
    }
    else { // lock-free version
        void *result = c->map[key];
        if (result == NULL) {
            // cache miss, we want to try to CAS
            int success = __sync_bool_compare_and_swap(&c->map[key], result, _UPDATING);
	    __sync_synchronize();
            if (success) { 
                // we won the race to create the resource, now no one should try to edit it until we done
                return c->map[key] = (*c->create)(key, a);
                
            } else {
                // it has changed from NULL to something else
                _exponential_backoff(&c->map[key], _UPDATING);
                return c->map[key];
            }
        } else {
            _exponential_backoff(&c->map[key], _UPDATING);
	    __sync_synchronize();
            return c->map[key];
        }
    }   
}



