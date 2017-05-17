#include "id3cb.h"
#include "url.h"


int id3_frame_url_handler(ID3CB *id3, void *aux) {
    id3_frame *frame = id3_current_frame(id3);
    id3_url    contents;
    contents.type = id3_get_url_frame_type(frame);
    if (contents.type == ID3_URL_USER) {
        contents.encoding = (id3_enc_t)frame->data[0];
        contents.desc = (char *)(frame->data + 1);

        // TBD: find end of description for ISO, UTF16, UTF8
        if (contents.encoding == ID3_ENC_ISO ||
            contents.encoding == ID3_ENC_UTF8) {
            // simple string length should work
        }
        else {
            // UTF16 string length function required
        }
    } else {
        contents.desc = (char *)id3_get_url_description(contents.type);
        contents.encoding = ID3_ENC_ISO;
        contents.data = frame->data;    
    }
    id3_url_handler handler = id3_get_url_handler(id3, contents.type);
    return handler ? handler(&contents, aux) : -1;
}

int id3_url_default_handler(const id3_url *url, void *aux) {
    return 0;
}

id3_url_t id3_get_url_frame_type (const id3_frame *frame) {

}
