#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <event2/event.h>
#include <evhtp/evhtp.h>
#include <sqlite3.h>
#include <syslog.h>
#include "config.h"
#include "http.h"
#include "vector.h"
#include "meta.h"
#include "dmap.h"
#include "util.h"
#include "sql.h"
#include "session.h"
#include "writer.h"
#include "scratch.h"
#include "stream.h"

static const int  current_rev = 2;

#define DB_STR  "^/databases/"
#define REG_NUM "([0-9]+)"

const char *server_name = "LULU";
const char *library_name = "Robert";

static volatile int threads = 0;
pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;

static evthr_t *get_request_thr(evhtp_request_t * request) {
    evhtp_connection_t * htpconn;
    evthr_t            * thread;

    htpconn = evhtp_request_get_connection(request);
    thread  = htpconn->thread;

    return thread;
}

void app_init_thread(evhtp_t *htp, evthr_t *thread, void *arg) {
// per thread init here: open db, etc
    app_parent *parent = (app_parent *)arg;
    app *aux = malloc(sizeof(app));
        
    pthread_mutex_lock(&threads_mutex);
    aux->fd = ++threads;
    pthread_mutex_unlock(&threads_mutex);

    aux->parent = parent;
    aux->base   = evthr_get_base(thread);
    aux->config = parent->config;
// wait until writer thread is ready 
    wait_for_writer();
    db_open_database(aux, SQLITE_OPEN_READONLY|SQLITE_OPEN_NOMUTEX|SQLITE_OPEN_SHAREDCACHE);
    precompile_statements(aux);
// to be retrieved by request callbacks that need
    evthr_set_aux(thread, aux); 
    syslog(LOG_INFO, "evhtp thread listening for connections.\n");
}

void app_term_thread(evhtp_t *htp, evthr_t *thread, void *arg) {
    app *aux = (app *)evthr_get_aux(thread);
    db_close_database(aux);
    free(aux);
    syslog(LOG_INFO, "evhtp thread terminated.\n");
}

void add_headers_out(evhtp_request_t *req) {
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Accept-Ranges", "bytes", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("DAAP-Server", SERVER_IDENTIFIER, 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Content-Type", "application/x-dmap-tagged", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Access-Control-Allow-Origin", "*", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Server", SERVER_IDENTIFIER, 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Expires", "-1", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Cache-Control", "no-cache", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Content-Language", "en_us", 0, 0));
    evhtp_headers_add_header(req->headers_out,
          evhtp_header_new("Connection", "keep-alive", 0, 0));
          //evhtp_header_new("Connection", "close", 0, 0));
}

// generic response for unassigned uri
void server_xmlrpc(evhtp_request_t *req, void *a) {
    log_request(req, a);
    evhtp_send_reply(req, EVHTP_RES_OK);
    syslog(LOG_INFO, "received request %s: %s - %s\n", 
            (char *)a, req->uri->path->path, req->uri->path->file); 
}

void add_list(struct evbuffer *out, struct evbuffer *payload, 
              char *code, int mtco, int mrco) {
    int len = evbuffer_get_length(payload);
    dmap_add_list(out, code, len + 53);
    dmap_add_int (out, "mstt", 200);
    dmap_add_char(out, "muty", 0);
    dmap_add_int (out, "mtco", mtco);
    dmap_add_int (out, "mrco", mrco);
    dmap_add_list(out, "mlcl", len);
    evbuffer_add_buffer(out, payload);
}

void res_login(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    int request_session_id;
    const char *rsi = evhtp_kv_find(req->headers_in, 
                                    "Request-Session-Id");
    if (rsi) {
        int result = atoi(rsi);
        if (result < 0) request_session_id = 0;
        else request_session_id = result;
    } else request_session_id = 0;
    int session =  sessions_add();
    dmap_add_list(req->buffer_out, "mlog", 24);
    dmap_add_int (req->buffer_out, "mstt", 200);      
    dmap_add_int (req->buffer_out, "mlid", session);
    evhtp_send_reply(req, EVHTP_RES_OK);
}

void res_logout(evhtp_request_t *req, void *a) {
    log_request(req, a);
    const char *s_str = evhtp_kv_find(req->uri->query, "session-id");
    int s = atoi(s_str);
    sessions_kill(s);
    evhtp_send_reply(req, EVHTP_RES_NOCONTENT);
}

struct update_t {
    evhtp_request_t *req;
    int session_id;
};

void update_timer_cb(int fd, short event, void *a) { }

