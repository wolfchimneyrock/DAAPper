#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "id3cb.h"
#include "header.h"
#include "frame.h"
#include "text.h"
#include "url.h"

#define ID3_SCRATCH_SIZE 4096
const char *id3_text_encodings[] = {
    "ISO-8859-1",
    "UTF-16",
    "UTF-16BE",
    "UTF-8"
};
typedef struct id3_frame_table {
    const char *desc;
} id3_frame_table;
id3_frame_table default_frames[] = {
    { "invalid / unknown"        },
    { "unique identifier"        },
    { "text"                     },
    { "url"                      },
    { "involved players"         }, 
    { "music CD identifier"      },
    { "event timing code"        },
    { "MPEG location lookup"     },
    { "synchronized timecode"    },
    { "unsynchronized lyrics"    },
    { "synchronized lyrics"      },
    { "comment"                  },
    { "relative volume"          },
    { "equalization"             },
    { "reverb"                   },
    { "picture"                  },
    { "general object"           },
    { "play counter"             },
    { "popularimeter"            },
    { "recommended buffer size"  },
    { "audio encryption"         },
    { "linked information"       },
    { "position synchronization" },
    { "terms of use"             },
    { "ownership"                },
    { "commercial"               },
    { "encryption method"        },
    { "group identification"     },
    { "private"                  }};
typedef struct id3_text_table {
    const char       *desc;
    const char       *tag;
} id3_text_table;
id3_text_table default_texts[] = {
    { "invalid/unknown",   "----" }, 
    { "title",             "TIT2" },
    { "artist",            "TPE1" },
    { "track",             "TRCK" },
    { "album",             "TALB" },
    { "disc",              "TPOS" },
    { "media_type",        "TMED" },
    { "album_artist",      "TPE2" },
    { "genre",             "TCON" },
    { "year",              "TYER" },
    { "orig_year",         "TORY" },
    { "orig_artist",       "TOPE" },
    { "orig_title",        "TOAL" },
    { "composer",          "TCOM" },
    { "writer",            "TEXT" },
    { "orig_writer",       "TOLY" },
    { "key",               "TKEY" },
    { "language",          "TLAN" },
    { "publisher",         "TPUB" },
    { "encoded_by",        "TENC" },
    { "file_type",         "TFLT" },
    { "orig_filename",     "TOFN" },
    { "date",              "TDAT" },
    { "time",              "TIME" },
    { "grouping",          "TIT1" },
    { "subtitle",          "TIT3" },
    { "song_length",       "TLEN" },
    { "conductor",         "TPE3" },
    { "remixer",           "TPE4" },
    { "ISRC",              "TSRC" },
    { "encode_info",       "TSSE" },
    { "file_size",         "TSIZ" },
    { "station_name",      "TRSN" },
    { "station_owner",     "TRSO" },
    { "recording_date",    "TRDA" },
    { "owner",             "TOWN" },
    { "copyright",         "TCOP" },
    { "delay",             "TDLY" },
    { "bpm",               "TBPM" },
    { "album_artist_sort", "TSO2" },
    { "album_sort",        "TSOA" },
    { "artist_sort",       "TSOP" },
    { "title_sort",        "TSOT" },
    { "user_defined",      "TXXX" }};
typedef struct id3_url_table {
    const char       *desc;
    const char       *tag;
} id3_url_table;
id3_url_table default_urls[] = {
    { "commercial",     "WCOM" },
    { "copyright",      "WCOP" },
    { "file",           "WOAF" },
    { "artist",         "WOAR" },
    { "source",         "WOAS" },
    { "station",        "WORS" },
    { "payment",        "WPAY" },
    { "publisher",      "WPUB" },
    { "user defined",   "WXXX" }};

