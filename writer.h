#ifndef __WRITER_H__
#define __WRITER_H__
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "ringbuffer.h"
#include "sql.h"
extern volatile sig_atomic_t writer_active;
extern sem_t db_ready;
void db_inc_playcount    (const int song); 
void db_remove_file      (const int pathid); 
int  db_upsert_path      (app *aux, const char *path, const int parent); 
int  db_upsert_artist    (app *aux, const char *artist, 
                          const char *artist_sort); 
int  db_upsert_publisher (app *aux, const char *publisher);
int  db_upsert_genre     (app *aux, const char *genre);
int  db_upsert_album     (app *aux, const char *album, 
                          const char *album_sort, const int artist, 
                          const int publisher, const int year, 
                          const int total_tracks, const int total_discs);
int  db_upsert_song      (app *aux, const char *title, const int path, 
                          const int artist, const int album, 
                          const int genre, const int track, const int disc, 
                          const int song_length);
void wait_for_writer     ();
void *writer_thread      (void *arg);
time_t db_last_update_time();
void   db_init_status();
#endif