void res_update(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    const char *s_str  = evhtp_kv_find(req->uri->query, "session-id");
    int s = s_str ? atoi(s_str) : -1;
    const char *rev_no = evhtp_kv_find(req->uri->query, "revision-number");
    int rev = rev_no ? atoi(rev_no) : 1;
    if (rev == 1) {
        dmap_add_list(req->buffer_out, "mupd", 24);
        dmap_add_int (req->buffer_out, "mstt", 200);
        dmap_add_int (req->buffer_out, "musr", 2 /*current_rev*/);  // 2
        evhtp_send_reply(req, EVHTP_RES_OK);
    }
    // revision-number: 2+ is left open until a change occurs on the db
    // since the db is currently read-only, this will never close unless 
    // client disconnects
//  evhtp_request_pause(req);
}

void res_database_list(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    evhtp_query_t *query = req->uri->query;
    evthr_t *thread = get_request_thr(req);
    app *aux = (app *)evthr_get_aux(thread);
    int library_size   = count_precompiled_items(aux, 
                         Q_COUNT_ALL_SONGS, NULL);
    int playlist_count = count_precompiled_items(aux, 
                         Q_COUNT_ALL_PLAYLISTS, NULL);
    struct evbuffer *item, *payload;
    char radio_name[] = "radio";
    payload = evbuffer_new();
    item = evbuffer_new();
    // magic codes from iTunes
    dmap_add_int    (item, "miid", 1);
    dmap_add_long   (item, "mper", 1);  /// long??
    dmap_add_int    (item, "mdbk", 1);
    dmap_add_int    (item, "aeCs", 1);
    dmap_add_string (item, "minm", (aux->config)->library_name);
    dmap_add_int    (item, "mimc", library_size);
    dmap_add_int    (item, "mctc", playlist_count);
    //dmap_add_int (item, "mctc", 1);
    dmap_add_int    (item, "meds", 3);
    int len = evbuffer_get_length(item);
    dmap_add_list(payload, "mlit", len);
    evbuffer_add_buffer(payload, item);
    evbuffer_free(item);
    item = evbuffer_new();
    dmap_add_int    (item, "miid", 2);
    dmap_add_long   (item, "mper", 2);  /// long??
    dmap_add_int    (item, "mdbk", 0x64);
    dmap_add_int    (item, "aeCs", 0);
    dmap_add_string (item, "minm", radio_name);
    dmap_add_int    (item, "mimc", playlist_count);
    dmap_add_int    (item, "mctc", 0);
    dmap_add_int    (item, "aeMk", 1);
    dmap_add_int    (item, "meds", 3);
    len = evbuffer_get_length(item);
    dmap_add_list(payload, "mlit", len);
    evbuffer_add_buffer(payload, item);
    evbuffer_free(item);
    add_list(req->buffer_out, payload, "avdb", 2, 2);
    evhtp_send_reply(req, EVHTP_RES_OK);
    evbuffer_free(payload);
}

/**
 * @brief callback for each item added
 * @return 1 if the item is to be added, or 0 if it is to be skipped.
 */
int containerlist_itemcb(app *aux, struct evbuffer *item, meta_info_t *info) {
        sqlite3 *db = aux->db;
        if (info->id != 1) {
            if (info->kind == PL_SPECIAL || info->kind == PL_SMART) 
                dmap_add_char(item, "aeSP", 1); // special playlist
            else
                dmap_add_char(item, "aeSP", 0);
            if (info->kind == PL_SMART)
                dmap_add_char(item, "aePS", 1); // smart playlist
            else
                dmap_add_char(item, "aePS", 0);
        }
        dmap_add_char(item, "aePP", 0); // podcast
        dmap_add_char(item, "aeSG", 0); // "saved genius"
        int qty;    
        char *query_str = (char *)get_smart_playlist_query(aux, info->id);
        vector clauses;
        vector_new(&clauses, 2);  // "where" and query
        if (strcmp(query_str, "(NULL)")) {
            vector_pushback(&clauses, "WHERE");
            vector_pushback(&clauses, query_str); // the playlist's query string
            qty = count_smart_items(aux, T_SONGS, &clauses, NULL);
        } else 
            qty = count_precompiled_items(aux, Q_COUNT_PL_ITEMS, &info->id);
        vector_free(&clauses);
        free(query_str);
        dmap_add_int(item, "mimc", qty);
        if (info->id == 1)  { // first playlist is "base" playlist
            dmap_add_char (item, "abpl", 1);
            dmap_add_int  (item, "mpco", 0); // parentcontainerid
        } else
            dmap_add_int  (item, "mpco", 1);
        return 1; // (qty > 0 || info->kind != PL_SPECIAL);
}

