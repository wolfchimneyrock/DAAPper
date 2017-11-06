#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <syslog.h>
#include <event2/util.h>
#include <evhtp/evhtp.h>
#include <time.h>
#include "vector.h"
#include "sql.h"
#include "meta.h"
#include "util.h"

const sql_t queries[] = {
    { "Q_GET_PATH",
        "SELECT group_concat(path, '/')  "\
        "FROM   paths p, pathtree pt "\
        "WHERE  pt.id = (SELECT path from songs where id = ?) "\
        "AND    p.id = pt.root;"
    }, 
    { "Q_COUNT_ALL_SONGS",
        "SELECT COUNT(*) "\
        "FROM   songs;"
    },
    { "Q_COUNT_ALL_PLAYLISTS",
        "SELECT COUNT(*) "\
        "FROM   playlists;"
    },
    { "Q_COUNT_ALL_ARTISTS",
        "SELECT COUNT(*) - 1 "\
        "FROM   artists;"
    },
    { "Q_COUNT_ALL_ALBUMS",
        "SELECT COUNT(*) - 1 "\
        "FROM   albums;"
    },
    { "Q_COUNT_PL_ITEMS",
        "SELECT COUNT(*) "\
        "FROM   songs s, playlistitems pl "\
        "WHERE  pl.playlistid = ? "\
        "AND    s.id = pl.songid;"
    },
    { "Q_COUNT_ALBUM_ITEMS",
        "SELECT count(*) "\
        "FROM   songs "\
        "WHERE album = ?;"
    },
    { "Q_COUNT_ARTIST_ITEMS",
        "SELECT COUNT(*) "\
        "FROM   songs "\
        "WHERE  artist = (SELECT id from artists where artist=?);"
    },
    { "Q_SMARTPL_QUERY",
        "SELECT query "\
        "FROM   playlists p "\
        "WHERE  p.id = ?;"
    },
    { "Q_ALBUM_ARTIST",
        "SELECT album_artist "\
        "FROM   albums "\
        "WHERE  id = ? "\
        "LIMIT  1;"
    }, 
    { "Q_REMOVE_PATH",
        "DELETE FROM paths "\
        "WHERE  id = ?;"
    },
    { "Q_REMOVE_SONG",
        "DELETE FROM songs "\
        "WHERE  path = ?;"
    },
/* ORIGINAL
    { "Q_PLAYCOUNT_INC",
        "UPDATE songs "\
        "SET    play_count  = play_count + 1, "\
        "       time_played = ? "\
        "WHERE  id = ?;"
    },
*/
    { "Q_PLAYCOUNT_INC",
        "WITH new (song, time_requested) as ( VALUES(?, ?) ) \n"\
        "INSERT INTO plays(song, time_requested, time_processed) \n"\
        "SELECT new.song, new.time_requested, \n"\
        "cast((julianday('now') - 2440587.49999999)*86400000000 as integer) \n"\
        "FROM new;"
    },
    { "Q_CHANGE_PATH",
        "WITH new (id, parent, path) AS ( VALUES(?, ?, ?) ) \n"\
        "INSERT OR REPLACE INTO paths (id, parent, path) \n"\
        "SELECT old.id, new.parent, new.path  \n"\
        "FROM new LEFT JOIN paths AS old \n"\
        "ON  old.id = new.id;"\
    },
    { "Q_UPSERT_PATH",
        "WITH new (parent, path) \n"\
        "AS ( VALUES(?, ?) ) \n"\
        "INSERT OR REPLACE INTO paths (id, parent, path) \n"\
        "SELECT old.id, new.parent, new.path \n"\
        "FROM new LEFT JOIN paths AS old \n"\
        "ON new.path = old.path AND new.parent = old.parent; "
    },
    { "Q_UPSERT_ARTIST",
        "WITH new (artist, artist_sort) \n"\
        "AS ( VALUES(?, ?) ) \n"\
        "INSERT OR REPLACE INTO artists (id, artist, artist_sort) \n"\
        "SELECT old.id, new.artist, new.artist_sort \n"\
        "FROM new LEFT JOIN artists AS old \n"\
        "ON new.artist = old.artist;"
    },
    { "Q_UPSERT_PUBLISHER",
        "WITH new (publisher) \n"\
        "AS ( VALUES(?) ) \n"\
        "INSERT OR REPLACE INTO publishers (id, publisher) \n"\
        "SELECT old.id, new.publisher \n"\
        "FROM new LEFT JOIN publishers AS old \n"\
        "ON new.publisher = old.publisher;"
    },
    { "Q_UPSERT_ALBUM",
        "WITH new (album_artist, publisher, year, total_tracks, \n"\
        "          total_discs, album, album_sort) \n"\
        "AS ( VALUES(?, ?, ?, ?, ?, ?, ?) ) \n"\
        "INSERT OR REPLACE INTO albums (id, album_artist, publisher, \n"\
        "       year, total_tracks, total_discs, album, album_sort) \n"\
        "SELECT old.id, new.album_artist, new.publisher, new.year, \n"\
        "       new.total_tracks, new.total_discs, new.album, \n"\
        "       new.album_sort \n"\
        "FROM new LEFT JOIN albums AS old \n"\
        "ON new.album_artist = old.album_artist AND new.album = old.album;"
    },
    { "Q_UPSERT_SONG",
        "WITH new (path, artist, album, genre, track, disc, \n"\
        "          song_length, title) \n"\
        "AS ( VALUES(?, ?, ?, ?, ?, ?, ?, ?) ) \n"\
        "INSERT OR REPLACE INTO songs (id, title, path, artist, album, \n"\
        "genre, track, disc, song_length) \n"\
        "SELECT old.id, new.title, new.path, new.artist, new.album, \n"\
        "       new.genre, new.track, new.disc, new.song_length \n"\
        "FROM new LEFT JOIN songs AS old \n"\
        "ON new.path = old.path; "
    },
    { "Q_UPSERT_GENRE",
        "WITH new (genre) \n"\
        "AS ( VALUES(?) ) \n"\
        "INSERT OR REPLACE INTO genres (id, genre) \n"\
        "SELECT old.id, new.genre \n"\
        "FROM new LEFT JOIN genres AS old \n"\
        "ON new.genre = old.genre;"
    },
    { "Q_GET_RETURN",
        "SELECT id FROM t_temp;"
    },
    { "Q_CLEAR_RETURN",
        "DELETE FROM t_temp;"
    },
    { "Q_GET_SONG_PATH_WITH_PARENT",
        "SELECT id FROM paths WHERE path = ? and parent = ?;"
    },
    { "Q_GET_SONG_FROM_PATH1",
        "SELECT id FROM paths WHERE  path = ?;"
    },
    { "Q_GET_SONG_FROM_PATH2",
        "SELECT id FROM paths \n"\
        "WHERE  path   = ? \n"\
        "AND    parent = \n "\
        "       ( SELECT id FROM paths WHERE path = ? );"
    },
    { "Q_GET_SONG_FROM_PATH3",
        "SELECT id FROM paths \n"\
        "WHERE path   = ? \n"\
        "AND   parent = \n"\
        "      ( SELECT id FROM paths \n"\
        "        WHERE path   = ? \n"\
        "        AND   parent = \n"\
        "              ( SELECT id FROM paths WHERE path = ? ));"
    },  
    { "Q_GET_SONG_FROM_PATH4",
        "SELECT id FROM paths \n"\
        "WHERE path   = ? \n"\
        "AND   paths.parent = \n"\
        "      ( SELECT id FROM paths \n"\
        "        WHERE path   = ? \n"\
        "        AND   parent = \n"\
        "              ( SELECT id FROM paths \n"\
        "                WHERE path = ? \n"\
        "                AND   parent = \n"\
        "                      ( SELECT id FROM paths WHERE path = ? )));"
    },
    { "Q_FIND_ARTIST",
      "SELECT id FROM artists WHERE artist = ?;"
    },
    { "Q_FIND_PUBLISHER",
      "SELECT id FROM publishers WHERE publisher = ?;"
    },
    { "Q_FIND_GENRE",
      "SELECT id FROM genres WHERE genre = ?;"
    },
    { "Q_FIND_ALBUM",
      "SELECT id FROM albums WHERE album = ? \n"\
      "AND    album_artist = ? AND year = ?;"
    },
    { "Q_FIND_SONG",
      ""
    },
    { "Q_PRECOMPILED_MAX",
        NULL
    },
    { "Q_ITEMLIST",
        "SELECT %s "\
        "FROM   songs s, artists ar, albums al, genres g, codecs c "\
        "WHERE  s.artist = ar.id AND s.album = al.id \n"\
        "AND    s.genre = g.id AND s.codec = c.id "
    },
    { "Q_CONTAINERLIST",
        "SELECT %s "\
        "FROM   playlists p "
    },
    { "Q_CONTAINERITEMS",
        "SELECT %s "\
        "FROM   files f, "\
        "       playlistitems pi "
    },
    { "Q_GROUPLIST",
        "SELECT %s "\
        "FROM   groups g "\
        "WHERE  g.type = ? "
    },
    { "Q_COUNT_SMART",
        "SELECT COUNT(*) "\
        "FROM "
    },
    { "Q_MAX",
        NULL
    },
    NULL
};
const char *table_strings[] = {
    /* T_ARTISTS       */ "artists ar",
    /* T_PUBLISHERS    */ "publishers p",
    /* T_ALBUMS        */ "albums al",
    /* T_CODECS        */ "codecs c",
    /* T_PATHS         */ "paths p",
    /* T_PATHTREE      */ "pathtree pt",
    /* T_GENRES        */ "genres g",
    /* T_SONGS         */ "songs s",
    /* T_PLAYLISTS     */ "playlists pl",
    /* T_PLAYLISTITEMS */ "songs s, playlistitems pi",
    /* T_TEMP          */ "t_temp",
    /* T_MAX           */ NULL,
    /* T_GROUPS        */ "groups gr",
    /* T_INOTIFY       */ "inotify i",
    /* T_PAIRINGS      */ "pairings pg",
    /* T_SPEAKERS      */ "speakers s",
    /* T_QUEUE         */ "queue q",
    /* T_GROUP_ITEMS   */ "groups g, files f",
    NULL
};
const sql_t tables[] = {
    { "artists",    
        "CREATE TABLE IF NOT EXISTS artists (\n"\
        "    id            INTEGER PRIMARY KEY NOT NULL,\n"\
        "    persistent_id INTEGER DEFAULT 0,\n"\
        "    artist        VARCHAR(1024) NOT NULL,\n"\
        "    artist_sort   VARCHAR(1024) DEFAULT NULL,\n"\
        "    ts            TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"\
        "); CREATE INDEX IF NOT EXISTS idx_artist ON artists(artist); \n"\
        "   INSERT OR REPLACE INTO artists (id, artist) VALUES \n"\
        "   (0, '(no artist)'); \n"\
        "CREATE TRIGGER IF NOT EXISTS art_upd AFTER UPDATE ON artists \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END; \n"\
        "CREATE TRIGGER IF NOT EXISTS art_ins AFTER INSERT ON artists \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END;"
    },
    { "publishers",
        "CREATE TABLE IF NOT EXISTS publishers (\n"\
        "    id            INTEGER PRIMARY KEY NOT NULL,\n"\
        "    publisher     VARCHAR(1024) NOT NULL,\n"\
        "    ts            TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"\
        "); \n"\
        "CREATE INDEX IF NOT EXISTS idx_pub ON publishers(publisher); \n"\
        "   INSERT OR REPLACE INTO publishers (id, publisher) VALUES \n"\
        "   (0, '(no publisher)'); \n"\
       "CREATE TRIGGER IF NOT EXISTS pub_upd AFTER UPDATE ON publishers\n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END; \n"\
       "CREATE TRIGGER IF NOT EXISTS pub_ins AFTER INSERT ON publishers\n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END;"
    },
    { "albums",     
        "CREATE TABLE IF NOT EXISTS albums (\n"\
        "    id           INTEGER PRIMARY KEY NOT NULL,\n"\
        "    album_artist INTEGER DEFAULT 0 REFERENCES artists (id),\n"\
        "    publisher    INTEGER DEFAULT 0 REFERENCES publishers (id),\n"\
        "    album        VARCHAR(1024) NOT NULL,\n"\
        "    album_sort   VARCHAR(1024) DEFAULT NULL,\n"\
        "    total_tracks INTEGER       DEFAULT 0,\n"\
        "    total_discs  INTEGER       DEFAULT 0,\n"\
        "    compilation  INTEGER       DEFAULT 0,\n"\
        "    year         INTEGER       DEFAULT 0,\n"\
        "    ts           TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"\
        "); CREATE INDEX IF NOT EXISTS idx_album ON albums(album); \n"\
        "   INSERT OR REPLACE INTO albums (id, album) VALUES \n"\
        "   (0, '(no album)'); \n"\
        "CREATE TRIGGER IF NOT EXISTS alb_upd AFTER UPDATE ON albums \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END; \n"\
        "CREATE TRIGGER IF NOT EXISTS alb_ins AFTER INSERT ON albums \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END;"
    },
    { "codecs",     
        "CREATE TABLE IF NOT EXISTS codecs (\n"\
        "    id            INTEGER PRIMARY KEY NOT NULL,\n"\
        "    codectype     VARCHAR(5)  NOT NULL,\n"\
        "    description   VARCHAR(50) NOT NULL\n"\
        "); \n"\
        "   INSERT OR REPLACE into codecs(id, codectype, description) \n"\
        "   VALUES (0, '----', '(no codec information)');"  
    }, 
    { "paths", 
        "CREATE TABLE IF NOT EXISTS paths (\n"\
        "    id            INTEGER PRIMARY KEY NOT NULL,\n"\
        "    parent        INTEGER NOT NULL,\n"\
        "    path          VARCHAR(255) NOT NULL,\n"\
        "    ts            TIMESTAMP DEFAULT CURRENT_TIMESTAMP, \n"\
        "    UNIQUE(path, parent) \n"\
        "); \n"\
        "CREATE INDEX IF NOT EXISTS idx_paths_parent ON paths(parent); \n"\
        "CREATE INDEX IF NOT EXISTS idx_paths_path ON paths(path); \n"\
        "CREATE TRIGGER IF NOT EXISTS path_upd AFTER UPDATE ON paths \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END; \n"\
        "CREATE TRIGGER IF NOT EXISTS path_ins AFTER INSERT ON paths \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END; \n"
    },
    { "pathtree",
        "CREATE VIRTUAL TABLE IF NOT EXISTS pathtree "\
        "USING transitive_closure (\n"\
        "    tablename=paths,\n"\
        "    idcolumn=id,\n"\
        "    parentcolumn=parent\n"\
        ");"
    },
    { "genres",     
        "CREATE TABLE IF NOT EXISTS genres (\n"\
        "    id            INTEGER PRIMARY KEY NOT NULL,\n"\
        "    genre         VARCHAR(255) NOT NULL,\n"\
        "    ts            TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"\
        "); CREATE INDEX IF NOT EXISTS idx_genre ON genres(genre); \n"\
        "   INSERT OR REPLACE INTO genres(id, genre) VALUES \n"\
        "   (0, '(no genre)'); \n"\
        "CREATE TRIGGER IF NOT EXISTS gen_upd AFTER UPDATE ON genres \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END; \n"\
        "CREATE TRIGGER IF NOT EXISTS gen_ins AFTER INSERT ON genres \n"\
        "      BEGIN INSERT INTO t_temp SELECT NEW.id; END;"
    }, 
    { "songs",      
        "CREATE TABLE IF NOT EXISTS songs (\n" \
        "    id            INTEGER PRIMARY KEY NOT NULL,\n" \
        "    path          INTEGER DEFAULT 0 REFERENCES paths  (id),\n" \
        "    codec         INTEGER DEFAULT 0 REFERENCES codecs (id),\n"\
        "    artist        INTEGER DEFAULT 0 REFERENCES artists(id),\n"\
        "    album         INTEGER DEFAULT 0 REFERENCES albums (id),\n"\
        "    genre         INTEGER DEFAULT 0 REFERENCES genres (id),\n"\
        "    comment       VARCHAR(4096) DEFAULT NULL,\n"\
        "    composer      VARCHAR(1024) DEFAULT NULL,\n"\
        "    orchestra     VARCHAR(1024) DEFAULT NULL,\n"\
        "    conductor     VARCHAR(1024) DEFAULT NULL,\n"\
        "    grouping      VARCHAR(1024) DEFAULT NULL,\n"\
        "    url           VARCHAR(1024) DEFAULT NULL,\n"\
        "    bitrate       INTEGER       DEFAULT 0,\n"\
        "    samplerate    INTEGER       DEFAULT 0,\n"\
        "    song_length   INTEGER       DEFAULT 0,\n"\
        "    file_size     INTEGER       DEFAULT 0,\n"\
        "    track         INTEGER       DEFAULT 0,\n"\
        "    disc          INTEGER       DEFAULT 0,\n"\
        "    bpm           INTEGER       DEFAULT 0,\n"\
        "    rating        INTEGER       DEFAULT 0,\n"\
        "    time_added    INTEGER       DEFAULT 0,\n"\
        "    time_played   INTEGER       DEFAULT 0,\n"\
        "    has_video     INTEGER       DEFAULT 0,\n"\
        "    ts            TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"\
        "    title         VARCHAR(1024) DEFAULT NULL\n"\
        "); \n"\
        "CREATE INDEX IF NOT EXISTS idx_song_artist ON songs(artist);\n"\
        "CREATE INDEX IF NOT EXISTS idx_song_album  ON songs(album); \n"\
        "CREATE INDEX IF NOT EXISTS idx_song_genre  ON songs(genre); \n"\
        "CREATE INDEX IF NOT EXISTS idx_song_path   ON songs(path);  \n"\
        "CREATE INDEX IF NOT EXISTS idx_song_title  ON songs(title); \n"
    },
    { "playlists",  
        "CREATE TABLE IF NOT EXISTS playlists (\n"\
        "    id            INTEGER PRIMARY KEY NOT NULL,\n"\
        "    path          INTEGER NOT NULL REFERENCES paths (id),\n"\
        "    type          INTEGER DEFAULT 0,\n"\
        "    items         INTEGER DEFAULT 0,\n"\
        "    fname         VARCHAR(255)  NOT NULL,\n"\
        "    title         VARCHAR(1024) NOT NULL,\n"\
        "    query         VARCHAR(1024) DEFAULT NULL,\n"\
        "    ts            TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"\
        ");"
    },
    { "playlistitems",
        "CREATE TABLE IF NOT EXISTS playlistitems (\n"\
        "    id            INTEGER PRIMARY KEY NOT NULL,\n"\
        "    playlistid    INTEGER NOT NULL REFERENCES playlists (id),\n"\
        "    songid        INTEGER NOT NULL REFERENCES songs     (id)\n"\
        "); \n"\
        "CREATE INDEX IF NOT EXISTS idx_pli ON playlistitems(playlistid, songid);"
    },
    { "t_temp",
        "CREATE TABLE IF NOT EXISTS t_temp(id INTEGER);"
    },
    { "plays",
        "CREATE TABLE IF NOT EXISTS plays (\n"\
        "    id             INTEGER PRIMARY KEY NOT NULL, \n"\
        "    song           INTEGER NOT NULL, \n"\
        "    time_requested INTEGER, \n"\
        "    time_processed INTEGER, \n"\
        "    ts             TIMESTAMP DEFAULT CURRENT_TIMESTAMP \n"\
        ");"
    },
        NULL
};

