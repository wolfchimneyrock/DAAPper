#include <stdlib.h>
#include <string.h>
#include "meta.h"
#include "util.h"

const char *default_meta_container[] = {
    "dmap.itemid",
    "dmap.itemname",
//  "dmap.containeritemid",
//  "dmap.parentcontainerid",
//  "dmap.specialid",
    NULL
};

const char *default_meta_group[] = {
    "dmap.itemkind",
    "dmap.itemid",
    "dmap.itemname",
    "dmap.persistentid",
    NULL
};


const char *default_meta_items[] = {
    "dmap.itemkind",
    "daap.songdatakind",
    "com.apple.itunes.mediakind",
    "daap.songalbum",
    "daap.songartist",
    "daap.songbeatsperminute",
    "daap.songbitrate",
    "daap.songdateadded",
    "daap.songdatemodified",
    "daap.songdisccount",
    "daap.songdiscnumber",
    "dmap.itemid",
    "daap.songformat",
    "daap.songdescription",
    "dmap.itemname",
    "daap.songsamplerate",
    "daap.songsize",
    "daap.songtime",
    "daap.songtrackcount",
    "daap.songtracknumber",
    "daap.songuserrating",
    "daap.songyear",
    "daap.songcodectype",
    "dmap.containeritemid",
    "com.apple.itunes.has-video",

    "dmap.persistentid",
    "daap.songcomment",
    "daap.songcompilation",
    "daap.songcomposer",
    "daap.songalbumartist",
    "daap.songdisabled",
    "daap.songalbumid",
    "daap.songartistid",
    "daap.songgenre",
    "daap.songdiscnumber",
    "daap.sortartist",
    "daap.sortalbum",

    NULL
};

const char *codec_strings[] = {
/* CODEC_MPEG */ "mpeg",
/* CODEC_MP4A */ "mp4a",
/* CODEC_FLAC */ "flac",
/* CODEC_WMAV */ "wmav",
/* CODEC_OGGV */ "ogg",
                 NULL
};

const char *type_strings[] = {
/* CODEC_MPEG */ "mp3",
/* CODEC_MP4A */ "m4a",
/* CODEC_FLAC */ "flac",
/* CODEC_WMAV */ "wma",
/* CODEC_OGGV */ "ogg",
                 NULL
};

const char *desc_strings[] = {
/* CODEC_MPEG */ "MPEG audio file",
/* CODEC_MP4A */ "AAC audio file",
/* CODEC_FLAC */ "FLAC audio file",
/* CODEC_WMAV */ "WMA audio file",
/* CODEC_OGGV */ "Ogg Vorbis audio file",
                 NULL
};