void res_container_list(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    evhtp_query_t *query = req->uri->query;
    evthr_t *thread = get_request_thr(req);
    app *aux = (app *)evthr_get_aux(thread);
    query_t q;
    int len;
    char **meta = NULL;
    char *raw_meta = NULL;
    int nmeta = 0;
    const char *param = evhtp_kv_find(query, "meta");
    int npl = count_precompiled_items(aux, Q_COUNT_ALL_PLAYLISTS, NULL);
    syslog(LOG_INFO, "Found %d playlists.\n", npl);
    if (param == NULL) {
        meta  = (char **)default_meta_container;
    } else {
        raw_meta = strdup(param);
        meta_parse(raw_meta, &meta, &nmeta); 
    }
    vector tags;
    vector_new(&tags, 20);
// to be able to tell if a playlist is "smart", we need at least this meta
// tag, so if the client specifies metatags we need to make sure this is there
//if (param && strstr(raw_meta, "dmap.itemkind") == NULL)   
    vector_pushback(&tags, container_meta_tags); // dmap.itemkind == 0 index
    int i = -1;
    while (meta[++i]) {
        int index = meta_find_container_tag(meta[i]);
        if (index >= 0)
            vector_pushback(&tags, container_meta_tags + index);
        else syslog(LOG_NOTICE, "unrecognized metatag: %s\n", meta[i]);
    }
    char *sqlstr;
    q.type = Q_CONTAINERLIST;
    struct evbuffer *item, *payload;
    payload = evbuffer_new();
    sql_build_query(&q, &tags, NULL, &sqlstr);
    int nitems = 0, ret;
    nitems = count_precompiled_items(aux, Q_COUNT_ALL_PLAYLISTS, NULL); 
    struct evbuffer **items = malloc (nitems * sizeof(struct evbuffer *));
    ret = sql_put_results(&payload, items, aux, &tags, 1, 
                          sqlstr, NULL, containerlist_itemcb); 
    add_list(req->buffer_out, payload, "aply", nitems, nitems);
    evhtp_send_reply(req, EVHTP_RES_OK);
    syslog(LOG_INFO, "sent %d playlists.\n", nitems);
    // cleanup
    if (raw_meta) free(raw_meta);
    if (meta && meta != (char **)default_meta_container) free(meta);
    vector_free(&tags);
    free(sqlstr);
    for (int n = 0; n < nitems; n++) evbuffer_free(items[n]);
    free(items);
    evbuffer_free(payload);
}

/**
 * @brief http handler for retrieving list of items in a container
 *
 */
