#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "evhtp/evhtp.h"
#include <event2/util.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include "config.h"
#include "dmap.h"
#include "util.h"
#include "meta.h"
#include "http.h"
#include "session.h"
#include "sql.h"
#include "watcher.h"
#include "writer.h"
#include "scanner.h"
#include "system.h"

/**
 * @brief this is called automatically when the main thread is cancelled
 */
static void main_cleanup(void *arg) {
    app_parent *parent = (app_parent *)arg;
    event_base_loopbreak(parent->base);
    evhtp_unbind_socket(parent->htp);
    evhtp_free(parent->htp);
    event_base_free(parent->base);
    watcher_active = 0;
    writer_active = 0;
    syslog(LOG_INFO, "main thread terminated.\n");
}

int main (int argc, char *argv[]) {
    main_pid = pthread_self();
    int res;
    evbase_t *evbase;
    evhtp_t  *evhtp;
    app_parent parent;
    static config_t conf;
    char hostname[255];
    gethostname(hostname, 255);
// default values if unable to read configuration file
    conf.dbfile       = strdup("${DAAPPER_DBFILE:-/tmp/songs3.db}");
    conf.extfile      = strdup("${DAAPPER_EXTFILE:-/usr/lib/sqlite3/closure.so}");
    conf.port         = 3689;
    conf.threads      = 4;
    conf.timeout      = 1800;
    conf.root         = strdup("${DAAPPER_ROOT:-/srv/music}");
    conf.userid       = strdup("${DAAPPER_USER:-nobody}");
    conf.fullscan     = 0;
    conf.server_name  = strdup(hostname);
    conf.library_name = strdup("${USER:-User}'s Library");
    get_config(&conf, "daap.conf");
// unix specific initialization in system.c
// TBD windows version
    daemonize(&conf, argv);
    assign_signal_handler();
    atexit(cleanup);
    int cleanup_pop_val;
    pthread_cleanup_push(main_cleanup, &parent);
// WRITER thread:
// this is the only thread with write access to the database
// other threads must request writes by adding a query request
// to a global ringbuffer
    pthread_create((pthread_t *)&writer_pid, NULL, &writer_thread, &conf);
// WATCHER thread:
// this is the thread that watches for library changes,
// and submits updates to the SCANNER thread
    pthread_create((pthread_t *)&watcher_pid, NULL, &watcher_thread, &conf);
// SCANNER thread:
// this is the thread that scans mpeg files for metadata
    pthread_create((pthread_t *)&scanner_pid, NULL, &scanner_thread, &conf);
// this maybe should be moved to WRITER thread initialization?
// this maintains "clean" and "dirty" status of the database
    db_init_status();
// reset global DAAP sessions
    sessions_init();
// the rest of this is boilerplate LIBEVENT / EVHTP for a multi 
// threaded, thread-pooling HTTP server.  Application specific 
// callbacks are located in http.c
    syslog(LOG_INFO, "initialize event_base...\n");
    parent.base = event_base_new();
    syslog(LOG_INFO, "initialize evhtp_base...\n");
    parent.htp  = evhtp_new(parent.base, NULL);
    parent.config = &conf;
    register_callbacks(parent.htp);
    evhtp_use_threads_wexit(parent.htp, app_init_thread, app_term_thread, 
                            conf.threads, &parent);
    int socket = 0;
    if (argc > 1) socket = atoi(argv[1]);
    if (socket == 0) socket = conf.port;
    syslog(LOG_INFO, "binding socket %d...\n", socket);
    if ((res = evhtp_bind_socket(parent.htp, "0.0.0.0", socket, 2048)) 
            == -1) {
        syslog(LOG_EMERG, "failed to bind socket error %d.\n", errno);
        evhtp_free(parent.htp);
        event_base_free(parent.base);
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "event loop...\n");
    event_base_loop(parent.base, 0);
    pthread_cleanup_pop(cleanup_pop_val);
    return EXIT_SUCCESS;
}
