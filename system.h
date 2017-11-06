#ifndef __DAAP_SYSTEM_H__
#define __DAAP_SYSTEM_H__

#include <signal.h>
#include <pthread.h>
#include "config.h"

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
