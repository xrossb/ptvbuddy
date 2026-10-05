#include "map.h"
#include "array.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MAP_DEFAULT_BUCKETS 8

static size_t hash(void* k, size_t ksize) {
    return 1; // TODO: an actual hash function.
}

typedef struct Node {
    size_t hash;
    size_t ksize;
    void* k;
    void* v;
    struct Node* next;
} Node;

static Node* create_node(void* k, size_t ksize, size_t h, void* v, Node* next) {
    void* kcopy = malloc(ksize);
    memcpy(kcopy, k, ksize);

    Node* node = malloc(sizeof(Node));
    *node = (Node){
        .hash = h,
        .ksize = ksize,
        .k = kcopy,
        .v = v,
        .next = next,
    };

    return node;
}

static void free_node(Node* n) {
    if (n->k) {
        free(n->k);
    }

    if (n->next) {
        free_node(n->next);
    }

    free(n);
}

static bool node_eq(Node* n, void* k, size_t ksize, size_t h) {
    if (h != n->hash)
        return false;
    if (ksize != n->ksize)
        return false;
    return memcmp(k, n->k, ksize) == 0;
}

static Node* find_node(Node* n, void* k, size_t ksize, size_t h) {
    if (!n) {
        return NULL;
    }

    if (node_eq(n, k, ksize, h)) {
        return n;
    }

    return find_node(n->next, k, ksize, h);
}

typedef struct {
    Node* node;
} Bucket;

struct Map {
    Bucket* buckets;
};

Map* mapcreate(void) {
    Map* m = malloc(sizeof(Map));
    *m = (Map){
        .buckets = NULL,
    };

    arrgrow(m->buckets, MAP_DEFAULT_BUCKETS);
    memset(m->buckets, 0, MAP_DEFAULT_BUCKETS * sizeof(Bucket));

    return m;
}

void mapfree(Map* m) {
    for (size_t i = 0; i < arrlen(m->buckets); i++) {
        Bucket* bucket = &m->buckets[i];
        if (bucket->node) {
            free_node(bucket->node);
        }
    }
    arrfree(m->buckets);
    free(m);
}

void* mapget_f(Map* m, void* k, size_t ksize) {
    size_t h = hash(k, ksize);
    size_t i = h % arrlen(m->buckets);
    Bucket* bucket = &m->buckets[i];
    Node* node = find_node(bucket->node, k, ksize, h);

    if (!node) {
        return NULL;
    }

    return node->v;
}

void mapset_f(Map* m, void* k, size_t ksize, void* v) {
    // TODO: dynamically increase bucket count as map fills up.

    size_t h = hash(k, ksize);
    size_t i = h % arrlen(m->buckets);
    Bucket* bucket = &m->buckets[i];

    Node* existing = find_node(bucket->node, k, ksize, h);
    if (existing) {
        existing->v = v;
        return;
    }

    Node* next = bucket->node;
    bucket->node = create_node(k, ksize, h, v, next);
}
