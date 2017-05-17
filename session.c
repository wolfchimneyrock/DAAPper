#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "session.h"
#include "vector.h"
#include "writer.h"
#include "sql.h"

typedef struct session_t {
    status_t status;
    time_t   time_created,
             time_updated;
} session_t;

static vector sessions;
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

extern volatile time_t db_updated;

void sessions_init() {
    pthread_mutex_lock(&sessions_mutex);
    vector_new(&sessions, 10);
    pthread_mutex_unlock(&sessions_mutex);
}

int sessions_add() {
    int id;
    time_t now;
    time(&now);
    session_t *s     = malloc(sizeof(session_t));
    s->status        = ST_DIRTY;// born dirty, i.e. needs an update
    s->time_created  = now; 
    s->time_updated  = 0;       // never been updated yet   

    pthread_mutex_lock(&sessions_mutex);
    id = sessions.used;
    vector_pushback(&sessions, s);
    pthread_mutex_unlock(&sessions_mutex);
    return id;
}

void sessions_touch(size_t s) { 
    time_t now;
    time(&now);
    pthread_mutex_lock(&sessions_mutex);
    if (s < sessions.used) {
        session_t *sess = sessions.data[s];
        if (sess->status != ST_DEAD) {
            sess->time_updated = now;
            sess->status = ST_CLEAN;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
}

void sessions_kill(size_t s) {
    time_t now;
    time(&now);
    pthread_mutex_lock(&sessions_mutex);
    if (s < sessions.used) {
        session_t *sess = sessions.data[s];
        if (sess->status != ST_DEAD) {
            sess->time_updated = now;
            sess->status = ST_DEAD;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
}

status_t session_get_status(size_t s) {
    status_t result = ST_DEAD;
    time_t db = db_last_update_time();
    pthread_mutex_lock(&sessions_mutex);
    if (s < sessions.used) {
        session_t *sess = sessions.data[s];
        if (sess->status == ST_CLEAN && difftime(db, sess->time_updated) > 0)
            sess->status = ST_DIRTY;
        result = sess->status;
    }
    pthread_mutex_unlock(&sessions_mutex);
    return result;
}

void sessions_cleanup() {
    pthread_mutex_lock(&sessions_mutex);
    for (size_t i = 0; i < sessions.used; i++) {
        session_t *s = sessions.data[i];
        free(s);
    }
    vector_free(&sessions);
}
