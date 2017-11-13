#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <fts.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "config.h"
#include "writer.h"
#include "sql.h"
#include "ringbuffer.h"
#include "scratch.h"
#include "system.h"
#include "util.h"

volatile sig_atomic_t writer_active = 0;
static int db_return_int;
static char *db_return_str;
static RINGBUFFER *writer_buffer;
static sem_t db_return_int_sem, 
             db_return_str_sem;
static pthread_cond_t  writer_ready       = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t writer_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t db_status_mutex    = PTHREAD_MUTEX_INITIALIZER;
volatile time_t db_updated;
volatile struct timeval 
    write_started,
    write_finished;

#define TIMESTAMP(t) \
    (1000000 * t.tv_sec + t.tv_usec)


time_t db_last_update_time() {
    time_t result;
    pthread_mutex_lock(&db_status_mutex);
    result = db_updated;
    pthread_mutex_unlock(&db_status_mutex);
    return result;
}


void db_init_status() {
    time_t now;
    time(&now);
    pthread_mutex_lock(&db_status_mutex);
    db_updated = now;
    pthread_mutex_unlock(&db_status_mutex);
}

/**
 * @brief a db-read-only thread executes this to write to the database
 *        this has been wrapped by the following specific action commands
 *        db_inc_playcount()
 *        db_remove_file()
 *        db_upsert_path()
 *        db_upsert_artist()
 *        db_upsert_publisher()
 *        db_upsert_genre()
 *        db_upsert_album()
 *        db_upsert_song()
 */
static int submit_write_query(query_t **q) {
    if (q == NULL || *q == NULL)
        return -1;
    struct timeval now;
    gettimeofday(&now, NULL);
    (*q)->created = TIMESTAMP(now);
    (*q)->place = rb_size(writer_buffer);
    return rb_pushback(writer_buffer, q);
}

void db_inc_playcount(const int song) {
// we don't have to be (and can't always be) this precise with scratch size
    SCRATCH *s = scratch_new( 2*sizeof(query_t *) + 
                              sizeof(query_t) + 
                              1*sizeof(int) +
                              1*sizeof(int64_t)
                             );
    query_t **q = scratch_get(s, 2*sizeof(query_t *));
    struct timeval now;
    gettimeofday(&now, NULL);
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_PLAYCOUNT_INC;
    q[0]->n_int = 1;
    q[0]->n_int64 = 1;
    q[0]->intvals = scratch_get(s, 1*sizeof(int));
    q[0]->int64vals = scratch_get(s, 1*sizeof(int64_t));
    q[0]->intvals[0] = (int)song;
    q[0]->int64vals[0] = TIMESTAMP(now);
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
}

void db_remove_file(int pathid) {
    SCRATCH *s = scratch_new( 3*sizeof(query_t *) + 
                              2*sizeof(query_t) + 
                              2*sizeof(int));
    query_t **q = scratch_get(s, 3*sizeof(query_t *));
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_REMOVE_SONG;
    q[0]->n_str = 0;
    q[0]->n_int = 1;
    q[0]->intvals = scratch_get(s, sizeof(int));
    q[0]->intvals[0] = pathid;
    q[1] = scratch_get(s, sizeof(query_t));
    q[1]->type = Q_REMOVE_PATH;
    q[1]->n_str = 0;
    q[1]->n_int = 1;
    q[1]->intvals = scratch_get(s, sizeof(int));
    q[1]->intvals[0] = pathid;
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
}

