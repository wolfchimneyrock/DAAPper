#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include "confuse.h"
#include "config.h"

config_t config;

void get_config(config_t *config, char *cfg_file) {
	cfg_opt_t opts[] = {
		CFG_SIMPLE_STR("server_name",  &(config->server_name)),
		CFG_SIMPLE_STR("library_name", &(config->library_name)),
		CFG_SIMPLE_STR("uid",          &(config->userid)),
		CFG_SIMPLE_INT("port",         &(config->port)),
		CFG_SIMPLE_INT("threads",      &(config->threads)),
		CFG_SIMPLE_INT("timeout",      &(config->timeout)),
		CFG_SIMPLE_STR("name",         &(config->name)),
		CFG_SIMPLE_STR("root",         &(config->root)),
		CFG_SIMPLE_STR("dbfile",       &(config->dbfile)),
		CFG_SIMPLE_STR("extfile",      &(config->extfile)),
		CFG_SIMPLE_BOOL("fullscan",    &(config->fullscan)),
		CFG_END()
	};
	cfg_t *cfg;

	char *cfg_path = cfg_file ? cfg_file : "/etc/daapper.conf";

	setlocale(LC_MESSAGES, "");
	setlocale(LC_CTYPE, "");

	cfg = cfg_init(opts, 0);
	cfg_parse(cfg, cfg_path);
}


