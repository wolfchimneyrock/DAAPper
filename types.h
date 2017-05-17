#ifndef __ID3_TYPES_H__
#define __ID3_TYPES_H__
typedef struct _id3cb ID3CB;
typedef enum id3_enc_t {
    ID3_ENC_ISO = 0,         ///< ISO-8859-1
    ID3_ENC_UTF16,           ///< UTF-16 with BOM: FFFE=BE, FEFF=LE
    ID3_ENC_UTF16BE,         ///< UTF-16 big-ending without BOM
    ID3_ENC_UTF8             ///< UTF-8
} id3_enc_t;
extern const char *id3_text_encodings[];
typedef enum id3_frame_t {
    ID3_FRAME_INVALID = 0,
    ID3_FRAME_UNIQUE,        ///< Unique identifier
    ID3_FRAME_TEXT,          ///< Text
    ID3_FRAME_URL,           ///< Official URLs
    ID3_FRAME_IPLS,          ///< Involved players 
    ID3_FRAME_MCDI,          ///< Music CD identifier
    ID3_FRAME_ETCO,          ///< Event timing code
    ID3_FRAME_MLLT,          ///< MPEG location lookup table
    ID3_FRAME_SYTC,          ///< Synchronized time codes
    ID3_FRAME_USLT,          ///< Unsynchronized lyrics
    ID3_FRAME_SYLT,          ///< Synchronized lyrics
    ID3_FRAME_COMMENT,       ///< Comment
    ID3_FRAME_RVOLUME,       ///< Relative volume
    ID3_FRAME_EQ,            ///< Equalization
    ID3_FRAME_REVERB,        ///< Reverb
    ID3_FRAME_PICTURE,       ///< Picture
    ID3_FRAME_GENERAL,       ///< General encapsulation object
    ID3_FRAME_PLAYCOUNT,     ///< Play counter
    ID3_FRAME_POPULARITY,    ///< Popularimeter
    ID3_FRAME_BUFFERSIZE,    ///< Recommended buffer size
    ID3_FRAME_ENCRYPTION,    ///< Audio Encryption
    ID3_FRAME_LINK,          ///< Linked information
    ID3_FRAME_POSITION,      ///< Position synchronization
    ID3_FRAME_TERMS,         ///< Terms of use
    ID3_FRAME_OWNERSHIP,     ///< Ownership
    ID3_FRAME_COMMERCIAL,    ///< Commercial
    ID3_FRAME_ENCRYPTREG,    ///< Encryption method registration
    ID3_FRAME_GROUPID,       ///< Group Identification
    ID3_FRAME_PRIVATE,       ///< Private
    ID3_FRAME_MAX
} id3_frame_t;
typedef enum id3_text_t {
    ID3_TEXT_INVALID = 0,
    ID3_TEXT_TITLE,             ///< daap.itemname
    ID3_TEXT_ARTIST,            ///< daap.songartist
    ID3_TEXT_TRACK,             ///< daap.songtracknumber / count
    ID3_TEXT_ALBUM,             ///< daap.songalbum
    ID3_TEXT_DISC,              ///< daap.songdiscnumber / count
    ID3_TEXT_MEDIA_TYPE,
    ID3_TEXT_ALBUM_ARTIST,      ///< daap.songalbumartist
    ID3_TEXT_GENRE,             ///< daap.songgenre
    ID3_TEXT_YEAR,              ///< daap.songyear
    ID3_TEXT_ORIG_YEAR,         ///< """
    ID3_TEXT_ORIG_ARTIST,       
    ID3_TEXT_ORIG_TITLE,        
    ID3_TEXT_COMPOSER,          ///< daap.songcomposer
    ID3_TEXT_SONGWRITER,
    ID3_TEXT_ORIG_SONGWRITER,
    ID3_TEXT_KEY,
    ID3_TEXT_LANGUAGE,
    ID3_TEXT_PUBLISHER,
    ID3_TEXT_ENCODED_BY,
    ID3_TEXT_FILE_TYPE,
    ID3_TEXT_ORIG_FILENAME,
    ID3_TEXT_DATE,
    ID3_TEXT_TIME,
    ID3_TEXT_GROUPING,
    ID3_TEXT_SUBTITLE,
    ID3_TEXT_SONG_LENGTH,       ///< daap.songtime
    ID3_TEXT_CONDUCTOR,
    ID3_TEXT_REMIXER,
    ID3_TEXT_ISRC,
    ID3_TEXT_ENCODE_INFO,
    ID3_TEXT_FILE_SIZE,         ///< daap.songsize
    ID3_TEXT_STATION_NAME,
    ID3_TEXT_STATION_OWNER,
    ID3_TEXT_RECORDING_DATE,
    ID3_TEXT_OWNER,
    ID3_TEXT_COPYRIGHT,
    ID3_TEXT_DELAY,
    ID3_TEXT_BPM,               ///< daap.songbeatsperminute
    ID3_TEXT_SORT_ALBUM_ARTIST, ///< daap.sortalbumartist
    ID3_TEXT_SORT_ALBUM,        ///< daap.sortalbum
    ID3_TEXT_SORT_ARTIST,       ///< daap.sortartist
    ID3_TEXT_SORT_TITLE,        ///< daap.sortname
    ID3_TEXT_USER_DEFINED,
    ID3_TEXT_MAX
} id3_text_t;
typedef enum id3_url_t {
    ID3_URL_COMMERCIAL,
    ID3_URL_COPYRIGHT,
    ID3_URL_FILE,
    ID3_URL_ARTIST,
    ID3_URL_SOURCE,
    ID3_URL_STATION,
    ID3_URL_PAYMENT,
    ID3_URL_PUBLISHER,
    ID3_URL_USER,
    ID3_URL_MAX
} id3_url_t;
#endif
