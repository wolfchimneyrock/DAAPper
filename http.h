#ifndef __HTTP_H__
#define __HTTP_H__
#include <evhtp/evhtp.h>
#include <sqlite3.h>
#include "sql.h"

#define SERVER_IDENTIFIER "RJW/1.0"

void app_init_thread(evhtp_t *htp, evthr_t *thread, void *arg);
void precompile_statements(void *arg);
void app_term_thread(evhtp_t *htp, evthr_t *thread, void *arg);
void add_headers_out(evhtp_request_t *req);
void register_callbacks(evhtp_t *evhtp);
void *create_segment(int id, void *a);
#endif
