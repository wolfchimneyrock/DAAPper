#ifndef __SESSION_H__
#define __SESSION_H__

typedef enum status_t {
	ST_DEAD  = 0,
	ST_CLEAN = 1,
	ST_DIRTY = 2
} status_t;

void     sessions_init();
int      sessions_add();
void     sessions_touch(size_t s);
void     sessions_kill (size_t s);
status_t session_get_status(size_t s);  
void     sessions_cleanup();
#endif
