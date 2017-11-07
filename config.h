#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef struct config_t {
	int   port;
	int   threads;
	int   fullscan;
    int   timeout;
    int   buffercap;
    int   verbose;
    int   sequential;
    int   cachestripes;
	char *name;
	char *root;
	char *dbfile;
	char *server_name;
	char *library_name;
	char *userid;
} config_t;

extern config_t config;

void get_config(config_t *config, char *cfg_file);
#endif