int db_open_database(app *aux, int flags) {
    int ret;
    ret = sqlite3_open_v2((aux->config)->dbfile, &aux->db, flags, NULL);
    if (ret) {
        syslog(LOG_ERR, "thread couldn't open database\n");
        exit(1);
        return ret;
    } else
        syslog(LOG_INFO, "thread opened database \n");
    sqlite3_enable_load_extension(aux->db, 1);
    char *error = NULL;
    ret = sqlite3_load_extension(aux->db, 
            (aux->config)->extfile, NULL, &error);
    if (ret != SQLITE_OK) {
        syslog(LOG_ERR, "failed to load sqlite extension: %s.\n", error);
        exit(1);
        return ret;
    }

    ret = sqlite3_exec(aux->db, "PRAGMA foreign_keys = ON;", 0, 0, 0);

    return ret;
}
int db_close_database(app *aux) {
    int ret;
    for (q_type q = 0; q < Q_PRECOMPILED_MAX; q++) 
        sqlite3_finalize(aux->stmts[q]);
    ret = sqlite3_close_v2(aux->db);
    if (ret != SQLITE_OK) {
        syslog(LOG_ERR, "thread failed to close database.\n");
    }
    return ret;
}

void precompile_statements(void *arg) {
    app *aux = (app *)arg;
    aux->stmts = calloc(Q_PRECOMPILED_MAX, sizeof(sqlite3_stmt *));
    int ret;
    q_type q;
    for (q = 0; q < Q_PRECOMPILED_MAX; q++) {
        while((ret  = sqlite3_prepare_v2(aux->db, 
                      queries[q].query, -1, 
                      &(aux->stmts[q]), NULL)) == SQLITE_BUSY);
        if (ret != SQLITE_OK) 
            syslog(LOG_INFO, 
                   "precompiling query %s failed: %d\n", 
                   queries[q].name, ret);
    }   
}

