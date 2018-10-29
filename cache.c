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

/* creates a node with given uri and response */
Node_t *create_node(char *uri, char *response, int response_size) {
    Node_t *node = (Node_t *)Malloc(sizeof(Node_t));
    if (uri != NULL) {
        strcpy(node->uri, uri);
    }
    if (response != NULL && response_size) {
        memcpy(node->response, response, response_size);
        node->size = response_size;
    } else {
        node->size = 0;
    }
    node->count = 0;
    return node;
}

/* checks whether the uri in a given node is the same as a given uri */
int cmp(Node_t *node, char *uri) {
    if (node->uri == NULL || uri == NULL) {
        /* dummy node is not comparable */
        printf("Error: dummy node is not comparable.");
        return 0;
    }
    return (strcasecmp(node->uri, uri) == 0) ? 1 : 0;
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
void insert_node(Node_t *cur, Node_t *pos) {
    if (cur == NULL || pos == NULL || pos->next == NULL) {
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
Node_t *remove_node(Node_t *cur) {
    if (cur == NULL || cur->prev == NULL || cur->next == NULL) {
        /* dummy node cannot be removed */
        printf("Error: dummy node cannot be removed.");
        return NULL;
    }
    Node_t *tmp = cur->prev;
    tmp->next = cur->next;
    tmp->next->prev = tmp;
    Free(cur);
    return tmp;
}

/* moves node cur to the position after node pos */
void move_node(Node_t *cur, Node_t *pos) {
    if (cur == NULL || pos == NULL || cur->prev == NULL || cur->next == NULL) {
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
    insert_node(cur, pos);
}

/* finds a node with the given uri from head */
Node_t *find_node(char *uri, Node_t *head) {
    Node_t *cur = head->next;
    while (cur->next != NULL) {
        if (cmp(cur, uri)) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

/* updates cache after accessing a uri */
void access_node(char *uri) {
    P(&sem_w);

    Node_t *tmp = find_node(uri, LFU_head);

    if (tmp) {
        /* uri in LFU */
        tmp->count++;
        while (tmp->prev != LFU_head && tmp->count > tmp->prev->count) {
            move_node(tmp, tmp->prev->prev);
        }
    } else {
        /* uri not in LFU */
        tmp = find_node(uri, LRU_head);
        if (tmp) {
            /* uri in LRU */
            tmp->count++;
            if (LFU_len < MAX_LFU_LEN || tmp->count > LFU_tail->prev->count) {
                move_node(tmp, LFU_tail->prev);
                LRU_size -= tmp->size;
                LRU_len--;
                LFU_size += tmp->size;
                LFU_len++;
                while (tmp->prev != LFU_head && tmp->count > tmp->prev->count) {
                    move_node(tmp, tmp->prev->prev);
                }
                while (LFU_len > MAX_LFU_LEN) {
                    LFU_size -= LFU_tail->prev->size;
                    remove_node(LFU_tail->prev);
                    LFU_len--;
                }
            } else {
                move_node(tmp, LRU_head);
            }
        }
    }
    V(&sem_w);
}

/* gets the cached response with the given uri if it exists */
void get_cache(char *uri, char *response) {
    P(&sem_r);
    read_count++;
    if (read_count == 1) {
        P(&sem_w);
    }
    V(&sem_r);

    Node_t *tmp = find_node(uri, LFU_head);
    if (tmp == NULL) {
        /* uri not in LFU */
        tmp = find_node(uri, LRU_head);
    }

    if (tmp) {
        strcpy(response, tmp->response);
    }

    P(&sem_r);
    read_count--;
    if (read_count == 0) {
        V(&sem_w);
    }
    V(&sem_r);
}

/* puts (uri, response) into the cache */
Node_t *put_cache(char *uri, char *response, int response_size) {
    if (strlen(response) > MAX_OBJECT_SIZE) {
        return NULL;
    }

    P(&sem_w);

    Node_t *tmp = find_node(uri, LFU_head);
    if (tmp == NULL) {
        /* uri not in LFU */
        tmp = find_node(uri, LRU_head);
    }

    if (tmp == NULL) {
        tmp = create_node(uri, response, response_size);
        insert_node(tmp, LRU_head);
        LRU_len++;
        LRU_size += tmp->size;
        while (LRU_len > MAX_LRU_LEN || LRU_size + LFU_size > MAX_CACHE_SIZE) {
            LRU_size -= LRU_tail->prev->size;
            remove_node(LRU_tail->prev);
            LRU_len--;
        }
    }

    V(&sem_w);

    return tmp;
}


/* $end cache.c */
