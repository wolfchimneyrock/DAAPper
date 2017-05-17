#ifndef __ID3_FRAME_H__
#define __ID3_FRAME_H__
#include "types.h"
#include "id3cb.h"
typedef struct id3_frame {
    char  tag[4];
    int   size;
    char  flags[2];
    char *data;
} id3_frame;

typedef int (*id3_frame_handler)(ID3CB *, void *);
int id3_frame_default_handler(ID3CB *id3, void *aux);
int id3_is_frame(const char *raw); 
id3_frame_t id3_get_frame_type(const char *tag); 
void id3_get_frame(id3_frame *frame, const char *raw);


#endif