void res_container_items(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    evhtp_query_t *query = req->uri->query;
    evthr_t *thread = get_request_thr(req);
    app *aux = (app *)evthr_get_aux(thread);
    query_t q;
    int len;
    const char *path = req->uri->path->path;
    int pl_num = uri_get_number(path, 1);
    const char *param = evhtp_kv_find(query, "meta");
    char *raw_meta;
    char **meta;
    int nmeta;
    if (param) {
        raw_meta = strdup(param);
        meta_parse(raw_meta, &meta, &nmeta); 
    }
    else {
        raw_meta = NULL;
        meta = (char **)default_meta_items;
    }
    vector tags;
    vector_new(&tags, 2);
    int i = -1;
    while (meta[++i]) {
        int index = meta_find_item_tag(meta[i]);
        if (index >= 0)
            vector_pushback(&tags, item_meta_tags + index);
        else syslog(LOG_NOTICE, "unrecognized metatag: %s\n", meta[i]);
    }
    char *sqlstr;
    size_t nitems;
    vector clauses;
    vector_new(&clauses, 2); // "WHERE" and "ORDER" clauses
    char *query_str = (char *)get_smart_playlist_query(aux, pl_num);
    if (strcmp(query_str, "(NULL)")) {
        q.type = Q_ITEMLIST;
        vector_pushback(&clauses, "WHERE");
        vector_pushback(&clauses, query_str); 
        // the playlist's query string
        sql_build_query(&q, &tags, &clauses, &sqlstr);
        nitems = count_smart_items(aux, T_SONGS, &clauses, NULL);
    } else {
        q.type = Q_CONTAINERITEMS;
        vector_pushback(&clauses, "WHERE s.path = pi.filepath AND pi.playlistid = ?");
        vector_pushback(&clauses, "ORDER BY pi.id");
        sql_build_query(&q, &tags, &clauses, &sqlstr);
        //nitems = count_items(aux->db, T_PLAYLISTITEMS, &clauses, &pl_num);
        nitems = count_precompiled_items(aux, Q_COUNT_PL_ITEMS, &pl_num);
    }
    struct evbuffer *payload;
    payload = evbuffer_new();
    evbuffer_expand(payload, 100000);
    vector_free(&clauses);
    // all evbuffers must be created in scope of the evhtp_send_reply()
    struct evbuffer **items = malloc (nitems * sizeof(struct evbuffer *));
    int ritems;
    if (q.type == Q_ITEMLIST)
        ritems = sql_put_results(&payload, items, aux, &tags, 0, 
                                 sqlstr, NULL, NULL);
    else
        ritems = sql_put_results(&payload, items, aux, &tags, 0, 
                                 sqlstr, &pl_num, NULL);
    add_list(req->buffer_out, payload, "apso", ritems, ritems);
    syslog(LOG_INFO, "found %d items\n", ritems);
    syslog(LOG_INFO, "sending %lu bytes...\n", 
                     evbuffer_get_length(req->buffer_out));
    evhtp_send_reply(req, EVHTP_RES_OK);
    // cleanup
    evbuffer_free(payload);
    if (meta && meta != (char **)default_meta_items)
        free(meta);
    for (int i = 0; i < nitems; i++) evbuffer_free(items[i]);
    free(items);
    vector_free(&tags);
    free(sqlstr);
    free(query_str);
    if (raw_meta) free(raw_meta);
}

void res_item_list(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    evhtp_query_t *query = req->uri->query;
    evthr_t *thread = get_request_thr(req);
    app *aux = (app *)evthr_get_aux(thread);
    query_t q;
    int len;
    const char *param = evhtp_kv_find(query, "meta");
    char *raw_meta;
    char **meta;
    int nmeta;
    if (param && strcmp(param, "all")) {
        raw_meta = strdup(param);
        meta_parse(raw_meta, &meta, &nmeta); 
    } else {
        raw_meta = NULL;
        meta = (char **)default_meta_items;
    }
    vector tags;
    vector_new(&tags, 20);
    for (int i = -1; meta[++i]; ) {
        int index = meta_find_item_tag(meta[i]);
        if (index >= 0)
            vector_pushback(&tags, item_meta_tags + index);
        else syslog(LOG_NOTICE, "unrecognized metatag: %s\n", meta[i]);
    }
    char *sqlstr;
    q.type = Q_ITEMLIST;
    struct evbuffer *payload;
    payload = evbuffer_new();
    sql_build_query(&q, &tags, NULL, &sqlstr); // no clauses
    size_t nitems = count_all_files(aux);
    struct evbuffer **items = malloc(nitems * sizeof(struct evbuffer *));
    int ritems = sql_put_results(&payload, items, aux, &tags, 0, 
                                 sqlstr, NULL, NULL);
    add_list(req->buffer_out, payload, "adbs", ritems, ritems);
    syslog(LOG_INFO, "found %lu items.\n", nitems);
    syslog(LOG_INFO, "sending %lu bytes...\n", 
                     evbuffer_get_length(req->buffer_out));
    evhtp_send_reply(req, EVHTP_RES_OK);
    // cleanup
    if (meta && meta != (char **)default_meta_items)
        free(meta);
    evbuffer_free(payload);
    for (int i = 0; i < nitems; i++) 
        evbuffer_free(items[i]);
    free(items);
    vector_free(&tags);
    free(sqlstr);
    if (raw_meta) free(raw_meta);
}