struct _id3cb {
    char               *buffer;
    char               *scratch;
    char               *free_scratch;
    size_t              available_scratch;
    id3_header_handler  header_handler;
    id3_frame_handler   frame_handlers [ID3_FRAME_MAX];
    id3_text_handler    text_handlers  [ID3_TEXT_MAX ];
    id3_url_handler     url_handlers   [ID3_URL_MAX  ];
    id3_final_handler   final_handler;
    size_t              position;
    int                 tag_count;
    int                 autoconvert_to_utf8;
    id3_header          header;
    id3_frame_t         ftype;
    id3_frame           frame;
};
const char *id3_get_frame_description(const id3_frame_t ftype) {
    return default_frames[ftype].desc;
}
const char *id3_get_text_description(const id3_text_t ttype) {
    return default_texts[ttype].desc;
}
const char *id3_get_url_description(const id3_url_t utype) {
    return default_urls[utype].desc;
}
int id3_final_default_handler (FILE *f, void *aux) {
    return 0;
}
static void id3_reset_scratch(ID3CB *id3) {
    memset(id3->scratch, 0, ID3_SCRATCH_SIZE);
    id3->free_scratch = id3->scratch;
    id3->available_scratch = ID3_SCRATCH_SIZE;
}
ID3CB *id3_new_parser(int flags) {
    ID3CB *id3 = calloc(1, sizeof(ID3CB));
    if ((flags & ID3_AUTOCONVERT_TO_UTF8) == ID3_AUTOCONVERT_TO_UTF8)
        id3->autoconvert_to_utf8 = 1;
    else id3->autoconvert_to_utf8 = 0;
    id3->header_handler = id3_header_default_handler;
    id3->final_handler  = id3_final_default_handler;
    for (int i = 0; i < ID3_FRAME_MAX; i++)
        (id3->frame_handlers)[i] = id3_frame_default_handler;
    for (int i = 0; i < ID3_TEXT_MAX; i++) 
        (id3->text_handlers)[i] = &id3_text_default_handler;
    for (int i = 0; i < ID3_URL_MAX; i++)
        (id3->url_handlers)[i] = &id3_url_default_handler;
    (id3->frame_handlers)[ID3_FRAME_TEXT] = &id3_frame_text_handler;
    (id3->frame_handlers)[ID3_FRAME_URL ] = &id3_frame_url_handler;
    id3->buffer = NULL;
    id3->scratch = malloc(ID3_SCRATCH_SIZE);
    id3_reset_scratch(id3);
    return id3;
}
char *id3_get_scratch(ID3CB *id3, size_t size) {
    char *result;
    if (id3->available_scratch >= size) {
        result = id3->free_scratch;
        id3->free_scratch = id3->free_scratch + size;
        id3->available_scratch -= size;
    } else result = NULL;
    return result;
}
int id3_get_autoconvert_to_utf8(ID3CB *id3) {
    return id3->autoconvert_to_utf8;
}
void id3_set_autoconvert_to_utf8(ID3CB *id3) {
    id3->autoconvert_to_utf8 = 1;
}
void id3_dispose_parser(ID3CB *id3) {
    if (id3->buffer)
        free(id3->buffer);
    if (id3->scratch)
        free(id3->scratch);
    if (id3)
        free(id3);
}
id3_frame *id3_current_frame (ID3CB *id3) {
    if (!id3) return NULL;
    return &id3->frame;
}
void id3_set_header_handler(ID3CB *id3, id3_header_handler handler) {
    if (!id3) return;
    if (handler)
        id3->header_handler = handler;
    else
        id3->header_handler = id3_header_default_handler;
}
void id3_set_final_handler (ID3CB *id3, id3_final_handler handler) {
    if (!id3) return;
    if (handler)
        id3->final_handler = handler;
    else
        id3->final_handler = id3_final_default_handler;
}
void id3_set_frame_handler(ID3CB *id3, id3_frame_t ftype, 
        id3_frame_handler handler) {
    if (!id3) return;
    if (handler)
        (id3->frame_handlers)[ftype] = handler;
    else
        (id3->frame_handlers)[ftype] = id3_frame_default_handler;
}
void id3_set_multiple_frames_handler(ID3CB *id3, id3_frame_t *ftype, 
        id3_frame_handler handler) {
// TBD
}
void id3_set_most_frames_handler(ID3CB *id3, id3_frame_handler handler) {
// TBD
}
void id3_set_text_handler (ID3CB *id3, const id3_text_t ttype, 
        id3_text_handler handler) {
    if (handler) 
        id3->text_handlers[ttype] = handler;
    else
        id3->text_handlers[ttype] = id3_text_default_handler;
}
void id3_set_multiple_texts_handler (ID3CB *id3, 
        const id3_text_t const *ttypes, 
        id3_text_handler handler) {
    int i = 0;
    if (!handler) 
        handler = id3_text_default_handler;
    while (ttypes[i]) {
        id3->text_handlers[ttypes[i]] = handler;
        i++;
    }
}
void id3_set_all_texts_handler (ID3CB *id3, 
        id3_text_handler handler) {
    if (!handler) 
        handler = id3_text_default_handler;
    for (id3_text_t i = 0; i < ID3_TEXT_MAX; i++)
        id3->text_handlers[i] = handler;
}
id3_text_handler id3_get_text_handler (ID3CB *id3, 
        const id3_text_t ttype) {
    if (ttype < ID3_TEXT_MAX)
        return id3->text_handlers[ttype];
    else return NULL;
}
void id3_set_url_handler (ID3CB *id3, 
        const id3_url_t utype, 
        id3_url_handler handler) {
    if (handler)
        id3->url_handlers[utype] = handler;
    else 
        id3->url_handlers[utype] = id3_url_default_handler;
}
void id3_set_multiple_urls_handler (ID3CB *id3, 
        const id3_url_t const *utypes, 
        id3_url_handler handler) {
    int i = 0;
    if (!handler) 
        handler = id3_url_default_handler;
    while (utypes[i]) {
        id3->url_handlers[utypes[i]] = handler;
        i++;
    }
}
void id3_set_all_urls_handler (ID3CB *id3, 
        id3_url_handler handler) {
    if (!handler) 
        handler = id3_url_default_handler;
    for (id3_url_t i = 0; i < ID3_URL_MAX; i++)
        id3->url_handlers[i] = handler;
}
id3_url_handler id3_get_url_handler (ID3CB *id3, 
        const id3_url_t utype) {
    if (utype < ID3_URL_MAX)
        return id3->url_handlers[utype];
    else return NULL;
}
void id3_parse_file (ID3CB *id3, const char *path, 
        void *aux) {
    size_t   ret;
    FILE    *f;
    if ((f = fopen(path, "r")) == NULL) {
        fprintf(stderr, "error opening file: %s\n", path);
        goto error1;
    }
    if (!id3->buffer)
        id3->buffer = malloc(14);
    ret = fread(id3->buffer, 1, 14, f);
    if (!id3_is_header(id3->buffer)) {
        fprintf(stderr, "file has no id3 tag.\n");
        goto error2;
    }
    id3->tag_count = 0;
    id3_frame_t ftype;
    size_t      tag_size, 
                remaining;
    off_t       start_offset;
    id3_get_header(&id3->header, id3->buffer);
    if ((id3->header).exthead_size != 0) {
        start_offset = 14 + (id3->header).exthead_size;
        remaining = (id3->header).tag_size;
    }
    else {
        remaining = (id3->header).tag_size;
        start_offset = 10;
    }
    (id3->header_handler)(&id3->header, aux);   
    id3->buffer = realloc(id3->buffer, start_offset + remaining);
    ret = fread(id3->buffer + 14, 1, start_offset + remaining - 14, f);
    if (ret < start_offset + remaining - 14) {
// the size of the entire file is smaller than the reported header size
// some files are valid but hold this property, therefore we won't fail or
// throw an error
    }
    id3->position = start_offset;
    while (id3->position < (id3->header).tag_size + start_offset) {
        if(id3_is_frame(id3->buffer + id3->position)) {
            id3->tag_count++;
            id3_get_frame(&id3->frame, id3->buffer + id3->position);
            ftype = id3_get_frame_type(id3->frame.tag);
            ret = (id3->frame_handlers[ftype])(id3,  aux);
            id3->position += 10 + (id3->frame).size;
        } else {
// many id3 writers appear to report a much larger tag than exists
// in this case, the next four bytes will be 0x00 padding
            goto finalize;
        }
    }
finalize:
// for the final handler, seek to the first byte
// after the tag and padding
    (id3->final_handler)(f, aux);
error2:
// see what happens if we don't free until id3_parser_dispose() is called
// so nothing to do here
error1:
    id3_reset_scratch(id3);
    fclose(f);
}