const meta_tag_t item_meta_tags[] = {
    // dmap.itemkind must be first entry!!
{ "dmap.itemkind",          T_CHAR,   "mikd", 
    "f.item_kind",     MOFS(kind)               },
{ "daap.songdatakind",      T_CHAR,   "asdk", 
    "f.data_kind",     MOFS(data_kind)          },
{ "dmap.itemid",            T_INT,    "miid", 
    "s.id",            MOFS(id)                 },
{ "dmap.itemname",          T_STRING, "minm", 
    "s.title",         MOFS(title)              },
{ "daap.songcomment",       T_STRING, "ascm", 
    "s.comment",       MOFS(comment)            },
{ "daap.songalbum",         T_STRING, "asal", 
    "al.album",        MOFS(album)              },
{ "daap.songalbumid",       T_LONG,   "asai", 
    "f.songalbumid",   MOFS(songalbumid)        },
{ "daap.songartistid",      T_LONG,   "asri", 
    "f.songartistid",  MOFS(songartistid)       },
{ "com.apple.itunes.itms-artistid", T_LONG, "aeAI", 
    "f.songartistid",  MOFS(songartistid)  },
{ "daap.songartist",        T_STRING, "asar", 
    "ar.artist",       MOFS(artist)             },
{ "daap.songgenre",         T_STRING, "asgn", 
    "g.genre",         MOFS(genre)              },
{ "daap.songgrouping",      T_STRING, "asgr", 
    "f.grouping",      MOFS(grouping)           },
{ "daap.songsize",          T_INT,    "assz", 
    "s.file_size",     MOFS(file_size)          },
{ "daap.songtime",          T_INT,    "astm", 
    "s.song_length",   MOFS(song_length)        },
{ "daap.songtrackcount",    T_SHORT,  "astc", 
    "al.total_tracks", MOFS(total_tracks)       },
{ "daap.songtracknumber",   T_SHORT,  "astn", 
    "s.track",         MOFS(track)              },
{ "daap.songyear",          T_SHORT,  "asyr", 
    "al.year",         MOFS(year)               },
{ "daap.songformat",        T_STRING, 
    "asfm", "c.codectype",     MOFS(type)               },
{ "daap.songbitrate",       T_SHORT,  "asbr", 
    "s.bitrate",       MOFS(bitrate)            },
{ "daap.songdiscnumber",    T_SHORT,  "asdn", 
    "s.disc",          MOFS(disc)               },
{ "daap.songdisccount",     T_SHORT,  "asdc", 
    "al.total_discs",  MOFS(total_discs)        },
{ "daap.songcompilation",   T_CHAR,   "asco", 
    "al.compilation",  MOFS(compilation)        },
{ "daap.songdataurl",       T_STRING, "asul", 
    "s.url",           MOFS(url)                },
{ "daap.songuserrating",    T_CHAR,   "asur", 
    "s.rating",        MOFS(rating)             },
{ "daap.sortartist",        T_STRING, "assa", 
    "ar.artist_sort",  MOFS(artist_sort)        },
{ "daap.songsamplerate",    T_INT,    "assr", 
    "s.samplerate",    MOFS(samplerate)         },
{ "daap.songcodectype",     T_STRING, "ascd", 
    "c.codectype",     MOFS(codectype)          },
{ "daap.sortalbum",         T_STRING, "assu", 
    "al.album_sort",   MOFS(album_sort)         },
{ "daap.songalbumartist",   T_STRING, "asaa", 
    "al.album_artist", MOFS(album_artist)       },
{ "daap.songdisabled",      T_INT,    "asdb", 
    "f.disabled",      MOFS(disabled)           },
{ "daap.songdescription",   T_STRING, "asdt", 
    "f.description",   MOFS(description)        },
{ "daap.songcomposer",      T_STRING, "ascp", 
    "s.composer",      MOFS(composer)           },
{ "daap.songbeatsperminute",T_SHORT,  "asbt", 
    "s.bpm",           MOFS(bpm)                },
{ "dmap.persistentid",      T_LONG,   "mper", 
    "f.id",            MOFS(persistent_id)      },
{ "com.apple.itunes.has-video", T_CHAR, "aeHV", 
    "s.has_video",     MOFS(has_video)          },
{ "com.apple.itunes.mediakind", T_CHAR, "aeMK", 
    "f.media_kind",    MOFS(media_kind)         },
{ "dmap.containeritemid",   T_INT,    "mcti", 
    "s.id",            MOFS(id)                 },
{ "daap.songdateadded",     T_DATE,   "asda", 
    "s.time_added",    MOFS(time_added)         },
{ "daap.songdatemodified",  T_DATE,   "asdm", 
    "s.time_modified", MOFS(time_modified)      },
{ NULL, 0, NULL, NULL, 0 }
};

const meta_tag_t container_meta_tags[] = {
// dmap.itemkind must be first entry!!
{ "dmap.itemkind",          T_CHAR,   "mikd", 
    "p.type",         MOFS(kind)             },
{ "dmap.itemid",            T_INT,    "miid", 
    "p.id",           MOFS(id)               },
{ "dmap.itemname",          T_STRING, "minm", 
    "p.title",        MOFS(title)            },
{ "dmap.containeritemid",   T_INT,    "mcti", 
    "p.id",           MOFS(containeritem_id) },
{ "dmap.parentcontainerid", T_INT,    "mpco", 
    "p.parent_id",    MOFS(parent_id)        },
{ "dmap.specialid",         T_CHAR,   "aePS", 
    "p.special_id",   MOFS(special_id)       },   
{ "dmap.persistentid",      T_LONG,   "mper", 
    "p.id",           MOFS(persistent_id)    },
{ NULL, 0, NULL, NULL, 0 }
};

