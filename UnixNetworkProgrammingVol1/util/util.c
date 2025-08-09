#include "util.h"

// TOOD: ch20 图20.7,学习该函数使用
extern int pselect(int maxfdp1, fd_set *rset, fd_set* writeset, fd_set *exceptset,
                    const struct timespec *timeout, const sigset_t *sigmask);


int max(int a, int b)
{
    return a > b ? a : b;
}

void test(void)
{
    // POLLIN
    // INTTIM
}

void *Malloc(size_t size)
{
    void *p;

    if ((p = malloc(size)) == NULL)
        return NULL;
    
    return p;
}

void Free(void *ptr)
{
    free(ptr);
}

void *Calloc(size_t nmemb, size_t size)
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
        return NULL;
    
    return p;
}

void *Realloc(void *ptr, size_t size)
{
    void *p;

    if ((p = realloc(ptr, size)) == NULL)
        return NULL;
    
    return p;
}