int db_find_path_with_parent(app *aux, const char *path, const int parent, SCRATCH *s) {
    sqlite3_stmt *stmt = aux->stmts[Q_GET_SONG_PATH_WITH_PARENT];
    int ret;
    ret = sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_bind_int(stmt, 2, parent);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        sqlite3_reset(stmt);
        return 0;
    }
    int result = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

int db_find_path(app *aux, const char *path,  SCRATCH *s) {

    int    count  = split_path(path, s);
    char **p      = scratch_head(s);
    int    i      = count ? count - 1 : 0;
    int    result = 0;
    int    ret;
// try searching on just the filename first
    sqlite3_stmt *stmt = aux->stmts[Q_GET_SONG_FROM_PATH1];
    sqlite3_bind_text(stmt, 1, p[i], -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    result = sqlite3_column_int(stmt, 0);
    ret = sqlite3_step(stmt);
    if (ret == SQLITE_ROW && i > 1) { 
// we got duplicates;  have to try more specific query
        sqlite3_reset(stmt);
        stmt = aux->stmts[Q_GET_SONG_FROM_PATH2];
        sqlite3_bind_text(stmt, 1, p[i], -1, NULL);
        sqlite3_bind_text(stmt, 2, p[i - 1], -1 , NULL);
        ret = sqlite3_step(stmt);
        result = sqlite3_column_int(stmt, 0);
        syslog(LOG_INFO, "%d\n", result);
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_ROW && i > 2) { 
// we got dupicates again; try more specific query
            sqlite3_reset(stmt);
            stmt = aux->stmts[Q_GET_SONG_FROM_PATH3];
            sqlite3_bind_text(stmt, 1, p[i], -1, NULL);
            sqlite3_bind_text(stmt, 2, p[i - 1], -1, NULL);
            sqlite3_bind_text(stmt, 3, p[i - 2], -1, NULL);
            ret = sqlite3_step(stmt);
            result = sqlite3_column_int(stmt, 0);
            syslog(LOG_INFO, "%d\n", result);
            ret = sqlite3_step(stmt);
            if (ret == SQLITE_ROW && i > 3) { 
// again dupicates, again try more specific
                sqlite3_reset(stmt);
                stmt = aux->stmts[Q_GET_SONG_FROM_PATH4];
                sqlite3_bind_text(stmt, 1, p[i], -1, NULL);
                sqlite3_bind_text(stmt, 2, p[i - 1], -1, NULL);
                sqlite3_bind_text(stmt, 3, p[i - 2], -1, NULL);
                sqlite3_bind_text(stmt, 3, p[i - 3], -1, NULL);
                ret = sqlite3_step(stmt);
                result = sqlite3_column_int(stmt, 0);
                syslog(LOG_INFO, "%d\n", result);
                ret = sqlite3_step(stmt);
                if (ret == SQLITE_ROW) { 
// this time just fail
                    sqlite3_reset(stmt);
                    return -1;
                } else sqlite3_reset(stmt);
            } else sqlite3_reset(stmt);
        } else sqlite3_reset(stmt);
    } else sqlite3_reset(stmt);
    return result;
}

