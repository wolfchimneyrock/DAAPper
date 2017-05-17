#ifndef __UTIL_H__
#define __UTIL_H__
#include "scratch.h"

void timestamp_rfc1123(char *buf) ;
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

#endif
