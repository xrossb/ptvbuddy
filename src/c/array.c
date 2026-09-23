#include "array.h"

void* arrgrow_f(void* a, size_t elemsize, size_t addlen) {
    size_t newlen = arrlen(a) + addlen;
    if (newlen <= arrcap(a)) {
        return a;
    }

    size_t newcap = arrcap(a) || 1;
    while (newcap < newlen) {
        newcap *= 2;
    }

    void* newa = realloc(a ? arrhead(a) : NULL, elemsize * newcap + sizeof(ArrayHeader));
    newa += sizeof(ArrayHeader);

    if (!a) {
        arrhead(newa)->len = 0;
    }
    arrhead(newa)->cap = newcap;

    return newa;
}
