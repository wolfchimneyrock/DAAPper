#ifndef __SPSCQ_H__
#define __SPSCQ_H__

typedef struct _spscq SPSCQ;

SPSCQ *spscq_init    (size_t capacity);
int    spscq_isfull  (SPSCQ *s);
int    spscq_isempty (SPSCQ *s);
void  *spscq_pop     (SPSCQ *s);
int    spscq_push    (SPSCQ *s, void *data);
void   spscq_free    (SPSCQ *s);

#endif