int db_upsert_path(app *aux, const char *path, const int parent) {
    size_t len = strlen(path) + 1;
    SCRATCH *s = scratch_new( 4*sizeof(query_t *) + 
                              3*sizeof(query_t) + 
                              1*sizeof(char *) + 
                              1*sizeof(int) + 
                              len);
    int pathid = db_find_path_with_parent(aux, path, parent, s);
    scratch_reset(s);
    if (!pathid) {
        query_t **q = scratch_get(s, 4*sizeof(query_t *));
        q[0] = scratch_get(s, sizeof(query_t));
        q[0]->type = Q_CLEAR_RETURN;
        q[1] = scratch_get(s, sizeof(query_t));
        q[1]->type = Q_UPSERT_PATH;
        q[1]->n_str = 1;
        q[1]->n_int = 1;
        q[1]->strvals = scratch_get(s, sizeof(char *));
        q[1]->intvals = scratch_get(s, sizeof(int));
        q[1]->strvals[0] = scratch_get(s, len);
        strncpy(q[1]->strvals[0], path, len);
        q[1]->intvals[0] = (int)parent;
        q[2] = scratch_get(s, sizeof(query_t));
        q[2]->type = Q_GET_RETURN;
        q[2]->returns = R_INT;
        scratch_free(s, SCRATCH_KEEP);
        submit_write_query(q);
        sem_wait(&db_return_int_sem);
        int val = db_return_int;
        return val;
    } else return pathid;
}

int db_change_path(app *aux, const int pathid, const char *path, const int parent) {
    size_t len = strlen(path) + 1;
    SCRATCH *s = scratch_new( 2*sizeof(query_t *) + 
                              1*sizeof(query_t) + 
                              1*sizeof(char *) + 
                              2*sizeof(int) + 
                              len);
    scratch_reset(s);
    query_t **q = scratch_get(s, 2*sizeof(query_t *));
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_CHANGE_PATH;
    q[0]->n_str = 1;
    q[0]->n_int = 2;
    q[0]->strvals = scratch_get(s, sizeof(char *));
    q[0]->intvals = scratch_get(s, 2*sizeof(int));
    q[0]->strvals[0] = scratch_get(s, len);
    strncpy(q[0]->strvals[0], path, len);
    q[0]->intvals[0] = (int)pathid;
    q[0]->intvals[1] = (int)parent;
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
}

int db_upsert_artist(app *aux, const char *artist, const char *artist_sort) {
    int artistid = db_find_artist(aux, artist);
    if (!artistid) {
    size_t len1 = strlen(artist) + 1;
    size_t len2 = strlen(artist_sort) + 1;
    SCRATCH *s  = scratch_new( 4*sizeof(query_t *) +
                               3*sizeof(query_t) +
                               2*sizeof(char *) +
                               len1 + 
                               len2);
    query_t **q = scratch_get(s, 4*sizeof(query_t *));
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_CLEAR_RETURN;
    q[1] = scratch_get(s, sizeof(query_t));
    q[1]->type = Q_UPSERT_ARTIST;
    q[1]->n_str = 2;
    q[1]->strvals = scratch_get(s, 2*sizeof(char *));
    q[1]->strvals[0] = scratch_get(s, len1);
    q[1]->strvals[1] = scratch_get(s, len2);
    strncpy(q[1]->strvals[0], artist, len1);
    strncpy(q[1]->strvals[1], artist_sort, len2);
    q[2] = scratch_get(s, sizeof(query_t));
    q[2]->type = Q_GET_RETURN;
    q[2]->returns = R_INT;
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
    sem_wait(&db_return_int_sem);
    int val = db_return_int;
    return val;
    } else return artistid;
}

int db_upsert_publisher(app *aux, const char *publisher) {
    int publisherid = db_find_publisher(aux, publisher);
    if (!publisherid) {
    size_t len = strlen(publisher) + 1;
    SCRATCH *s = scratch_new( 4*sizeof(query_t *) +
                              3*sizeof(query_t) +
                              1*sizeof(char *) +
                              len);
    query_t **q = scratch_get(s, 4*sizeof(query_t *));
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_CLEAR_RETURN;
    q[1] = scratch_get(s, sizeof(query_t));
    q[1]->type = Q_UPSERT_PUBLISHER;
    q[1]->n_str = 1;
    q[1]->strvals = scratch_get(s, sizeof(char *));
    q[1]->strvals[0] = scratch_get(s, len);
    strncpy(q[1]->strvals[0], publisher, len);
    q[2] = scratch_get(s, sizeof(query_t));
    q[2]->type = Q_GET_RETURN;
    q[2]->returns = R_INT;
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
    sem_wait(&db_return_int_sem);
    int val = db_return_int;
    return val;
    } else return publisherid;
}