void res_content_codes(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    EVBUF *item, *payload;
    payload = evbuffer_new();
    dmap_add_int(payload, "mstt", 200);
    for (int i = -1; misc_meta_tags[++i].tag; ) {
        item = evbuffer_new();
        dmap_add_string(item, "mcnm", misc_meta_tags[i].tag);
        dmap_add_string(item, "mcna", misc_meta_tags[i].meta_tag);
        dmap_add_short (item, "mcty", (short)misc_meta_tags[i].type);
        dmap_add_list(payload, "mdcl", evbuffer_get_length(item));
        evbuffer_add_buffer(payload, item);
    }
    for (int i = -1; item_meta_tags[++i].tag; ) {
        item = evbuffer_new();
        dmap_add_string(item, "mcnm", item_meta_tags[i].tag);
        dmap_add_string(item, "mcna", item_meta_tags[i].meta_tag);
        dmap_add_short (item, "mcty", (short)item_meta_tags[i].type);
        dmap_add_list(payload, "mdcl", evbuffer_get_length(item));
        evbuffer_add_buffer(payload, item);
    }
    for (int i = -1; container_meta_tags[++i].tag; ) {
        item = evbuffer_new();
        dmap_add_string(item, "mcnm", container_meta_tags[i].tag);
        dmap_add_string(item, "mcna", container_meta_tags[i].meta_tag);
        dmap_add_short (item, "mcty", (short)container_meta_tags[i].type);
        dmap_add_list(payload, "mdcl", evbuffer_get_length(item));
        evbuffer_add_buffer(payload, item);
    }
    dmap_add_list(req->buffer_out, "mccr", evbuffer_get_length(payload));
    evbuffer_add_buffer(req->buffer_out, payload);
    evhtp_send_reply(req, EVHTP_RES_OK);
    evbuffer_free(payload);
}

void res_server_info(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    evthr_t *thread = get_request_thr(req);
    app     *aux    = (app *)evthr_get_aux(thread);
    struct evbuffer *payload;
    payload = evbuffer_new();
// actual contents reverse engineered from Apple iTunes
    int mpro = 2 << 16 | 10;
    int apro = 3 << 16 | 12;
// dmap.status
    dmap_add_int   (payload, "mstt", 200);           
// dmap.protocolversion 
    dmap_add_int   (payload, "mpro", mpro);          
// daap.protocolversion
    dmap_add_int   (payload, "apro", apro);          
// com.apple.itunes.music-sharing-version
    //dmap_add_int (payload, "aeSV", apro);          
// the following are unrecognized by linux iTunes 4.6 clone
// but probably for newer version of iTunes.
// daap.supportsextradata
    dmap_add_short (payload, "ated", 7);           
// daap.supportsgroups
    dmap_add_short (payload, "asgr", 3);          
    dmap_add_char  (payload, "aeMQ", 1);
    dmap_add_char  (payload, "aeTr", 1);
    dmap_add_char  (payload, "aeSL", 1);
    dmap_add_char  (payload, "aeSR", 1);
// dmap.supportsedit
    dmap_add_char  (payload, "msed", 1);           
// dmap.requires login
    dmap_add_char  (payload, "mslr", 1);           
//dmap.timeoutinterval
    dmap_add_int   (payload, "mstm", (aux->config)->timeout); 
// dmap.itemname
    dmap_add_string(payload, "minm", (aux->config)->server_name); 
// dmap.supportsautologout
    dmap_add_char  (payload, "msal", 1);             
// 0 - no password, 2 - passwd
    dmap_add_char  (payload, "msau", 0); 
// dmap.supportsextensions
    dmap_add_char  (payload, "msex", 1);             
// dmap.supportsindex
    dmap_add_char  (payload, "msix", 1);             
// dmap.supportsbrowse
    dmap_add_char  (payload, "msbr", 1);             
// dmap.supportsquery
    dmap_add_char  (payload, "msqy", 1);             
// dmap.supportspersistentids
    dmap_add_char  (payload, "mspi", 1);             
// dmap.supportsupdate
    dmap_add_char  (payload, "msup", 1);             
// dmap.databasescount
    dmap_add_int   (payload, "msdc", 2);             
    int len = evbuffer_get_length(payload);
// server info
    dmap_add_list(req->buffer_out, "msrv", len);   
    evbuffer_add_buffer(req->buffer_out, payload);
    evhtp_send_reply(req, EVHTP_RES_OK);
    evbuffer_free(payload);
}

int grouplist_itemcb(app *aux, struct evbuffer *item, meta_info_t *info) {
    int qty;
    if (info->kind == G_ALBUM) 
        qty = count_precompiled_items(aux, 
                Q_COUNT_ALL_ALBUMS, &info->id);
    else
        qty = count_precompiled_items(aux, 
                Q_COUNT_ALL_ARTISTS, &info->id);
    dmap_add_int(item, "mimc", qty);
// add albumartist if the group is an album
    if (info->kind == G_ALBUM && qty > 0) {
        char *asaa = (char *)get_albumgroup_artist(aux, info->id);
        dmap_add_string(item, "asaa", asaa);
        free(asaa);
    }
    return qty > 1;  // don't add singleton or empty groups
}

