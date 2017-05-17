#ifndef __WATCHER_H__
#define __WATCHER_H__
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "ringbuffer.h"
#include "sql.h"

extern volatile sig_atomic_t watcher_active;
extern volatile sig_atomic_t library_updated;
extern volatile sig_atomic_t watcher_fd;

void wait_for_watcher  ();
void *watcher_thread   (void *arg); 

typedef enum filetype_t {
    FT_OTHER,
    FT_AUDIO,
    FT_VIDEO,
    FT_PLAYLIST
} filetype_t; 

#endif