int db_upsert_genre(app *aux, const char *genre) {
    int genreid = db_find_genre(aux, genre);
    if (!genreid) {
    size_t len = strlen(genre) + 1;
    SCRATCH *s = scratch_new( 4*sizeof(query_t *) +
                              3*sizeof(query_t) +
                              1*sizeof(char *) +
                              len);
    query_t **q = scratch_get(s, 4*sizeof(query_t *));
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_CLEAR_RETURN;
    q[1] = scratch_get(s, sizeof(query_t));
    q[1]->type = Q_UPSERT_GENRE;
    q[1]->n_str = 1;
    q[1]->strvals = scratch_get(s, sizeof(char *));
    q[1]->strvals[0] = scratch_get(s, len);
    strncpy(q[1]->strvals[0], genre, len);
    q[2] = scratch_get(s, sizeof(query_t));
    q[2]->type = Q_GET_RETURN;
    q[2]->returns = R_INT;
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
    sem_wait(&db_return_int_sem);
    int val = db_return_int;
    return val;
    } else return genreid;
}
int db_upsert_album(app *aux, const char *album, const char *album_sort, 
        const int artist, const int publisher, const int year, 
        const int total_tracks, const int total_discs) {
    int albumid = db_find_album(aux, album, artist, year);
    if (!albumid) {
    size_t len1 = strlen(album) + 1;
    size_t len2 = strlen(album_sort) + 1;
    SCRATCH  *s = scratch_new( 4*sizeof(query_t *) +
                               3*sizeof(query_t) +
                               5*sizeof(int) +
                               2*sizeof(char *) +
                               len1 +
                               len2);
    query_t **q = scratch_get(s, 4*sizeof(query_t *));
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_CLEAR_RETURN;
    q[1] = scratch_get(s, sizeof(query_t));
    q[1]->type = Q_UPSERT_ALBUM;
    q[1]->n_str = 2;
    q[1]->n_int = 5;
    q[1]->strvals = scratch_get(s, 2*sizeof(char *));
    q[1]->intvals = scratch_get(s, 5*sizeof(int));
    q[1]->strvals[0] = scratch_get(s, len1);
    q[1]->strvals[1] = scratch_get(s, len2);
    strncpy(q[1]->strvals[0], album, len1);
    strncpy(q[1]->strvals[1], album_sort, len2);
    q[1]->intvals[0] = (int)artist;
    q[1]->intvals[1] = (int)publisher;
    q[1]->intvals[2] = (int)year;
    q[1]->intvals[3] = (int)total_tracks;
    q[1]->intvals[4] = (int)total_discs;
    q[2] = scratch_get(s, sizeof(query_t));
    q[2]->type = Q_GET_RETURN;
    q[2]->returns = R_INT;
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
    sem_wait(&db_return_int_sem);
    int val = db_return_int;
    return val;
    } else return albumid;
}

int db_upsert_song(app *aux, const char *title, const int path, 
        const int artist, const int album, const int genre, 
        const int track, const int disc, const int song_length) {
    size_t len = strlen(title) + 1;
    SCRATCH *s = scratch_new( 2*sizeof(query_t *) +
                              1*sizeof(query_t) +
                              7*sizeof(int) +
                              1*sizeof(char *) +
                              len);
    query_t **q = scratch_get(s, 2*sizeof(query_t *));
    q[0] = scratch_get(s, sizeof(query_t));
    q[0]->type = Q_UPSERT_SONG;
    q[0]->n_str = 1;
    q[0]->n_int = 7;
    q[0]->strvals = scratch_get(s, sizeof(char *));
    q[0]->intvals = scratch_get(s, 7*sizeof(int));
    q[0]->strvals[0] = scratch_get(s, len);
    strncpy(q[0]->strvals[0], title, len);
    q[0]->intvals[0] = (int)path;
    q[0]->intvals[1] = (int)artist;
    q[0]->intvals[2] = (int)album;
    q[0]->intvals[3] = (int)genre;
    q[0]->intvals[4] = (int)track;
    q[0]->intvals[5] = (int)disc;
    q[0]->intvals[6] = (int)song_length;
    scratch_free(s, SCRATCH_KEEP);
    submit_write_query(q);
    return 0;
}