int db_find_artist(app *aux, const char *artist) {
    sqlite3_stmt *stmt = aux->stmts[Q_FIND_ARTIST];
    int ret;
    ret = sqlite3_bind_text(stmt, 1, artist, -1, SQLITE_STATIC);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        sqlite3_reset(stmt);
        return 0;
    }
    int result = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

int db_find_publisher(app *aux, const char *publisher) {
    sqlite3_stmt *stmt = aux->stmts[Q_FIND_PUBLISHER];
    int ret;
    ret = sqlite3_bind_text(stmt, 1, publisher, -1, SQLITE_STATIC);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        sqlite3_reset(stmt);
        return 0;
    }
    int result = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

int db_find_genre(app *aux, const char *genre) {
    sqlite3_stmt *stmt = aux->stmts[Q_FIND_GENRE];
    int ret;
    ret = sqlite3_bind_text(stmt, 1, genre, -1, SQLITE_STATIC);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        sqlite3_reset(stmt);
        return 0;
    }
    int result = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

int db_find_album(app *aux, const char *album, int artist, int year) {
    sqlite3_stmt *stmt = aux->stmts[Q_FIND_ALBUM];
    int ret;
    ret = sqlite3_bind_text(stmt, 1, album, -1, SQLITE_STATIC);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_bind_int(stmt, 2, artist);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_bind_int(stmt, 3, year);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return 0;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        sqlite3_reset(stmt);
        return 0;
    }
    int result = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

