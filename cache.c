/*
 * cache.c - LFU and LRU - LFU and LRU cache for web proxy, implementation.
 */
/* $begin cache.c */
#include "cache.h"

/* LFU cache */
Node_t *LFU_head;
Node_t *LFU_tail;
volatile int LFU_len;
volatile size_t LFU_size;

/* LRU cache */
Node_t *LRU_head;
Node_t *LRU_tail;
volatile int LRU_len;
volatile size_t LRU_size;

/* semaphores */
volatile int read_count;
sem_t sem_r;  /* semaphore for read_count (NOT for cache read) */
sem_t sem_w;  /* semaphore for cache write */

/* functions */

/* initializes cache */
void init_cache() {
    LFU_head = NULL;
    LFU_tail = NULL;
    LFU_len = 0;
    LRU_size = 0;

    LRU_head = NULL;
    LRU_tail = NULL;
    LRU_len = 0;
    LRU_size = 0;

    read_count = 0;
    Sem_init(&sem_r, 0, 1);
    Sem_init(&sem_w, 0, 1);
}

/* checks whether the url in a given node is the same as a given url */
int isSame(Node_t *node, char *url) {
    return (strcasecmp(node->url, url) == 0) ? 1 : 0;
}

/* $end cache.c */
