#pragma once

#include <stddef.h>
#include <stdlib.h>

#define arrhead(a) ((ArrayHeader*)(a) - 1)
#define arrfree(a) ((a) ? free(arrhead(a)) : NULL, (a) = NULL)

#define arrlen(a) ((a) ? arrhead(a)->len : 0)
#define arrcap(a) ((a) ? arrhead(a)->cap : 0)
#define arrpush(a, v) (arrgrow((a), 1), (a)[arrhead(a)->len++] = (v))
#define arrpop(a) (arrhead(a)->len -= 1, (a)[arrhead(a)->len])
#define arrgrow(a, n) ((a) = arrgrow_f((a), sizeof(*(a)), (n)))

typedef struct {
    size_t len, cap;
} ArrayHeader;

void* arrgrow_f(void* a, size_t elemsize, size_t addlen);