const char *get_smart_playlist_query(app *aux, int playlist) {
    sqlite3_stmt *stmt = aux->stmts[Q_SMARTPL_QUERY];
    int ret;
    ret = sqlite3_bind_int(stmt, 1, playlist);
    if (ret != SQLITE_OK) {
        sqlite3_reset(stmt);
        return NULL;
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        sqlite3_reset(stmt);
        return NULL;
    }
    const char *result = sqlite3_column_text(stmt, 0);
    char *str = malloc(strlen(result) + 1);
    strcpy(str, result);
    sqlite3_reset(stmt); // leave the statement ready to go again
    return str;
}

const char *get_albumgroup_artist(app *aux, int id) {
    sqlite3_stmt *stmt = aux->stmts[Q_ALBUM_ARTIST];
    int ret;
    const char *result;
    sqlite3_bind_int(stmt, 1, id);
    ret = sqlite3_step(stmt);
    result = sqlite3_column_text(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

size_t count_all_files(app *aux) {
    int ret;
    size_t result;
    sqlite3_stmt *stmt = aux->stmts[Q_COUNT_ALL_SONGS];
    ret = sqlite3_step(stmt);
    result = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

size_t count_smart_items(app *aux, t_type table, vector *clauses, int *bindvar) {
    sqlite3_stmt *stmt;
    char *clause;
    int ret;
    size_t qty = 0;
    syslog(LOG_INFO, "finding smart playlist items...\n");
    size_t len = strlen(queries[Q_COUNT_SMART].query) + 
                 strlen(table_strings[table]) + 2;
    if (clauses) {
        for (int i = 0; i < clauses->used; i++) {
            clause = clauses->data[i];
            len += strlen(clause) + 1;
        }
    }
    char *sqlstr = malloc(len);
    sqlstr[0] = '\0';
    strcat(sqlstr, queries[Q_COUNT_SMART].query);
    strcat(sqlstr, table_strings[table]);
    if (clauses) {
        for (int i = 0; i < clauses->used; i++) {
            clause = clauses->data[i];
            strcat(sqlstr, " ");
            strcat(sqlstr, clause);
        }
    }
    strcat(sqlstr, ";");
    // now the query string is built
    syslog(LOG_INFO, "query: %s\n", sqlstr);
    ret = sqlite3_prepare_v2(aux->db, sqlstr, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        free(sqlstr);
        sqlite3_finalize(stmt);
        return 0;
    }
    if (bindvar) {
        sqlite3_bind_int(stmt, 1, *bindvar);
        if (ret != SQLITE_OK) {
            free(sqlstr);
            sqlite3_finalize(stmt);
            return 0;
        }
    }
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        free(sqlstr);
        sqlite3_finalize(stmt);
        return 0;
    }
    qty = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    free(sqlstr);
    return qty;
    
}

size_t count_precompiled_items(app *aux, q_type q, int *bindvar) {
    int ret;
    size_t result;
    sqlite3_stmt *stmt;
    if (q < Q_PRECOMPILED_MAX) stmt = aux->stmts[q];
    else return 0;
    if (bindvar)
        sqlite3_bind_int(stmt, 1, *bindvar);
    ret = sqlite3_step(stmt);
    result = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);
    return result;
}

void sql_delete_file(sqlite3 *db, const char *path) {
    sqlite3_stmt *stmt;
    int ret;
    sqlite3_prepare_v2(db, queries[Q_REMOVE_PATH].query, -1, &stmt, NULL); 
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        syslog(LOG_ERR, "failed to delete file '%s' error %d.\n", path, ret);
    }
    sqlite3_finalize(stmt);
}

