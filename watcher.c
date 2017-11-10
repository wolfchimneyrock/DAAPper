#include <stdint.h>
#include <syslog.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fts.h>
#include "util.h"
#include <libfswatch/c/libfswatch.h>
#include <sqlite3.h>
#include "config.h"
#include "vector.h"
#include "watcher.h"
#include "writer.h"
#include "scanner.h"
#include "sql.h"
#include "system.h"
#include "id3cb.h"
#include "scratch.h"

#define WATCH_SLEEP_INTERVAL 1000
#define META_SCRATCH_SIZE    4096
#define PATH_SCRATCH_SIZE    4096 

volatile sig_atomic_t watcher_active  = 0;
volatile sig_atomic_t library_updated = 0;
volatile sig_atomic_t watcher_fd;
// this will provide an id for each detected change to the filesystem
static int watchid = 0;
// protect this thread's resources
pthread_mutex_t watchid_mutex       = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t watcher_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  watcher_ready       = PTHREAD_COND_INITIALIZER;

void wait_for_watcher() {
    pthread_mutex_lock(&watcher_ready_mutex);
    while (watcher_active == 0)
        pthread_cond_wait(&watcher_ready, &watcher_ready_mutex);
    pthread_mutex_unlock(&watcher_ready_mutex);
}

// this function is called when the watcher detects a change
void library_change_cb(fsw_cevent const *const events, 
                       const unsigned int event_num, void *data) {
    app *state = (app *)data;   
    vector moveTo, moveFrom, removed, created, updated, renamed;
    vector_new(&moveTo,   20);
    vector_new(&moveFrom, 20);
    vector_new(&updated,  20);
    vector_new(&renamed,  20);
    SCRATCH *s = scratch_new(PATH_SCRATCH_SIZE);    
    int this_id;
    pthread_mutex_lock(&watchid_mutex);
    this_id = ++watchid;
    pthread_mutex_unlock(&watchid_mutex);
    struct stat st;
    for (int i = 0; i < event_num; i++) {
        enum fsw_event_flag *f = events[i].flags;
        int all_flags = 0;
        for (int j = 0; j < events[i].flags_num; j++) all_flags |= f[j];
// when we respond to a create event, the file may not be all there yet.
// the difficulty is that as a file is copied into a folder, 
// it triggers multiple update events.
// so we can't blindly respond to update events.
        syslog(LOG_INFO, "[%d, %d] %s\n", 
                         this_id, all_flags, events[i].path);
        switch(all_flags) {
            case Removed: {
                char **path = scratch_head(s);
                int pathid = db_find_path(state, events[i].path, s);
                scratch_reset(s);
                db_remove_file(pathid);
                syslog(LOG_INFO, "removing file '%s'\n", events[i].path);
                          }
            break;
            case Created: {
                syslog(LOG_INFO, "added file '%s'\n", events[i].path);
                scanner_submit_request(strdup(events[i].path));
                          }
            break;
            case MovedTo | Created:
                syslog(LOG_INFO, "'%s' created\n", events[i].path);
            vector_pushback(&moveTo, strdup(events[i].path));
            break;
            case MovedFrom | Removed:
                syslog(LOG_INFO, "'%s' removed\n", events[i].path);
            vector_pushback(&moveFrom, strdup(events[i].path));
            break;
            case Updated:
                syslog(LOG_INFO, "'%s' renamed\n", events[i].path);
            vector_pushback(&updated, strdup(events[i].path));
        }
    }
    while (!vector_isempty(&moveTo) && !vector_isempty(&moveFrom)) {
        char *frompath = strdup(vector_peekback(&moveFrom));
        int pathid = db_find_path(state, frompath, s);
        scratch_reset(s);
        char *fromfile = split_filename(frompath);
        char *topath   = strdup(vector_peekback(&moveTo));
        char *tofile   = split_filename(topath);
        int newparent;
        if (strcmp(topath, state->config->root)) {
            newparent = db_find_path(state, topath, s);
        } 
        else newparent = 1;
        scratch_reset(s);

        syslog(LOG_INFO, "moved %d [%s/%s] to parent %d [%s/%s]\n",
                pathid, frompath, fromfile, newparent, topath, tofile);
        db_change_path(state, pathid, tofile, newparent);

        vector_popback(&moveFrom);
        vector_popback(&moveTo);
        free(frompath);
        free(topath);

    }
    vector_free(&moveTo);
    vector_free(&moveFrom);
    vector_free(&updated);
    scratch_free(s, SCRATCH_FREE);
}

/**
 * @brief this is called automatically when watcher thread is cancelled
 *
 */
static void watcher_cleanup(void *arg) {
    app *state = (app *)arg;
    fsw_destroy_session(state->fd);
    db_close_database(state);
    syslog(LOG_INFO, "watcher thread terminated.\n");
}

void *watcher_thread(void *arg) {
    syslog(LOG_INFO, "watcher thread starting...\n");
    watcher_pid = pthread_self();
    const fsw_event_type_filter include_created   = { Created };
    const fsw_event_type_filter include_removed   = { Removed };
    const fsw_event_type_filter include_movedTo   = { MovedTo };
    const fsw_event_type_filter include_movedFrom = { MovedFrom };
    const fsw_event_type_filter include_updated   = { Updated };
    const fsw_event_type_filter include_renamed   = { Renamed };
    app state;
    int cleanup_pop_val;
    int ret;
    config_t *conf = (config_t *)arg;
    state.config = conf;
    wait_for_writer();
    pthread_cleanup_push(watcher_cleanup, &state);
    state.fd = fsw_init_session(system_default_monitor_type);
// open our own database connection read-only
    db_open_database(&state, SQLITE_OPEN_READONLY); 
    precompile_statements(&state);  
    fsw_set_callback(state.fd, library_change_cb, &state);
    fsw_add_event_type_filter(state.fd, include_created);
    fsw_add_event_type_filter(state.fd, include_removed);
    fsw_add_event_type_filter(state.fd, include_movedTo);
    fsw_add_event_type_filter(state.fd, include_movedFrom);
    fsw_add_event_type_filter(state.fd, include_updated);
    fsw_add_event_type_filter(state.fd, include_renamed);
    pthread_mutex_lock(&watcher_ready_mutex);
    watcher_active = 1;
    pthread_mutex_unlock(&watcher_ready_mutex);
    pthread_cond_broadcast(&watcher_ready);
    char *paths[2] = {0};
    paths[0] = conf->root;
    FTS *tree = fts_open(paths, FTS_NOCHDIR | FTS_NOSTAT, 0);
    FTSENT *node;
    while (node = fts_read(tree))
        if (node->fts_info & FTS_D)
            fsw_add_path(state.fd, node->fts_path);
    wait_for_scanner();
    syslog(LOG_INFO, "watcher thread active.\n");
    fsw_start_monitor(state.fd);
    while (watcher_active) {
        pthread_testcancel();
        sleep(WATCH_SLEEP_INTERVAL);
    }
    syslog(LOG_INFO, "watcher thread terminated\n");
    pthread_cleanup_pop(cleanup_pop_val);
    return NULL;
}
