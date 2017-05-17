/**
 * @file dmap.h
 * @author Robert Wagner
 *
 * This defines the data type and operations to encode data into a daap stream
 *
 */

#ifndef __DMAP_H__
#define __DMAP_H__

#include <event2/buffer.h>
typedef struct evbuffer EVBUF;
typedef enum dmap_t {
    T_CHAR   = 1,
    T_SHORT  = 3,
    T_INT    = 5,
    T_LONG   = 7,
    T_STRING = 9,
    T_DATE   = 10,
    T_VER    = 11,
    T_LIST   = 12
} dmap_t;

void dmap_add_char  (EVBUF *evbuf, const char *tag, const char ch) ;
void dmap_add_short (EVBUF *evbuf, const char *tag, const short s) ;
void dmap_add_int   (EVBUF *evbuf, const char *tag, const int i) ;
void dmap_add_long  (EVBUF *evbuf, const char *tag, const long l) ;
void dmap_add_string(EVBUF *evbuf, const char *tag, const char *str) ;
void dmap_add_list  (EVBUF *evbuf, const char *tag, const int len) ;
void dmap_add_date  (EVBUF *evbuf, const char *tag, const int date);
#endif