void sql_build_query(query_t *q, vector *columns, 
                     vector *clauses, char **output) {
    if (!q) return;
    char *buf, *str;
    int len;
    switch(q->type) {
        case Q_ITEMLIST:        
        case Q_GROUPLIST:
        case Q_CONTAINERITEMS:
        case Q_CONTAINERLIST:  {    
 // combine the columns with comma separation
            meta_tag_t *tag;
            if (columns && columns->used > 0) {
                len = 2;
                for (int i = 0; i < columns->used; i++) {
                    tag = columns->data[i];
                    if (tag->db_column)
                        len += strlen(tag->db_column) + 1;
                }
                buf = malloc(len);
                int opos = 0, ipos = 0, i = 0;
                tag = columns->data[i];
                char *src = tag->db_column;
                while (i < columns->used && opos < len) {
                    if (!src) {
                        tag = columns->data[++i];
                        src = tag->db_column;
                        continue;
                    }
                    if (src[ipos] == '\0') {
                        i++;
                        if (i < columns->used) {
                            tag = columns->data[i];
                            buf[opos++] = ',';
                        } else buf[opos++] = '\0';
                        ipos = 0;
                        src = tag->db_column;
                    } else
                        buf[opos++] = src[ipos++];
                }
            } else { 
 // if no columns specified, return all columns
                buf = "*";
                len = 1;
            }
            len += strlen(queries[q->type].query);
            // make space for clauses
            if (clauses) {
                for (int i = 0; i < clauses->used; i++) {
                    str = clauses->data[i];
                    len += strlen(str) + 1;
                }
            }
            char *out = malloc(len);
            sprintf(out, queries[q->type].query, buf); 
            if (clauses) {
                for (int i = 0; i < clauses->used; i++) {
                    str = clauses->data[i];
                    strcat(out, " ");
                    strcat(out, str);
                }
            }
            strcat(out, ";");
            *output = out;
            free(buf);
            }
            break;
        case Q_COUNT_SMART: 
            len = strlen(queries[q->type].query) + 2;
            if (clauses) {
                for (int i = 0; i < clauses->used; i++) {
                    str = clauses->data[i];
                    len += strlen(str) + 1;
                }
            }
            buf = malloc(len);
            buf[0] = '\0';
            strcat(buf, queries[q->type].query);
            if (clauses) {
                for (int i = 0; i < clauses->used; i++) {
                    str = clauses->data[i];
                    strcat(buf, " ");
                    strcat(buf, str);
                }
            }
            strcat(buf, ";");
            *output = buf;
            break;
        default:
            break;
    }
    syslog(LOG_INFO, "query: '%s'\n", *output);
}