const meta_tag_t group_meta_tags[] = {
// dmap.itemkind must be first entry!!
{ "dmap.itemkind",          T_CHAR,   "mikd", 
    "g.type",         MOFS(kind)             },
{ "dmap.itemid",            T_INT,    "miid", 
    "g.id",           MOFS(id)               },
{ "dmap.itemname",          T_STRING, "minm", 
    "g.name",         MOFS(title)            },
{ "dmap.persistentid",      T_LONG,   "mper", 
    "g.persistentid", MOFS(persistent_id)    },
{ NULL, 0, NULL, NULL, 0 }
};
const meta_tag_t misc_meta_tags[] = {
{ "dmap.serverinforesponse",                T_LIST,   "msrv", NULL, -1 },
{ "dmap.status",                            T_INT,    "mstt", NULL, -1 },
{ "dmap.statusstring",                      T_STRING, "msts", NULL, -1 },
{ "dmap.protocolversion",                   T_VER,    "mpro", NULL, -1 },
{ "dmap.itemname",                          T_STRING, "minm", NULL, -1 },
{ "daap.protocolversion",                   T_VER,    "apro", NULL, -1 },
{ "com.apple.itunes.music-sharing-version", T_INT,    "aeSV", NULL, -1 },
{ "dmap.timeoutinterval",                   T_INT,    "mstm", NULL, -1 },
{ "dmap.authenticationmethod",              T_CHAR,   "msau", NULL, -1 }, 
{ "dmap.supportsupdate",                    T_CHAR,   "msup", NULL, -1 },
{ "dmap.supportspersistentids",             T_CHAR,   "mspi", NULL, -1 },
{ "dmap.supportsextensions",                T_CHAR,   "msex", NULL, -1 },
{ "dmap.supportsbrowse",                    T_CHAR,   "msbr", NULL, -1 },
{ "dmap.supportsquery",                     T_CHAR,   "msqy", NULL, -1 },
{ "dmap.supportsindex",                     T_CHAR,   "msix", NULL, -1 },
{ "dmap.databasecount",                     T_INT,    "msdc", NULL, -1 },
{ "dmap.contentcodesresponse",              T_LIST,   "mccr", NULL, -1 },
{ "dmap.dictionary",                        T_LIST,   "mdcl", NULL, -1 },
{ "dmap.contentcodesnumber",                T_INT,    "mcnm", NULL, -1 },
{ "dmap.contentcodesname",                  T_STRING, "mcna", NULL, -1 },
{ "dmap.contentcodestype",                  T_SHORT,  "mcty", NULL, -1 },
{ "dmap.itemcount",                         T_INT,    "mimc", NULL, -1 },
{ "dmap.containercount",                    T_INT,    "mctc", NULL, -1 },
{ "dmap.listingitem",                       T_LIST,   "mlit", NULL, -1 },
{ "com.apple.itunes.extended-media-kind",   T_INT,    "aeMk", NULL, -1 },
{ "daap.serverdatabases",                   T_LIST,   "avdb", NULL, -1 },
{ "dmap.updatetype",                        T_CHAR,   "muty", NULL, -1 },
{ "dmap.specifiedtotalcount",               T_INT,    "mtco", NULL, -1 },
{ "dmap.returnedcount",                     T_INT,    "mrco", NULL, -1 },
{ "dmap.listing",                           T_LIST,   "mlcl", NULL, -1 },
{ "dmap.loginresponse",                     T_LIST,   "mlog", NULL, -1 },
{ "dmap.sessionid",                         T_INT,    "mlid", NULL, -1 },
{ "dmap.updateresponse",                    T_LIST,   "mupd", NULL, -1 },
{ "dmap.serverrevision",                    T_INT,    "musr", NULL, -1 },
{ "daap.baseplaylist",                      T_CHAR,   "abpl", NULL, -1 },
{ "com.apple.itunes.smart-playlist",        T_CHAR,   "aeSP", NULL, -1 },
{ "daap.databaseplaylists",                 T_LIST,   "aply", NULL, -1 },
{ "daap.playlistsongs",                     T_LIST,   "apso", NULL, -1 },
{ "daap.databasesongs",                     T_LIST,   "adbs", NULL, -1 },
{ "daap.databasebrowse",                    T_LIST,   "abro", NULL, -1 },
{ "daap.browsealbumlisting",                T_LIST,   "abal", NULL, -1 },
{ "daap.browseartistlisting",               T_LIST,   "abar", NULL, -1 },
{ "daap.browsecomposerlisting",             T_LIST,   "abcp", NULL, -1 },
{ "daap.browsegenrelisting",                T_LIST,   "abgn", NULL, -1 },
{ NULL, 0, NULL, NULL, 0}
};

/**
 * @brief return the array index of the specified meta tag.
 * TBD: use a hash table
 */
int meta_find_item_tag(const char *s) {
    int i = -1;
    while (item_meta_tags[++i].meta_tag)
        if (strcmp(item_meta_tags[i].meta_tag, s) == 0) return i;
    return -1;
}

int meta_find_container_tag(const char *s) {
    int i = -1;
    while (container_meta_tags[++i].meta_tag)
        if (strcmp(container_meta_tags[i].meta_tag, s) == 0) return i;
    return -1;
}
int meta_find_group_tag(const char *s) {
    int i = -1;
    while (group_meta_tags[++i].meta_tag)
        if (strcmp(group_meta_tags[i].meta_tag, s) == 0) return i;
    return -1;
}

/**
 * @brief given a comma separated string of tags, return an array of strings
 *        that is the list of tags
 */
void meta_parse(char *raw, char ***meta, int *nmeta) {
    int i = 0, n = 0;
    int len = count_char(raw, ',') + 2;
    *meta = malloc(len * sizeof(char *));
    (*meta)[n] = raw;
    *nmeta = len - 1;
    while(raw[i]) 
        if (raw[i] == ',') {
            raw[i] = '\0';
            (*meta)[++n] = raw + ++i;
        } else ++i;
    (*meta)[++n] = NULL;
}
