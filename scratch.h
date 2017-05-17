#ifndef __SCRATCH_H__
#define __SCRATCH_H__

typedef struct _scratch SCRATCH;
typedef enum scratch_keep {
	SCRATCH_FREE = 0,
	SCRATCH_KEEP
} scratch_keep;


SCRATCH *scratch_new     (size_t capacity);
void    *scratch_head    (SCRATCH *s);
void    *scratch_detach  (SCRATCH *s);
void    *scratch_get     (SCRATCH *s, size_t size);
void     scratch_free    (SCRATCH *s, scratch_keep keep);
void     scratch_reset   (SCRATCH *s);

#endif
