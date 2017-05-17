#include <string.h>
#include "id3cb.h"
#include "header.h"

int id3_is_header(const char *raw) {
	return !memcmp(raw, "ID3", 3);
}

int id3_header_default_handler(id3_header *header, void *aux) {
	return 0;
}

void id3_get_header(id3_header *header, char *raw) {
	memcpy(header->tag, raw, 3);
	header->vmajor = raw[3];
	header->vminor = raw[4];
	header->flags  = raw[5];
	DECODE_UINT32BE(raw + 6, header->tag_size);
	if (header->flags & ID3_HAS_EXTENDED == ID3_HAS_EXTENDED)
		DECODE_UINT32BE(raw + 10, header->exthead_size);
	else 
		header->exthead_size = 0;
}


