#ifndef __ID3_TEXT_H__
#define __ID3_TEXT_H__
#include "types.h"
#include "id3cb.h"
#include "frame.h"

typedef struct id3_text {
    id3_text_t    type;
    int           size;
    id3_enc_t     encoding;
    char         *data;
    char         *desc;
    char          tag[5];
} id3_text;

typedef int (*id3_text_handler)(const id3_text *,  void *);
int id3_frame_text_handler(ID3CB *id3, void *aux); 
int id3_text_default_handler(const id3_text *text, void *aux);
id3_text_t id3_get_text_frame_type(const id3_frame *frame); 
#endif
