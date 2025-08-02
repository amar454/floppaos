#ifndef SLAB_H
#define SLAB_H

#include <stddef.h>

void slab_init(void);
void *slab_alloc(size_t size);
void slab_free(void *ptr, size_t size);
#endif // SLAB_H
