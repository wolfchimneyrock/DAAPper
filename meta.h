#ifndef __META_H__
#define __META_H__
#include <stddef.h>
#include "dmap.h"

typedef enum codec_t {
    CODEC_MPEG = 0,
    CODEC_MP4A,
    CODEC_FLAC,
    CODEC_WMAV,
    CODEC_OGGV
} codec_t;

typedef struct meta_tag_t {
    char    *meta_tag;
    dmap_t   type;
    char    *tag;
    char    *db_column;
    size_t   ofs;
} meta_tag_t;


typedef struct meta_info_t {
    int id,
        kind,
        data_kind,
        media_kind,
        parent_id,
        containeritem_id,
        special_id,
        file_size,
        song_length,
        total_tracks,
        track,
        year,
        year_orig,
        bitrate,
        samplerate,
        bpm,
        disabled,
        has_video,
        compilation,
        time_added,
        time_modified,
        rating,
        total_discs,
        disc;
    long persistent_id,
         songartistid,
         songalbumid;
    char *title,
         *album,
         *artist,
         *album_artist,
         *composer,
         *genre,
         *grouping,
         *type,
         *url,
         *comment,
         *description,
         *codectype,
         *title_sort,
         *artist_sort,
         *album_sort,
         *album_artist_sort,
         *publisher;
} meta_info_t;

#define MOFS(x) offsetof(meta_info_t, x)

extern const meta_tag_t item_meta_tags[];
extern const meta_tag_t group_meta_tags[];
extern const meta_tag_t container_meta_tags[];
extern const meta_tag_t misc_meta_tags[];

extern const char *default_meta_container[];
extern const char *default_meta_items[];
extern const char *default_meta_group[];

extern const char *codec_strings[];
extern const char *type_strings[];
extern const char *desc_strings[];

int meta_find_item_tag(const char *s);
int meta_find_group_tag(const char *s);
int meta_find_container_tag(const char *s);
void meta_parse(char *raw, char ***meta, int *nmeta); 

#endif
