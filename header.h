#ifndef __ID3_HEADER_H__
#define __ID3_HEADER_H__

typedef struct id3_header {
	char tag[3];
	char vmajor;
	char vminor;
	char flags;
	int  tag_size;
	int  exthead_size;
} id3_header;

typedef int (*id3_header_handler)(id3_header *, void *);

int id3_header_default_handler(id3_header *header, void *aux);

int  id3_is_header (const char *raw);
void id3_get_header(id3_header *header, char *raw);


#endif