/**
 * @brief the db-write-access thread executes this to write to the database
 *        and return a value to the requesting thread if applicable
 *
 */
static int execute_write_query(app *aux, query_t **q_list) {
    int ret;
    int val;
    if (q_list == NULL) return -1;

    /*
    struct timeval now;
    uint64_t created = (*q_list)->created;
    int place = (*q_list)->place;
    gettimeofday(&now, NULL);
    uint64_t processed = TIMESTAMP(now);
    */

    uint64_t idle, writing;
    gettimeofday((struct timeval *)&write_started, NULL);
    idle = TIMESTAMP(write_started) - TIMESTAMP(write_finished);
    
    query_t *q = q_list[0];
    for(int i = 0; q; q = q_list[++i]) {
        if (q == NULL || aux == NULL) {
            LOGGER(LOG_ERR, "execute_write_query() got NULL");
            return -1;
        }
// a precompiled query, just bind params
        if (q->type < Q_PRECOMPILED_MAX) { 
            
            sqlite3_stmt *stmt = aux->stmts[q->type];
            int *intval = q->intvals;
            int col = 0;
            for (int i = 0; i < q->n_int; i++) {
                ret = sqlite3_bind_int(stmt, ++col, q->intvals[i]);
                if (ret != SQLITE_OK)
                    LOGGER(LOG_ERR, "failed to bind int column,");
            }
            int64_t *int64val = q->int64vals;
            for (int i = 0; i < q->n_int64; i++) {
                ret = sqlite3_bind_int64(stmt, ++col, q->int64vals[i]);
                if (ret != SQLITE_OK)
                    LOGGER(LOG_ERR, "failed to bind int64 column.");
            }
            char **strval = q->strvals;
            for (int i = 0; i < q->n_str; i++) {
                ret = sqlite3_bind_text(stmt, ++col, q->strvals[i], -1, NULL);
                if (ret != SQLITE_OK)
                    LOGGER(LOG_ERR, "failed to bind str column.");
            }
            while ((ret = sqlite3_step(stmt)) == SQLITE_BUSY);
            if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
                LOGGER(LOG_ERR, "failed to execute query '%s' error %d.", 
                        queries[q->type].name, ret);
            }
            if (q->returns == R_INT) {
                if (ret == SQLITE_ROW)
                    val = sqlite3_column_int(stmt, 0);
                db_return_int = val;
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
                sem_post(&db_return_int_sem);
            } else
            if (q->returns == R_STR) {
                if (ret == SQLITE_ROW) 
                    db_return_str = strdup(sqlite3_column_text(stmt, 1));
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
                sem_post(&db_return_str_sem);
            } else {
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
            }
        } else { 
// we have to prepare the statement
// so far, all of the write statements can be precompiled
            LOGGER(LOG_ERR, "query type not yet supported.");
        }
    }
    gettimeofday((struct timeval *)&write_finished, NULL);
    writing = TIMESTAMP(write_finished) - TIMESTAMP(write_started);

    //LOGGER(LOG_INFO, "[%.3f%% sqlite duty cycle]", (double)writing*100 / (writing + idle));
    
    /*
    uint64_t fulfilled;
    fulfilled = TIMESTAMP(now);

    if ((aux->config)->verbose)
        LOGGER(LOG_INFO, "[%lu, %lu, %lu, %d]", created, processed - created, fulfilled - processed, place);
    */
    free(q_list); // with SCRATCH, only one free() needed
}

