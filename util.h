#ifndef __UTIL_H__
#define __UTIL_H__
#include "scratch.h"
#include <evhtp/evhtp.h>
#include <sqlite3.h>

typedef enum fsw_event_flag fsw_event_flag;
#include <libfswatch/c/libfswatch.h>
#include "config.h"


typedef struct app_parent {
    evhtp_t  *htp;
    evbase_t *base;
    config_t *config;
} app_parent;

typedef struct app {
    FSW_HANDLE    fd;    
    int           header;
    int           thread_id;
    app_parent   *parent;
    evbase_t     *base;
    sqlite3      *db;
    config_t     *config;
    sqlite3_stmt **stmts; 
} app;

void timestamp_rfc1123(char *buf) ;
void log_request(evhtp_request_t *req, void *a);
int count_char(char *buf, char ch);
int uri_get_number(const char *uri, const int index); 
void printable_ipaddress(struct sockaddr_in *addr, char *buf) ;
int split_path(const char *path, SCRATCH *s);
char *split_filename(char *path);
// this can't be a subroutine because the date_now has to exist in the same scope
// as the evhtp_send_reply() called later
#define ADD_TIMESTAMP_HEADER(req)  \
    char date_now[30];             \
    timestamp_rfc1123(date_now);   \
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Date", date_now, 0, 0));

#define ADD_DATE_HEADER(txt) \
    char date_now[30]; timestamp_rfc1123(date_now);\
    evhtp_headers_add_header(req->headers_out, \
    evhtp_header_new(txt, date_now, 0, 0));

#if defined(__x86_64__) || defined(__i386) || defined(__amd_64__)
#define _spin_pause() __asm__ __volatile__ ("pause")
#elif defined(__arm__)
#define _spin_pause() __asm__ __volatile__ ("yield")
#endif

#endif
