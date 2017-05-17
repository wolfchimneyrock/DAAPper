#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <iconv.h>
#include "id3cb.h"
#include "text.h"



/**
 * @brief the handler for frametype text
 *
 * this will call the specific text type handler that has been previously registered
 *
 * @param ftype  the type of frame
 * @param frame  a pointer to the frame structure
 * @param aux    a user supplied data structure for use within the callback
 *
 */
int id3_frame_text_handler (ID3CB *id3, void *aux) {
    id3_frame *frame  = id3_current_frame(id3);
    id3_text   contents;
    contents.type = id3_get_text_frame_type(frame);
    memcpy(contents.tag, frame->tag, 4);
    contents.tag[4] = '\0';

    // the frame size doesn't count the 10 byte header
    // subtract one for the encoding byte
    contents.size   = frame->size - 1; 
    contents.encoding = (id3_enc_t)frame->data[0]; 
    switch (contents.encoding) {
        case ID3_ENC_ISO:
        case ID3_ENC_UTF8: {
            // for these encodings we can simply allocate and copy
            // and be sure to add a terminating \0
            contents.data = id3_get_scratch(id3, contents.size + 1);
            memcpy(contents.data, frame->data + 1, contents.size);
            contents.data[contents.size] = '\0';
            contents.size += 1;
        } break;
        default: {
            // for UTF-16 encodings we have more work:
            // if autoconvert is enabled, convert it to UTF-8
            contents.data = id3_get_scratch(id3, contents.size + 2);
            if (id3_get_autoconvert_to_utf8(id3)) {
                iconv_t utf16_to_utf8 = iconv_open("UTF-8", "UTF-16");
                char *in_p = frame->data + 1;
                char *out_p = contents.data;
                size_t out_size = contents.size + 2;
                size_t in_size  = contents.size;
                iconv(utf16_to_utf8, &in_p, &in_size, &out_p, &out_size);
                iconv_close(utf16_to_utf8);
                contents.encoding = ID3_ENC_UTF8;
            } else
                memcpy(contents.data, frame->data + 1, contents.size);
            contents.size += 2;
        }
    }
    // for TXXX tags, the first part of the data is a null terminated
    // description string, followed by the actual field contents.
    if (contents.type == ID3_TEXT_USER_DEFINED) {
        contents.desc = contents.data;
        switch (contents.encoding) {
            case ID3_ENC_ISO:
            case ID3_ENC_UTF8: {
                int l = strlen(contents.data);
                contents.data += l + 1;
                } break;               
            default: {
                uint16_t *p = (uint16_t *)contents.data;
                while (*p++);
                ++p;
                contents.data = (char *)p;
                     }
        }
    } else // default to pulling the description from our table
        contents.desc = (char *)id3_get_text_description(contents.type);

    id3_text_handler handler = id3_get_text_handler(id3, contents.type);
    int ret = -1;
    if (handler)
        ret = handler(&contents, aux);
    return ret;
}

int id3_text_default_handler (const id3_text *text, void *aux) {
    // default action is to ignore - so do nothing
    return 0;
}

id3_text_t id3_get_text_frame_type(const id3_frame *frame) {
    if (frame->tag[0] != 'T') 
        return ID3_TEXT_INVALID;
    switch(frame->tag[1]) {
        case 'A': return ID3_TEXT_ALBUM;
        case 'B': return ID3_TEXT_BPM;
        case 'C':
            switch (frame->tag[3]) {
                case 'M': return ID3_TEXT_COMPOSER;
                case 'N': return ID3_TEXT_GENRE;
                case 'P': return ID3_TEXT_COPYRIGHT;
                default:  return ID3_TEXT_INVALID;
            }
        case 'D':
            switch (frame->tag[2]) {
                case 'A': return ID3_TEXT_DATE;
                case 'L': return ID3_TEXT_DELAY;
                default:  return ID3_TEXT_INVALID;
            }
        case 'E':
            switch (frame->tag[2]) {
                case 'N': return ID3_TEXT_ENCODED_BY;
                case 'X': return ID3_TEXT_SONGWRITER;
                default:  return ID3_TEXT_INVALID;
            }
        case 'F': return ID3_TEXT_FILE_TYPE;
        case 'I':
            switch (frame->tag[3]) {
                case 'E': return ID3_TEXT_TIME;
                case '1': return ID3_TEXT_GROUPING;
                case '2': return ID3_TEXT_TITLE;
                case '3': return ID3_TEXT_SUBTITLE;
                default:  return ID3_TEXT_INVALID;
            }
        case 'K': return ID3_TEXT_KEY;
        case 'L':
            switch (frame->tag[2]) {
                case 'A': return ID3_TEXT_LANGUAGE;
                case 'E': return ID3_TEXT_SONG_LENGTH;
                default:  return ID3_TEXT_INVALID;
            }
        case 'M': return ID3_TEXT_MEDIA_TYPE;
        case 'O':
            switch (frame->tag[2]) {
                case 'A': return ID3_TEXT_ORIG_TITLE;
                case 'F': return ID3_TEXT_ORIG_FILENAME;
                case 'L': return ID3_TEXT_ORIG_SONGWRITER;
                case 'P': return ID3_TEXT_ORIG_ARTIST;
                case 'R': return ID3_TEXT_ORIG_YEAR;
                default:  return ID3_TEXT_INVALID;
            }
        case 'P':
            switch (frame->tag[3]) {
                case '1': return ID3_TEXT_ARTIST;
                case '2': return ID3_TEXT_ALBUM_ARTIST;
                case '3': return ID3_TEXT_CONDUCTOR;
                case '4': return ID3_TEXT_REMIXER;
                case 'S': return ID3_TEXT_DISC;
                case 'B': return ID3_TEXT_PUBLISHER;
                default:  return ID3_TEXT_INVALID;
            }
        case 'R':
            switch (frame->tag[3]) {
                case 'K': return ID3_TEXT_TRACK;
                case 'A': return ID3_TEXT_RECORDING_DATE;
                case 'N': return ID3_TEXT_STATION_NAME;
                case 'O': return ID3_TEXT_STATION_OWNER;
                default:  return ID3_TEXT_INVALID;
            }
        case 'S':
            switch (frame->tag[2]) {
                case 'I': return ID3_TEXT_FILE_SIZE;
                case 'O': 
                    switch (frame->tag[3]) {
                        case '2': return ID3_TEXT_SORT_ALBUM_ARTIST;
                        case 'A': return ID3_TEXT_SORT_ALBUM;
                        case 'P': return ID3_TEXT_SORT_ARTIST;
                        case 'T': return ID3_TEXT_SORT_TITLE;
                        default:  return ID3_TEXT_INVALID;
                    }
                case 'R': return ID3_TEXT_ISRC;
                case 'S': return ID3_TEXT_ENCODE_INFO;
                default:  return ID3_TEXT_INVALID;
            }
        case 'X': return ID3_TEXT_USER_DEFINED;
        case 'Y': return ID3_TEXT_YEAR;

    }
}
