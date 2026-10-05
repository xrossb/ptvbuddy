#pragma once

#include <stddef.h>

#define mapget(m, k) (mapget_f((m), (k), sizeof(*(k))))
#define mapset(m, k, v) (mapset_f((m), (k), sizeof(*(k)), v))

typedef struct Map Map;

Map* mapcreate(void);
void mapfree(Map* m);

void* mapget_f(Map* m, void* k, size_t ksize);
void mapset_f(Map* m, void* k, size_t ksize, void* v);
