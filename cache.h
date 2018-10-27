/*
 * cache.h - LFU and LRU cache for web proxy, definition and prototypes.
 */
/* $begin cache.h */
#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_LRU_LEN 1000
#define MAX_LFU_LEN 3

/* double linked list node */
typedef struct Node {
    struct Node *prev;
    struct Node *next;
    char *url;
    char *response;
    size_t size;
    int count;
} Node_t;

/* LFU cache */
extern Node_t *LFU_head;
extern Node_t *LFU_tail;
extern volatile int LFU_len;
extern volatile size_t LFU_size;

/* LRU cache */
extern Node_t *LRU_head;
extern Node_t *LRU_tail;
extern volatile int LRU_len;
extern volatile size_t LRU_size;

/* semaphores */
extern volatile int read_count;
extern sem_t sem_r;  /* semaphore for read_count (NOT for cache read) */
extern sem_t sem_w;  /* semaphore for cache write */

/* function prototypes */

/* creates a node with given url and response */
Node_t *create_node(char *url, char *response);

/* frees node cur */
void free_node(Node_t *cur);

/* checks whether the url in a given node is the same as a given url */
int isSame(Node_t *node, char *url);

/* initializes cache */
void init_cache();

/* inserts node cur after node pos */
void insert(Node_t *cur, Node_t *pos);

/* removes node cur, returns the node before it */
Node_t *remove(Node_t *cur);

/* moves node cur to the position after node pos */
void move(Node_t *cur, Node_t *pos);

/* gets the cached response with the given url, returns NULL if it not exists */
char *get(char *url);

/* puts (url, response) into the cache */
Node_t *put(char *url, char *response);

#endif /* __CACHE_H__ */
/* $end cache.h */
