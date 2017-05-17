#include <string.h>
#include <event2/buffer.h>
#include <syslog.h>
#include "dmap.h"

#define ENCODE_UINT32(buf, val) {  \
	*(buf + 0) = (val >> 24) & 0xff; \
	*(buf + 1) = (val >> 16) & 0xff; \
	*(buf + 2) = (val >>  8) & 0xff; \
	*(buf + 3) = val & 0xff;         \
}

#define ENCODE_UINT64(buf, val) {  \
	*(buf + 0) = (val >> 56) & 0xff; \
	*(buf + 1) = (val >> 48) & 0xff; \
	*(buf + 2) = (val >> 40) & 0xff; \
	*(buf + 3) = (val >> 32) & 0xff; \
	*(buf + 4) = (val >> 24) & 0xff; \
	*(buf + 5) = (val >> 16) & 0xff; \
	*(buf + 6) = (val >>  8) & 0xff; \
	*(buf + 7) = val & 0xff;         \
}

void dmap_add_char(struct evbuffer *evbuf, const char *tag, const char ch) {
	unsigned char buf[5] = {0};
	buf[3] = 1;
	buf[4] = ch;
	evbuffer_add(evbuf, tag, 4);
	evbuffer_add(evbuf, buf, 5);
}

void dmap_add_short(struct evbuffer *evbuf, const char *tag, const short s) {
	unsigned char buf[6] = {0};
	buf[3] = 2;
	buf[4] = (s >> 8) & 0xff;
	buf[5] = s & 0xff;
	evbuffer_add(evbuf, tag, 4);
	evbuffer_add(evbuf, buf, 6);
}

void dmap_add_int(struct evbuffer *evbuf, const char *tag, const int i) {
	unsigned char buf[8] = {0};
	buf[3] = 4;
	ENCODE_UINT32(buf + 4, i);
	evbuffer_add(evbuf, tag, 4);
	evbuffer_add(evbuf, buf, 8);
}

void dmap_add_string(struct evbuffer *evbuf, const char *tag, const char *str) {
	unsigned char buf[4];
	int len = strlen(str);
	ENCODE_UINT32(buf, len);
	evbuffer_add(evbuf, tag, 4);
	evbuffer_add(evbuf, buf, 4);
	if (str && len > 0)
	    evbuffer_add(evbuf, str, len);
}

void dmap_add_list(struct evbuffer *evbuf, const char *tag, const int len) {
	unsigned char buf[4];
	ENCODE_UINT32(buf, len);
	evbuffer_add(evbuf, tag, 4);
	evbuffer_add(evbuf, buf, 4);
}

void dmap_add_long(struct evbuffer *evbuf, const char *tag, const long l) {
	unsigned char buf[12] = {0};
	buf[3] = 8;
	ENCODE_UINT64(buf + 4, l);
	evbuffer_add(evbuf, tag, 4);
	evbuffer_add(evbuf, buf, 12);
}

void dmap_add_date(struct evbuffer *evbuf, const char *tag, const int date) {
   dmap_add_int(evbuf, tag, date); 
}