int sql_put_results(
        struct evbuffer **dest, 
        struct evbuffer **items,  
        app *aux, 
        vector *cols, 
        int start_col, 
        const char *sqlstr, 
        int *bindvar, 
        int(*item_cb)(app *aux, struct evbuffer *item, meta_info_t *info)
        ) {
    sqlite3 *db = aux->db;
    sqlite3_stmt *stmt; 
    int ret, val, i = 0, nitems = 0, keep_item;
    unsigned long long lval;
    sqlite3_prepare_v2(db, sqlstr, -1, &stmt, NULL);
    if (bindvar) {
        ret = sqlite3_bind_int(stmt, 1, *bindvar);
        if (ret != SQLITE_OK) {
            sqlite3_finalize(stmt);
            return -1;
        }
    }
    const char *str;
    meta_info_t info = {0};
    char *base = (char *)(&info);
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        items[i] = evbuffer_new();
        evbuffer_expand(items[i], 512);
// loop through columns
        for (int col = 0; col < cols->used; col++) { 
            meta_tag_t *tag = cols->data[col];
            if (!strcmp(tag->tag, "mcti")) { 
// dmap.containeritemid - replace it with place in results
                dmap_add_int(items[i], "mcti", i + 1);
            } else switch(tag->type) {
                case T_LONG:
                        lval = sqlite3_column_int64(stmt, col);
                        if (col >= start_col) 
                            dmap_add_long(items[i], tag->tag, lval);
                        if (item_cb) {
                            long *loc = (long *)(base + tag->ofs);
                            *loc = lval;
                        } break;
                case T_DATE:
                case T_INT:
                        val = sqlite3_column_int(stmt, col);
                        if (col >= start_col) 
                            dmap_add_int(items[i], tag->tag, val);
                        if (item_cb) {
                            int *loc = (int *)(base + tag->ofs);
                            *loc = val;
                        } break;
                case T_CHAR:
                        val = sqlite3_column_int(stmt, col);
                        if (col >= start_col) 
                            dmap_add_char(items[i], tag->tag, (char)val);
                        if (item_cb) {
                            char *loc = (char *)(base + tag->ofs);
                            *loc = (char)val;
                        } break;
                case T_SHORT: 
                        val = sqlite3_column_int(stmt, col);
                        if (col >= start_col) 
                            dmap_add_short(items[i], tag->tag, (short)val);
                        if (item_cb) {
                            short *loc = (short *)(base + tag->ofs);
                            *loc = (short)val;
                        } break;
                case T_STRING:
                        str = sqlite3_column_text(stmt, col);
                        if (col >= start_col) 
                            dmap_add_string(items[i], tag->tag, STR(str));
                        if (item_cb) {
                            char **loc = (char **)(base + tag->ofs);
                            *loc = (char *)str;
                        } break;
                default:  break;
            }
        }
        if (item_cb) 
            keep_item = item_cb(aux, items[i], &info);
        else keep_item = 1;
        if (keep_item) {
            dmap_add_list(*dest, "mlit", evbuffer_get_length(items[i]));
            evbuffer_add_buffer(*dest, items[i]);
            nitems++;
        }
        i++;
    }
    sqlite3_finalize(stmt);
    return nitems;
}
