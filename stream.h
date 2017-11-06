#ifndef __STREAM_H__
#define __STREAM_H__

#include <event2/event.h>
#include <evhtp/evhtp.h>
#include "cache.h"

typedef struct _cachenode {
    size_t size;
    void *file_segment;
} CACHENODE;

extern CACHE *file_cache;

void *create_segment(int id, void *a);
void res_stream_item(evhtp_request_t *req, void *a);

#endif
