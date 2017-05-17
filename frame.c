#include <string.h>
#include "id3cb.h"
#include "types.h"
#include "frame.h"

int id3_is_frame(const char *raw) {
    return id3_get_frame_type(raw);
}

void id3_get_frame(id3_frame *frame, const char *raw) {
    memcpy(frame->tag, raw, 4);
    DECODE_UINT32BE(raw + 4, frame->size);
    memcpy(frame->flags, raw + 8, 2);
    frame->data = (char *)raw + 10;
}

int id3_frame_default_handler(ID3CB *id3, void *aux) {
    return 0;
}

id3_frame_t id3_get_frame_type(const char *tag) {
    switch (tag[0]) {
        case 'A': switch(tag[1]) {
                case 'P': return ID3_FRAME_PICTURE;
                case 'E': return ID3_FRAME_ENCRYPTION;
                default : return ID3_FRAME_INVALID;    }
        case 'C': switch(tag[3]) {
                case 'M': return ID3_FRAME_COMMENT;
                case 'R': return ID3_FRAME_COMMERCIAL;
                default : return ID3_FRAME_INVALID;    }
        case 'E': switch(tag[1]) {
                case 'N': return ID3_FRAME_ENCRYPTREG;
                case 'Q': return ID3_FRAME_EQ;
                case 'T': return ID3_FRAME_ETCO;
                default : return ID3_FRAME_INVALID;    }
        case 'G': switch(tag[1]) {
                case 'E': return ID3_FRAME_GENERAL;
                case 'R': return ID3_FRAME_GROUPID;
                default : return ID3_FRAME_INVALID;    }
        case 'I': return ID3_FRAME_IPLS;
        case 'L': return ID3_FRAME_LINK;
        case 'M': switch (tag[1]) {
                case 'C': return ID3_FRAME_MCDI;
                case 'L': return ID3_FRAME_MLLT;
                default : return ID3_FRAME_INVALID;    }
        case 'O': return ID3_FRAME_OWNERSHIP;
        case 'P': switch (tag[2]) {
                case 'I': return ID3_FRAME_PRIVATE;
                case 'N': return ID3_FRAME_PLAYCOUNT;
                case 'P': return ID3_FRAME_POPULARITY;
                case 'S': return ID3_FRAME_POSITION;
                default : return ID3_FRAME_INVALID;    }
        case 'R': switch (tag[2]) {
                case 'A': return ID3_FRAME_RVOLUME;
                case 'R': return ID3_FRAME_REVERB;
                default : return ID3_FRAME_INVALID;    }
        case 'S': switch (tag[2]) {
                case 'L': return ID3_FRAME_SYLT;
                case 'T': return ID3_FRAME_SYTC;
                default : return ID3_FRAME_INVALID;    }
        case 'T': return ID3_FRAME_TEXT;
        case 'U': switch (tag[2]) {
                case 'I': return ID3_FRAME_UNIQUE;
                case 'E': return ID3_FRAME_TERMS;
                case 'L': return ID3_FRAME_USLT;
                default : return ID3_FRAME_INVALID;    }
        case 'W': return ID3_FRAME_URL;
        default:  return ID3_FRAME_INVALID;
    }
}
