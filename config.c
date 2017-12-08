#define _GNU_SOURCE
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include "confuse.h"
#include "config.h"

config_t conf;

#define DEFAULT_INT(x, v) { \
    if (x == -1) x = v;     \
}

#define DEFAULT_STR(x, v) { \
    if (!x) x = strdup(v);  \
}

void get_config(config_t *config, char *cfg_file) {
	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR("server_name",  &(config->server_name)),
		CFG_SIMPLE_STR("library_name", &(config->library_name)),
		CFG_SIMPLE_STR("uid",          &(config->userid)),
		CFG_SIMPLE_INT("port",         &(config->port)),
		CFG_SIMPLE_INT("threads",      &(config->threads)),
		CFG_SIMPLE_INT("timeout",      &(config->timeout)),
	CFG_SIMPLE_INT("buffer-capacity", &(config->buffercap)),
        CFG_SIMPLE_BOOL("sequential",  &(config->sequential)),
        CFG_SIMPLE_INT("stripes",      &(config->cachestripes)),
        CFG_SIMPLE_INT("chunk-size",   &(config->chunksize)),
		CFG_SIMPLE_STR("name",         &(config->name)),
		CFG_SIMPLE_STR("root",         &(config->root)),
		CFG_SIMPLE_STR("dbfile",       &(config->dbfile)),
		CFG_SIMPLE_BOOL("fullscan",    &(config->fullscan)),
		CFG_END()
	};
	cfg_t *cfg;
    

    // use sane defaults if not already set by cmdline parameters

    DEFAULT_INT(config->port,       3689);
    DEFAULT_INT(config->threads,    8);
    DEFAULT_INT(config->timeout,    1800);
    DEFAULT_INT(config->fullscan,   0);
    DEFAULT_INT(config->buffercap,  256);
    DEFAULT_INT(config->sequential, 0);
    DEFAULT_INT(config->cachestripes, 0);
    DEFAULT_INT(config->chunksize,     256*1024);
    DEFAULT_INT(config->chunkpreload,  config->chunksize * 4);
    DEFAULT_INT(config->chunkdelay,    config->chunksize / 8192);
    DEFAULT_INT(config->verbose,    0);

        // DAAPPER_DBFILE
    DEFAULT_STR(config->dbfile, "file:songs?mode=memory&cache=shared");
        // DAAPPER_ROOT
    DEFAULT_STR(config->root, "/srv/music");
        // DAAPPER_USER
    DEFAULT_STR(config->userid, "nobody");
        // LIBRARYNAME
    DEFAULT_STR(config->library_name, "Library");

    DEFAULT_STR(config->lock_style, "lock");

    char *cfg_path = cfg_file ? cfg_file : "/etc/daapper.conf";
    if (access(cfg_path, F_OK) != -1) {	
        fprintf(stderr, "Using config file '%s'\n", cfg_path);
        setlocale(LC_MESSAGES, "");
	setlocale(LC_CTYPE, "");
	cfg = cfg_init(opts, 0);
	cfg_parse(cfg, cfg_path);
    } else {
        fprintf(stderr, "Using default configuration.\n");
    }
	
}