void res_group_list(evhtp_request_t *req, void *a) {
    log_request(req, a);
    add_headers_out(req);
    ADD_DATE_HEADER("Date");
    evthr_t *thread = get_request_thr(req);
    app     *aux    = (app *)evthr_get_aux(thread);
    const char *type_str = evhtp_kv_find(req->uri->query, "group-type");
    char *tag = NULL;
    int type = G_NONE;
    if (!type_str) {
        syslog(LOG_ERR, "group list request missing group-type\n");
        evhtp_send_reply(req, EVHTP_RES_ERROR);
    }
    if      (!strcmp(type_str, "albums")) {
        type = G_ALBUM; 
        tag = "agal";
    }
    else if (!strcmp(type_str, "artists")) {
        type = G_ARTIST;
        tag = "agar";
    }
    const char *param = evhtp_kv_find(req->uri->query, "meta");
    char *raw_meta;
    char **meta;
    int nmeta;
    vector tags;
    vector_new(&tags, 4);
    if (param && strcmp(param, "all")) {
        raw_meta = strdup(param);
        meta_parse(raw_meta, &meta, &nmeta); 
    } else {
        raw_meta = NULL;
        meta = (char **)default_meta_group;
    }
    for (int i = -1; meta[++i]; ) {
        int index = meta_find_group_tag(meta[i]);
        if (index >= 0)
            vector_pushback(&tags, group_meta_tags + index);
        else syslog(LOG_NOTICE, "unrecognized metatag: %s\n", meta[i]);
    }
    struct evbuffer *payload;
    payload = evbuffer_new();
    char *sqlstr = NULL;
    query_t q = { Q_GROUPLIST, S_NONE, L_NONE };
    sql_build_query(&q, &tags, NULL, &sqlstr); // no clauses
    size_t nitems;
    if (type == G_ALBUM) 
        nitems = count_precompiled_items(aux, Q_COUNT_ALL_ALBUMS, NULL);
    else
        nitems = count_precompiled_items(aux, Q_COUNT_ALL_ARTISTS, NULL);
    struct evbuffer **items = malloc(nitems * sizeof(struct evbuffer *));
    int ritems = sql_put_results(&payload, items, aux, &tags, 1, 
                                 sqlstr, &type, grouplist_itemcb);
    add_list(req->buffer_out, payload, tag, ritems, ritems);
    syslog(LOG_INFO, "found %i items.\n", ritems);
    syslog(LOG_INFO, "sending %lu bytes...\n", 
                     evbuffer_get_length(req->buffer_out));
    evhtp_send_reply(req, EVHTP_RES_OK);
// cleanup
    if (meta && meta != (char **)default_meta_group)
        free(meta);
    evbuffer_free(payload);
    free(items);
    vector_free(&tags);
    if (sqlstr) free(sqlstr);
    if (raw_meta) free(raw_meta);
}

void register_callbacks(evhtp_t *evhtp) {
    syslog(LOG_INFO, "registering callbacks...\n");

// regex callbacks must be registered in correct order: most specific first
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/browse/", 
                   server_xmlrpc, 
                   "database browse");
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/items/" REG_NUM "/extra_data/artwork", 
                   server_xmlrpc, 
                   "item artwork");
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/items/", 
                   res_stream_item, 
                   "item stream");
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/items", 
                   res_item_list, 
                   "item list");
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/containers/" REG_NUM "/items", 
                   res_container_items, 
                   "container content list");
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/containers", 
                   res_container_list, 
                   "containers list");
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/groups/" REG_NUM "/extra_data/artwork", 
                   server_xmlrpc, 
                   "group artwork");
evhtp_set_regex_cb(evhtp, 
                   DB_STR REG_NUM "/groups", 
                   res_group_list, 
                   "group list");
// static callbacks can be registered in any order
evhtp_set_cb(evhtp, "/login",         res_login,         "login"         );
evhtp_set_cb(evhtp, "/logout",        res_logout,        "logout"        );
evhtp_set_cb(evhtp, "/update",        res_update,        "update"        );
evhtp_set_cb(evhtp, "/databases",     res_database_list, "database list" );
evhtp_set_cb(evhtp, "/content-codes", res_content_codes, "content codes" );
evhtp_set_cb(evhtp, "/server-info",   res_server_info,   "server info"   );
evhtp_set_cb(evhtp, "/", server_xmlrpc, "unassigned");
}
