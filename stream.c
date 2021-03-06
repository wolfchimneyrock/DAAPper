#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <event2/event.h>
#include <evhtp/evhtp.h>
#include <sqlite3.h>
#include "system.h"
#include "config.h"
#include "http.h"
#include "vector.h"
#include "meta.h"
#include "dmap.h"
#include "util.h"
#include "sql.h"
#include "session.h"
#include "writer.h"
#include "scratch.h"
#include "cache.h"
#include "stream.h"

//#define CHUNK_DELAY  250
//#define CHUNK_SIZE   1024*1024
//#define CHUNK_SIZE   0
//#define PRELOAD_SIZE 1024*1024
CACHE *file_cache = NULL;

void *create_segment(int id, void *a) {
	LOGGER(LOG_INFO, "    create_segment()");
    app *aux = (app *)a;
    char *path;
    sqlite3_stmt *stmt = NULL;
    CACHENODE *cn = NULL;
    if (aux->header == -1) { // we were passed an app object, not a string
        LOGGER(LOG_INFO, "got an app");
	    int ret;
	    stmt = aux->stmts[Q_GET_PATH];
            sqlite3_reset(stmt);
	    ret = sqlite3_bind_int(stmt, 1, id);
	    if (ret != SQLITE_OK) {
		syslog(LOG_ERR, "error binding value: '%d'\n", id);
		goto error;
	    }
	    ret = sqlite3_step(stmt);
	    if (ret != SQLITE_ROW) {
		syslog(LOG_ERR, "error retrieving file path for item %d\n", id);
		goto error;
	    }
	    path = sqlite3_column_text(stmt, 0);
    } else {  // we were passed a path string
        LOGGER(LOG_INFO, "got a string");
        path = (char *)a;
    }
    int fd = open(path, O_RDONLY); 
    struct stat st;
    if(fstat(fd, &st)) {
        LOGGER(LOG_ERR, "error stat() file %d", fd);
        goto error;
    }
    if (st.st_size > 0) {
        cn = malloc(sizeof(CACHENODE));
        cn->size = st.st_size;
        cn->file_segment = evbuffer_file_segment_new(
                fd, 0, st.st_size, 
                EVBUF_FS_CLOSE_ON_FREE
                );
    } else {
        LOGGER(LOG_ERR, "got file of size zero");
    }
error:
    if (stmt)
        sqlite3_reset(stmt);
    return cn;
}

typedef struct stream_t {
    int id;
    size_t size;
    size_t offset;
    size_t current;
    void *data;
    evbuf_t *buf;
    evhtp_request_t *req;
    evhtp_connection_t *conn;
    struct event *timer;
} stream_t;

static void schedule_next_chunk(stream_t *st, int ms) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = ms;
    evtimer_add(st->timer, &tv);
}

// send the next chunk
static void stream_item_chunk_cb(evutil_socket_t fd, short events, void *arg) {
    stream_t *st = (stream_t *)arg;
    if (st->conn->request == st->req) {
        if (st->offset >= st->size) {
            // no more chunks - clean up
		    //LOGGER(LOG_INFO, "    file %d finished sending chunks", st->id);
            evhtp_send_reply_chunk_end(st->req);
            event_free(st->timer);
            if (st->buf) {
                evbuffer_drain(st->buf, -1);
                evbuffer_free(st->buf);
            }
            free(st);
        } else {
		//LOGGER(LOG_INFO, "    file %d sending chunk %lu", st->id, st->current);
            size_t size = st->size - st->offset;
            st->current++;
            if (size > conf.chunksize)
                size = conf.chunksize;
            evbuffer_add_file_segment(st->buf, st->data, st->offset, size);
            st->offset += size;
            evhtp_send_reply_chunk(st->req, st->buf);
            schedule_next_chunk(st, conf.chunkdelay);
        }
    } else {
        // another request supercedes this one, stop sending chunks and clean up
        LOGGER(LOG_INFO, "    file %d interrupted at chunk %lu", st->id, st->current);
        if (st->buf) {
            while (evbuffer_get_length(st->buf))
                evbuffer_drain(st->buf, -1);
            evbuffer_free(st->buf);
        }
        free(st);
    }
    //close(fd);
    //return EVHTP_RES_OK;
}
void add_stream_headers_out(evhtp_request_t *req) {
    
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Accept-Ranges", "bytes", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Content-Type", "audio/*", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Cache-Control", "no-cache", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Expires", "-1", 0, 0));
 //   evhtp_headers_add_header(req->headers_out,
 //         evhtp_header_new("Connection", "keep-alive", 0, 0));

}

void res_stream_item(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_stream_headers_out(req);
    ADD_DATE_HEADER("Date");
    evhtp_connection_t *conn = evhtp_request_get_connection(req);
    evthr_t *thread          = conn->thread;
    app     *aux             = (app *)evthr_get_aux(thread);
    
    
     
    int id = atoi(req->uri->path->file);
    if (id == 0) {
        LOGGER(LOG_ERR, "invalid song id '%s' requested", 
                        req->uri->path->file);
        evhtp_send_reply(req, EVHTP_RES_ERROR);
        return;
    }

    CACHENODE *song = cache_set_and_get(file_cache, id, aux);
    
    if (song) { 
        stream_t *st = malloc(sizeof(stream_t));
        st->req    = req;
        st->size   = song->size;
        st->id     = id;
        st->data   = song->file_segment;
        st->offset = 0;
        st->current = 0;
        st->conn    = conn;
        st->buf = evbuffer_new();
        evbuffer_set_flags(st->buf, EVBUFFER_FLAG_DRAINS_TO_FD);
        int entire = 0;
        if (conf.chunksize <= 0) {
		LOGGER(LOG_INFO, "    thread %d sending entire file", st->id);
            entire = 1;
            evbuffer_add_file_segment(req->buffer_out, st->data, st->offset, st->size);
            st->offset += st->size;
        }  
        else if (conf.chunkpreload > 0) {
            size_t size = conf.chunkpreload;
            if (st->size <= size) {
                entire = 1;
                size = st->size;
            }
            evbuffer_add_file_segment(req->buffer_out, st->data, st->offset, size);
            st->offset += size;
        }
        if (entire) {
            req->flags |= EVHTP_REQ_FLAG_KEEPALIVE;
            evhtp_send_reply(req, EVHTP_RES_OK);
        } else {    
            req->flags |= EVHTP_REQ_FLAG_CHUNKED | EVHTP_REQ_FLAG_KEEPALIVE;
            st->timer  = evtimer_new(aux->base, stream_item_chunk_cb, st);
            evhtp_send_reply_chunk_start(req, EVHTP_RES_OK);
            schedule_next_chunk(st, 0);
        }
        db_inc_playcount(id);
    } else {
        LOGGER(LOG_INFO, "got a NULL song from cache");
        evhtp_send_reply(req, EVHTP_RES_NOTFOUND);
    }
    // cleanup now done in callback
}
