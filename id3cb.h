#ifndef __ID3CB_H__
#define __ID3CB_H__
#include <stdio.h>
#include "types.h"
#include "header.h"
#include "frame.h"
#include "text.h"
#include "url.h"

#define DECODE_UINT32BE(buf, val) \
    do { val = ((*(buf + 0) & 0xff) * 16777216  ) | \
               ((*(buf + 1) & 0xff) * 65536     ) | \
               ((*(buf + 2) & 0xff) * 256       ) | \
               ((*(buf + 3) & 0xff));               \
       } while (0)

#define ID3_HAS_EXTENDED (1 << 6)
#define ID3_AUTOCONVERT_TO_UTF8 1

ID3CB *id3_new_parser(int flags);
char *id3_get_scratch(ID3CB *id3, size_t size);
void id3_dispose_parser(ID3CB *id3);
id3_frame *id3_current_frame (ID3CB *id3);
const char *id3_get_frame_description(const id3_frame_t ftype);
const char *id3_get_text_description(const id3_text_t ttype);
const char *id3_get_url_description(const id3_url_t utype);
typedef int (*id3_final_handler)(FILE *, void *);
void id3_set_header_handler (ID3CB *id3, id3_header_handler handler);
void id3_set_final_handler  (ID3CB *id3, id3_final_handler  handler);
void id3_set_frame_handler  (ID3CB *id3, id3_frame_t ftype, 
                             id3_frame_handler handler);
void id3_set_multiple_frames_handler (ID3CB *id3, id3_frame_t *ftypes, 
                                      id3_frame_handler handler);
void id3_set_all_frames_handler (ID3CB *id3, id3_frame_handler handler);
id3_frame_handler id3_get_frame_handler (ID3CB *id3, 
                                         const id3_frame_t ftype);
void id3_set_text_handler (ID3CB *id3, const id3_text_t ttype, 
                           id3_text_handler handler);
void id3_set_multiple_texts_handler (ID3CB *id3, 
                                     const id3_text_t const *ttypes, 
                                     id3_text_handler handler);
void id3_set_all_texts_handler (ID3CB *id3, 
                                id3_text_handler handler);
id3_text_handler id3_get_text_handler (ID3CB *id3, 
                                       const id3_text_t ttype);
void id3_set_url_handler (ID3CB *id3, 
                          const id3_url_t ttype, 
                          id3_url_handler handler);
void id3_set_multiple_urls_handler (ID3CB *id3, 
                                    const id3_url_t *utypes, 
                                    id3_url_handler handler);
void id3_set_all_urls_handler (ID3CB *id3, id3_url_handler handler);
id3_url_handler id3_get_url_handler (ID3CB *id3, const id3_url_t utype);
int id3_get_autoconvert_to_utf8(ID3CB *id3);
void id3_parse_file (ID3CB *id3, const char *path, void *aux);
void id3_dispose_reader (ID3CB *id3);
#endif
