#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <evhtp/evhtp.h>
#include "system.h"
#include "util.h"
#include "scratch.h"

static const char *DAY_NAMES[] =
  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *MONTH_NAMES[] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

void timestamp_rfc1123(char *buf) {
    time_t now;
    struct tm ts;
    time(&now);
    gmtime_r(&now, &ts);
    strftime(buf, 30, "---, %d --- %Y %H:%M:%S GMT", &ts);
    memcpy(buf, DAY_NAMES[ts.tm_wday], 3);
    memcpy(buf+8, MONTH_NAMES[ts.tm_mon], 3);
}

void log_request(evhtp_request_t *req, void *a) {
    char ipaddr[16];
    app *aux = (app *)evthr_get_aux(req->conn->thread);
    printable_ipaddress((struct sockaddr_in *)req->conn->saddr, ipaddr);
    const char *ua  = evhtp_kv_find(req->headers_in, "User-Agent");
    LOGGER(LOG_INFO, "%s [%s]{%d} requested %s: %s%s", 
            ipaddr, ua, aux->thread_id, (char *)a, req->uri->path->path,
            req->uri->path->file);
    evhtp_kv_t *kv;
}


int count_char(char *buf, char ch) {
    if (buf == NULL) return 0;
    int i = 0, res = 0;
    while (buf[i]) if (buf[i++] == ch) ++res;
    return res;
}

int uri_get_number(const char *uri, const int index) {
    int pos = 0;
    int i = 0;
    if (!uri || uri[0] == '\0') return -1;
    int len = strlen(uri);
    while (pos < len && i < index) {
        while (pos < len && (uri[pos] < '0' || uri[pos] > '9')) pos++;
        // now we're at first number
        while (pos < len && i < index) {
            while(pos < len && (uri[pos] >= '0'&& uri[pos] <='9')) pos++;
            while(pos < len && (uri[pos] < '0' || uri[pos] > '9')) pos++;
            i++;
        }
    }
    return (pos < len && i == index) ? atoi(uri + pos) : -1;
}
    
void printable_ipaddress(struct sockaddr_in *addr, char *buf) {
    snprintf(buf, 16, "%d.%d.%d.%d", 
            (int)((addr->sin_addr.s_addr & 0xFF      )      ),
            (int)((addr->sin_addr.s_addr & 0xFF00    ) >>  8),
            (int)((addr->sin_addr.s_addr & 0xFF0000  ) >> 16),
            (int)((addr->sin_addr.s_addr & 0xFF000000) >> 24));
}

int split_path(const char *path, SCRATCH *s) {
    if (s == NULL || path == NULL) return -1;
    if (path[0] == '\0') return 0;
    int count = 0;
    int len;
    // first, count '/' and get strlen too with where i lands
    // len includes the trailing '\0'
    for (len = 0; path[len] != '\0'; len++)
        if (path[len] == '/') count++;

    // initialize an extra one to null terminate the list
    char **v = scratch_get(s, (1+count)*sizeof(void *));
    v[0]     = scratch_get(s, len);
    strncpy(v[0], path, len);
    int pos = len - 1;
    int current = count;
    while (pos > 0 && current > 1) {
        if (v[0][pos] == '/') {
            v[0][pos] = '\0';
            v[--current] = v[0] + pos + 1;
        } else pos--;
    }
    return count;
}

char *split_filename(char *path) {
    if (path == NULL || path[0] == '\0') return NULL;
    
    int pos = 0;
    char *last = NULL;
    while (path[pos] != '\0') {
        if (path[pos] == '/')
            last = path + pos;
        pos++;
    }
    if (last != NULL)
        *last++ = '\0';
    return last;
}
        
