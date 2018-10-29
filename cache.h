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
    char uri[MAXLINE];
    char response[MAX_OBJECT_SIZE];
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

/* creates a node with given uri and response */
Node_t *create_node(char *uri, char *response, int response_size);

/* checks whether the uri in a given node is the same as a given uri */
int cmp(Node_t *node, char *uri);

/* initializes cache */
void init_cache();

/* inserts node cur after node pos */
void insert_node(Node_t *cur, Node_t *pos);

/* removes node cur, returns the node before it */
Node_t *remove_node(Node_t *cur);

/* moves node cur to the position after node pos */
void move_node(Node_t *cur, Node_t *pos);

/* finds a node with the given uri from head */
Node_t *find_node(char *uri, Node_t *head);

/* updates cache after accessing a uri */
void access_node(char *uri);

/* gets the cached response with the given uri if it exists */
void get_cache(char *uri, char *response);

/* puts (uri, response) into the cache */
Node_t *put_cache(char *uri, char *response, int response_size);

#endif /* __CACHE_H__ */
/* $end cache.h */
