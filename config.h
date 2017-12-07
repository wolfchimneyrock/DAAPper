#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <confuse.h>

typedef struct config_t {
    long   port;
    long   threads;
    long   timeout;
    long   buffercap;
    cfg_bool_t   fullscan;
    cfg_bool_t   verbose;
    cfg_bool_t   sequential;
    long   cachestripes;
    long   chunksize;
    long   chunkpreload;
    long   chunkdelay;
    char *name;
    char *root;
    char *dbfile;
    char *server_name;
    char *library_name;
    char *userid;
    char *lock_style;
} config_t;

extern config_t conf;

void get_config(config_t *config, char *cfg_file);
#endif
