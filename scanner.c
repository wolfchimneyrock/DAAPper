#include <stdint.h>
#include <syslog.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libfswatch/c/libfswatch.h>
#include <unistd.h>
#include <string.h>
#include <fts.h>
#include <sqlite3.h>
#include "util.h"
#include "config.h"
#include "id3cb.h"
#include "ringbuffer.h"
#include "vector.h"
#include "scratch.h"
#include "scanner.h"
#include "watcher.h"
#include "writer.h"
#include "system.h"
#include "cache.h"

#define CACHE_INITIAL_CAP    4096
#define META_SCRATCH_SIZE    4096
#define PATH_SCRATCH_SIZE    4096 
#define BUFFER_CAPACITY      256 

static CACHE *file_cache;

volatile sig_atomic_t   scanner_active      = 0;
static pthread_mutex_t  scanner_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   scanner_ready       = PTHREAD_COND_INITIALIZER; 
static RINGBUFFER      *scanner_buffer;

void wait_for_scanner() {
    pthread_mutex_lock(&scanner_ready_mutex);
    while (scanner_active == 0)
        pthread_cond_wait(&scanner_ready, &scanner_ready_mutex);
    pthread_mutex_unlock(&scanner_ready_mutex);
}

int scanner_submit_request(char *path) {
    return rb_pushback(scanner_buffer, path);
}

static int process_text_tags(const id3_text *text, void *aux) {

    SCRATCH     *s    = (SCRATCH *)aux;
    meta_info_t *meta = (meta_info_t *)scratch_head(s);
    size_t       len  = strlen(text->data) + 1;

    switch (text->type) {

        case ID3_TEXT_TITLE:
            meta->title = scratch_get(s, len);
            strncpy(meta->title, text->data, len);
            break;

        case ID3_TEXT_SORT_TITLE:
            meta->title_sort = scratch_get(s, len);
            strncpy(meta->title_sort, text->data, len);
            break;

        case ID3_TEXT_ARTIST:
            meta->artist = scratch_get(s, len);
            strncpy(meta->artist, text->data, len);
            break;

        case ID3_TEXT_SORT_ARTIST:
            meta->artist_sort = scratch_get(s, len);
            strncpy(meta->artist_sort, text->data, len);
            break;

        case ID3_TEXT_ALBUM:
            meta->album = scratch_get(s, len);
            strncpy(meta->album, text->data, len);
            break;

        case ID3_TEXT_SORT_ALBUM:
            meta->album_sort = scratch_get(s, len);
            strncpy(meta->album_sort, text->data, len);
            break;

        case ID3_TEXT_TRACK: 
        // split into track / total tracks, convert to int
        // this modifies the original buffer
            {
                char *pos = strchr(text->data, '/');
                if (pos) {
                    meta->total_tracks = atoi(pos + 1);
                    pos[0] = '\0'; // convert to null for track
                } 
                meta->track = atoi(text->data);
            }
            break;

        case ID3_TEXT_DISC:
        // split disc / total discs, convert to int
        // this modifies the original buffer
            {
                char *pos = strchr(text->data, '/');
                if (pos) {
                    meta->total_discs = atoi(pos + 1);
                    pos[0] = '\0'; // convert to null for track
                } 
                meta->disc = atoi(text->data);
            }
            break;

        case ID3_TEXT_MEDIA_TYPE:
            meta->codectype = scratch_get(s, len);
            strncpy(meta->codectype, text->data, len);
            break;
        
        case ID3_TEXT_ALBUM_ARTIST:
            meta->album_artist = scratch_get(s, len);
            strncpy(meta->album_artist, text->data, len);
            break;

        case ID3_TEXT_SORT_ALBUM_ARTIST:
            meta->album_artist_sort = scratch_get(s, len);
            strncpy(meta->album_artist_sort, text->data, len);
            break;

        case ID3_TEXT_GENRE:
            meta->genre = scratch_get(s, len);
            strncpy(meta->genre, text->data, len);
            break;

        case ID3_TEXT_YEAR:
            meta->year = atoi(text->data);
            break;

        case ID3_TEXT_ORIG_YEAR:
            meta->year_orig = atoi(text->data);
            break;

        case ID3_TEXT_SONG_LENGTH:
            meta->song_length = atoi(text->data);
            break;
        
        case ID3_TEXT_PUBLISHER:
            meta->publisher = scratch_get(s, len);
            strncpy(meta->publisher, text->data, len);
            break;

        default:
            break;
    }
}

static int add_file(app *aux, const char *fname, const char *path, ID3CB *id3, SCRATCH *meta_scratch, int parent) {
    syslog(LOG_INFO, "    add_file() %s/%s", path, fname);
    char *ext = strchr(fname, '.');
    if (ext && !strcmp(ext + 1, "mp3")) {
        int pathid = db_upsert_path(aux, fname, parent);

        meta_info_t *meta = scratch_head(meta_scratch);

        id3_parse_file(id3, path, meta_scratch);
        
        int artistid, albumartistid, albumid, genreid, publisherid;
        
        if (meta->artist && (meta->artist)[0] != '\0')
            artistid = db_upsert_artist(aux, meta->artist, 
                                        meta->artist_sort);
        else artistid = 0;

        // does album_artist exist?
        // are artist and album_artist different?
        if (meta->album_artist) {
            if (strcmp(meta->artist, meta->album_artist)) 
                albumartistid = db_upsert_artist(aux, meta->album_artist, 
                        meta->album_artist_sort); 
            else albumartistid = artistid;
        } else 
            meta->album_artist = meta->artist;

        if (meta->publisher && (meta->publisher)[0] != '\0')    
            publisherid =db_upsert_publisher(aux, meta->publisher);
        else publisherid = 0;
        
        if (meta->album && (meta->album)[0] != '\0') {
            if (!meta->album_sort) meta->album_sort = meta->album;
            albumid  = db_upsert_album (aux, meta->album,
                                        meta->album_sort, 
                    albumartistid, publisherid, 
                    meta->year, meta->total_tracks, meta->total_discs);
        } 
        else albumid = 0;
        
        if (meta->genre && (meta->genre)[0] != '\0')    
            genreid  = db_upsert_genre (aux, meta->genre);
        else genreid = 0;

        db_upsert_song(aux, meta->title, pathid, artistid, albumid, 
                genreid, meta->track, meta->disc, meta->song_length );
        
        return 1;

    }
    return 0;
}


