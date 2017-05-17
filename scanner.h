#ifndef __SCANNER_H__
#define __SCANNER_H__

extern volatile sig_atomic_t scanner_active;

int    scanner_submit_request(char *path);
void   wait_for_scanner();
void  *scanner_thread(void *arg);

#endif
