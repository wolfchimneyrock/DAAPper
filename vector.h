#ifndef __VECTOR_H__
#define __VECTOR_H__

#define PTR_TO_INT(p) ((int)(long)(p))
#define INT_TO_PTR(i) ((void *)(long)(i))

typedef struct vector {     ///< A dynamic resizing character array
    void       **data;      ///< heap allocated buffer
    long         used,      ///< current amount of buffer used
                 capacity;  ///< current capacity of buffer
} vector;

int   vector_new(vector *d, long initial);
int   vector_pushback(vector *d, const void *x);
void  vector_popback(vector *d);
void *vector_peekback(vector *d);
void  vector_free(vector *d) ;
int vector_isempty(vector *d) ;
#endif