static int execute_scan(app *aux, char *path) {

    // both FTS and ID3CB should be threadsafe, _if_ we create thread specific
    // instances.  also, we will open our own connection to the database, so that
    // if multiple scan directory threads are running


    char *paths[2] = { 0 };
    vector parents;
    vector_new(&parents, 10); // unlikely to have a directory tree > 10 levels
    int searching_root = 0;
    if (path) {
        paths[0] = path;
        SCRATCH *path_scratch = scratch_new(PATH_SCRATCH_SIZE);
        char *parent_path = strdup(path);
        // after executing this, we will have the parent
        char *fname = split_filename(parent_path);
        int root_parent;
        if (strcmp(parent_path, aux->config->root)) 
            root_parent = db_find_path(aux, parent_path, path_scratch);
        else root_parent = 1;
        syslog(LOG_INFO, "parent: [%d] %s\n", root_parent, parent_path);
        vector_pushback(&parents, INT_TO_PTR(root_parent));
        scratch_free(path_scratch, SCRATCH_FREE);
    } else {
        searching_root = 1;
        paths[0] = aux->config->root;
    }

    syslog(LOG_INFO, "scanning '%s'\n", paths[0]);
    ID3CB *id3 = id3_new_parser(ID3_AUTOCONVERT_TO_UTF8);

    id3_set_all_texts_handler(id3, process_text_tags);
    //id3_set_autoconvert_to_utf8(id3);
    
    SCRATCH *meta_scratch = scratch_new(META_SCRATCH_SIZE);
    size_t file_count = 0;
    size_t dir_count  = 0;
    FTS *tree = fts_open(paths, FTS_NOCHDIR | FTS_NOSTAT, 0);
    FTSENT *node;
       
    while (node = fts_read(tree)) {
        if (node->fts_info & FTS_F) { // visiting a file
            syslog(LOG_INFO, "found file '%s'\n", node->fts_name);
                //fts_set(tree, node, FTS_SKIP);
            meta_info_t *meta = scratch_get(meta_scratch, sizeof(meta_info_t));
            int parent =  PTR_TO_INT(vector_peekback(&parents));
            file_count += add_file(aux, node->fts_name, node->fts_path, id3, meta_scratch, parent);
            // clear the meta struct for next use
            scratch_reset(meta_scratch);

        }
        else if (node->fts_info & FTS_D) { // visiting a dir pre-order
            // add watcher for this folder
            char *name;
            char *path = strdup(node->fts_path);
            if (node->fts_level == 0 && searching_root)
                name = node->fts_path;
            else name = node->fts_name;
            int parent = PTR_TO_INT(vector_peekback(&parents));
            int this = db_upsert_path(aux, strdup(name), parent);
            dir_count++;
            vector_pushback(&parents, INT_TO_PTR(this));
        }
        else if (node->fts_info & FTS_DP) { // visiting a dir post-order
            // pop our dir entry off tree when done
            vector_popback(&parents);
        }
    }
    scratch_free(meta_scratch, SCRATCH_FREE);
    //scratch_free(path_scratch, SCRATCH_FREE);
    id3_dispose_parser(id3);
    vector_free(&parents);
    syslog(LOG_INFO, "scanned %lu files in %lu directories.\n", file_count, dir_count);
}

static void scanner_cleanup(void *arg) {
    app *state = (app *)arg;
    int ret;

    rb_free(scanner_buffer);
    db_close_database(state);
    syslog(LOG_INFO, "scanner thread terminated.\n");
}

void  *scanner_thread(void *arg) {
    syslog(LOG_INFO, "scanner thread starting...\n");
    scanner_pid = pthread_self();
    config_t *conf = (config_t *)arg;
    app state;
    state.config = conf;

    int cleanup_pop_val;
    pthread_cleanup_push(scanner_cleanup, &state);

    scanner_buffer = rb_init(BUFFER_CAPACITY);

    int ret;
    
    wait_for_writer();

    db_open_database(&state, SQLITE_OPEN_READONLY);
    precompile_statements(&state);
    syslog(LOG_INFO, "scanner opened db.\n");   

    pthread_mutex_lock(&scanner_ready_mutex);
    scanner_active = 1;
    pthread_mutex_unlock(&scanner_ready_mutex);
    pthread_cond_broadcast(&scanner_ready);
    
    wait_for_watcher();
    
    syslog(LOG_INFO, "scanner thread active.\n");   

    if (conf->fullscan || count_all_files(&state) == 0) {
        syslog(LOG_INFO, "initiating full scan...\n");
        execute_scan(&state, NULL);
    }
    

    while (scanner_active) {
        pthread_testcancel();
        char *path = rb_popfront(scanner_buffer);
        syslog(LOG_INFO, "scanner got '%s'\n", path);
        execute_scan(&state, path);
    }

    pthread_cleanup_pop(cleanup_pop_val);

    return NULL;
}
