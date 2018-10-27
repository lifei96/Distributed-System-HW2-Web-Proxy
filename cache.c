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

/* creates a node with given url and response */
Node_t *create_node(char *url, char *response) {
    Node_t *node = (Node_t *)Malloc(sizeof(Node_t));
    node->url = url;
    node->response = response;
    if (response) {
        node->size = strlen(node->response);
    } else {
        node->size = 0;
    }
    node->count = 1;
    return node;
}

/* frees node cur */
void free_node(Node_t *cur) {
    Free(cur->url);
    Free(cur->response);
    Free(cur);
}

/* checks whether the url in a given node is the same as a given url */
int isSame(Node_t *node, char *url) {
    if (node->url == NULL || url == NULL) {
        /* dummy node is not comparable */
        printf("Error: dummy node is not comparable.")
        return 0;
    }
    return (strcasecmp(node->url, url) == 0) ? 1 : 0;
}

/* initializes cache */
void init_cache() {
    LFU_head = create_node(NULL, NULL);  /* dummy node */
    LFU_tail = create_node(NULL, NULL);  /* dummy node */
    LFU_head->next = LFU_tail;
    LFU_tail->prev = LFU_head;
    LFU_len = 0;
    LRU_size = 0;

    LRU_head = create_node(NULL, NULL);  /* dummy node */
    LRU_tail = create_node(NULL, NULL);  /* dummy node */
    LRU_head->next = LRU_tail;
    LRU_tail->prev = LRU_head;
    LRU_len = 0;
    LRU_size = 0;

    read_count = 0;
    Sem_init(&sem_r, 0, 1);
    Sem_init(&sem_w, 0, 1);
}

/* inserts node cur after node pos */
void insert(Node_t *cur, Node_t *pos) {
    if (pos->next == NULL) {
        /* node cur should not be inserted after tail (dummy node) */
        printf("Error: node cur should not be inserted after tail (dummy node).");
        return;
    }
    cur->prev = pos;
    cur->next = pos->next;
    pos->next = cur;
    cur->next->prev = cur;
}

/* removes node cur, returns the node before it */
Node_t *remove(Node_t *cur) {
    if (cur->prev == NULL || cur->next == NULL) {
        /* dummy node cannot be removed */
        printf("Error: dummy node cannot be removed.");
        return NULL;
    }
    Node *tmp = cur->prev;
    tmp->next = cur->next;
    tmp->next->prev = tmp;
    free_node(cur);
    return tmp;
}

/* moves node cur to the position after node pos */
void move(Node_t *cur, Node_t *pos) {
    if (cur->prev == NULL || cur->next == NULL) {
        /* dummy node cannot be moved */
        printf("Error: dummy node cannot be moved.");
        return;
    }
    if (pos->next == NULL) {
        /* node cur should not be moved after tail (dummy node) */
        printf("node cur should not be moved after tail (dummy node).");
        return;
    }
    cur->prev->next = cur->next;
    cur->next->prev = cur->prev;
    insert(cur, pos);
}

/* gets the cached response with the given url, returns NULL if it not exists */
char *get(char *url) {

}

/* puts (url, response) into the cache */
Node_t *put(char *url, char *response) {

}


/* $end cache.c */
