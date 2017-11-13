#ifndef __DAAP_SYSTEM_H__
#define __DAAP_SYSTEM_H__

#include <signal.h>
#include <pthread.h>
#include <syslog.h>
#include "config.h"

extern int flag_daemonize;

#define LOGGER(log, format, ...) { \
    if (flag_daemonize)                \
        syslog(log, format, ##__VA_ARGS__);     \
    else                               \
        fprintf(stderr, format "\n", ##__VA_ARGS__); \
}

extern volatile pthread_t 
	main_pid,
	signal_pid,
	watcher_pid,
	scanner_pid,
	writer_pid;
int  set_affinityrange(pthread_t pid, int firstcpu, int lastcpu);
void staylocal(config_t *conf, char **argv);
void daemonize(config_t *conf, char **argv);
void assign_signal_handler();
void cleanup();

#endif
