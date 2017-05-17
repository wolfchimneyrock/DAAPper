
#ifndef __ID3_URL_H__
#define __ID3_URL_H__

#include "types.h"
#include "id3cb.h"
#include "frame.h"

typedef struct id3_url {
    id3_url_t     type;
    id3_enc_t     encoding;
    char         *desc;
    char         *data;
} id3_url;

typedef int (*id3_url_handler)(const id3_url *,  void *);


int id3_frame_url_handler(ID3CB *id3, void *aux); 

int id3_url_default_handler(const id3_url *url, void *aux);

id3_url_t id3_get_url_frame_type(const id3_frame *frame); 


#endif