/**
 * @brief a thread who wants to open the database read-only executes this
 *        in order to wait for the writer to open (and possibly create)
 *        the database, and possibly create the tables, triggers, indexes.
 *        Also, we know that the ring buffer will be initialized.
 */
void wait_for_writer() {
    pthread_mutex_lock(&writer_ready_mutex);
    while (writer_active == 0)
        pthread_cond_wait(&writer_ready, &writer_ready_mutex);
    pthread_mutex_unlock(&writer_ready_mutex);
}

/**
 * @brief this is called automatically when the writer thread is cancelled
 *
 */
static void writer_cleanup(void *arg) {
    app *state = (app *)arg;
    int ret;
    rb_free(writer_buffer);
    db_close_database(state);
    LOGGER(LOG_INFO, "writer thread terminated.");
}

void *writer_thread(void *arg) {
    writer_pid = pthread_self();
    //config_t *conf = (config_t *)arg;
    app state;
    state.config = &conf;
    int cleanup_pop_val;
    pthread_cleanup_push(writer_cleanup, &state);
// initialize ringbuffer
    writer_buffer = rb_init(conf.buffercap);
    if (!writer_buffer) {
        LOGGER(LOG_ERR, "failed to init writer_buffer[%l]!", conf.buffercap);
        exit(1);
    }
// initialize semaphore for database write results
    sem_init(&db_return_int_sem, 0, 0);
    int ret;
    LOGGER(LOG_INFO, "dbfile: %s", conf.dbfile);
    //db_open_database(&state, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
    db_open_database(&state, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX);
// this only needs to be called once per database creation
// but at this point, we don't know if we created or 
// merely opened just now
    ret = sqlite3_exec(state.db, "PRAGMA journal_mode = WAL;", 0, 0, 0);
    if (ret != SQLITE_OK) {
        LOGGER(LOG_ERR, "failed to set sqlite3 journal_mode = WAL");
    }
    ret = sqlite3_exec(state.db, "PRAGMA synchronous = OFF;", 0, 0, 0);
    if (ret != SQLITE_OK) {
        LOGGER(LOG_ERR, "failed to set sqlite3 syncronous = OFF");
    }
    ret = sqlite3_exec(state.db, "PRAGMA temp_store = MEMORY;", 0, 0, 0);
    if (ret != SQLITE_OK) {
        LOGGER(LOG_ERR, "failed to set sqlite3 temp_store = MEMORY");
    }
// create tables, indexes, triggers
    for (t_type t = 0; t < T_MAX; t++) {
        ret = sqlite3_exec(state.db, tables[t].query, 0, 0, 0);
        if (ret != SQLITE_OK) {
            LOGGER(LOG_ERR, "failed to create table '%s'", tables[t].name);
        }
    }
    precompile_statements(&state);
// alert threads that the database is up and ready for action
    pthread_mutex_lock(&writer_ready_mutex);
    writer_active = 1;
    pthread_mutex_unlock(&writer_ready_mutex);
    pthread_cond_broadcast(&writer_ready);
// writer main loop
// TBD: batch drain the ringbuffer vs individual pops 
    int count;
    int bufsize = conf.buffercap;
    query_t ***buf = calloc(bufsize, sizeof(query_t **));
    LOGGER(LOG_INFO, "writer thread active.");
    while (writer_active) {
        if (!conf.sequential) {
        //query_t **q = rb_popfront(writer_buffer);
            count = rb_drain(writer_buffer, (void **)buf, bufsize);
            LOGGER(LOG_INFO, "writer got %d elements", count);
            for (int i = 0; i < count; i++) {
                if (buf[i])
                    execute_write_query(&state, buf[i]);
            }
        } else {
            query_t **q = rb_popfront(writer_buffer);
            if(q)
                execute_write_query(&state, q);
        }
    }
    pthread_cleanup_pop(cleanup_pop_val);
    return NULL;
}   
