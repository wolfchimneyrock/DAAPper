#ifndef __SQL_H__
#define __SQL_H__
#include <sqlite3.h>
#include <event2/buffer.h>
#include <evhtp/evhtp.h>
#include "sqlite3ext.h"
#include "config.h"
#include "meta.h"
#include "vector.h"
#include "scratch.h"
#include "util.h"

#define STR(x) (x ? x : "")
#define EMPTY_STRLIST { 0 }

int sqlite3_closure_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *aApi);
// t_type should be in order of key dependency
typedef enum t_type {
    T_ARTISTS = 0,
    T_PUBLISHERS,
    T_ALBUMS,
    T_CODECS,
    T_PATHS,
    T_PATHTREE,
    T_GENRES,
    T_SONGS,
    T_PLAYLISTS,
    T_PLAYLISTITEMS,
    T_TEMP,
    T_PLAYS,
    T_MAX,
    T_GROUPS,
    T_INOTIFY,
    T_PAIRINGS,
    T_SPEAKERS,
    T_QUEUE,
    T_GROUP_ITEMS,
} t_type;

typedef enum q_type {
    Q_GET_PATH = 0,
    Q_COUNT_ALL_SONGS,
    Q_COUNT_ALL_PLAYLISTS,
    Q_COUNT_ALL_ARTISTS,
    Q_COUNT_ALL_ALBUMS,
    Q_COUNT_PL_ITEMS,
    Q_COUNT_ALBUM_ITEMS,
    Q_COUNT_ARTIST_ITEMS,
    Q_SMARTPL_QUERY,
    Q_ALBUM_ARTIST,
    Q_REMOVE_PATH,
    Q_REMOVE_SONG,
    Q_PLAYCOUNT_INC,
    Q_CHANGE_PATH,
    Q_UPSERT_PATH,
    Q_UPSERT_ARTIST,
    Q_UPSERT_PUBLISHER,
    Q_UPSERT_ALBUM,
    Q_UPSERT_SONG,
    Q_UPSERT_GENRE,
    Q_GET_RETURN,
    Q_CLEAR_RETURN,
    Q_GET_SONG_PATH_WITH_PARENT,
    Q_GET_SONG_FROM_PATH1,
    Q_GET_SONG_FROM_PATH2,
    Q_GET_SONG_FROM_PATH3,
    Q_GET_SONG_FROM_PATH4,
    Q_FIND_ARTIST,
    Q_FIND_PUBLISHER,
    Q_FIND_GENRE,
    Q_FIND_ALBUM,
    Q_FIND_SONG,
    Q_BEGIN_TRANSACTION,
    Q_END_TRANSACTION,
    Q_PRECOMPILED_MAX,

    Q_ITEMLIST,
    Q_CONTAINERLIST,
    Q_CONTAINERITEMS,
    Q_GROUPLIST,
    Q_COUNT_SMART,
    Q_MAX             
} q_type;

typedef enum g_type {
    G_NONE   ,
    G_ALBUM  ,
    G_ARTIST 
} g_type;

typedef enum pl_type {
    PL_SPECIAL = 0,
    PL_FOLDER,
    PL_SMART,
    PL_PLAIN
} pl_type;

typedef enum q_sort {
    S_NONE        ,
    S_NAME        ,
    S_ALBUM       ,
    S_ARTIST      ,
    S_PLAYLIST    
} q_sort;

typedef enum q_limit {
    L_NONE  ,
    L_FIRST ,
    L_LAST  ,
    L_MID   
} q_limit;

typedef enum r_type {
    R_NONE = 0,
    R_INT,
    R_STR
} r_type;

typedef struct query_t {
    q_type   type;
    r_type   returns;
    uint64_t created;
    int      place;
    int     n_str, n_int, n_int64;;
    int     *intvals;
    int64_t *int64vals;
    char    **strvals;
    char    **intcols;
    char    **strcols;
} query_t;

typedef struct sql_t {
    const char *name,
               *query;
} sql_t;

// thread-pooling using libevhtp thread-design.c as a template

extern const char *query_strings[];
extern const char *table_strings[];
extern const sql_t queries[];
extern const sql_t tables[];

int db_open_database  (app *aux, int flags);
int db_close_database (app *aux);
void precompile_statements(void *arg); 

int db_find_artist    (app *aux, const char *artist);
int db_find_publisher (app *aux, const char *publisher);
int db_find_genre     (app *aux, const char *genre);
int db_find_album     (app *aux, const char *album, int artist, int year);

size_t count_all_files         (app *aux);
int    db_find_path            (app *aux, const char *path, SCRATCH *s);
int db_find_path_with_parent(app *aux, const char *path, const int parent, SCRATCH *s);
size_t count_precompiled_items (app *aux, q_type q, int *bindvar);
size_t count_smart_items       (app *aux, t_type table, 
                                vector *clauses, int *bindvar); 

const char *get_smart_playlist_query (app *aux, int playlist); 
const char *get_albumgroup_artist    (app *aux, int id); 

int sql_put_results(struct evbuffer **dest, 
                    struct evbuffer **items, 
                    app *aux, 
                    vector *cols, 
                    int start_col, 
                    const char *sqlstr, 
                    int *bindvar, 
                    int(*item_cb)(app *aux, 
                                  struct evbuffer *item, 
                                  meta_info_t *info
                                  )
                    );
void sql_build_query(query_t *q, vector *columns, 
                     vector *clauses, char **output);

void sql_delete_file(sqlite3 *db, const char *path);
#endif
