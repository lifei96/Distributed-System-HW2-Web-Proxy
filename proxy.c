/*
 * proxy.c - a web proxy
 */
/* $begin proxy.c */
#include "csapp.h"
#include "cache.h"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

static const char *user_agent_name = "User-Agent: ";
static const char *host_name = "Host: ";
static const char *connection_name = "Connection: ";
static const char *proxy_connection_name = "Proxy-Connection: ";

void *handle_client_request(void *arg);
int handle_server_response(int fd_server, int fd_client, char *response);
void parse_uri(char *uri, char *host, char *port, char *query);
void construct_request(char *request, const char *method, const char *query,
        const char *version, const char *user_agent, const char *host,
        const char *connection, const char *proxy_connection, rio_t *rio);
void client_error(int fd, char *cause, char *errnum,
        char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(EPIPE, SIG_IGN);
    int listenfd, fd_client;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    init_cache();

    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen = sizeof(clientaddr);
        fd_client = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname,
                MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        pthread_t tid;
        Pthread_create(&tid, NULL, handle_client_request, (void *)&fd_client);
        Pthread_detach(tid);
    }
}

/*
 * handle_client_request - handles http request from client
 */
void *handle_client_request(void *arg) {
    int fd_client = *((int *)arg);
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    char host[MAXLINE], port[MAXLINE], query[MAXLINE];
    char request[MAXBUF], response[MAX_OBJECT_SIZE];
    rio_t rio;
    int fd_server;
    int response_size = 0;

    Rio_readinitb(&rio, fd_client);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) {
        return NULL;
    }
    printf("Received HTTP request %s", buf);
    sscanf(buf, "%s %s", method, uri);
    if (strcasecmp(method, "GET")) {
        /* Not a GET request */
        client_error(fd_client, method, "501", "Not Implemented",
                     "Web Proxy does not implement this method");
        return NULL;
    }

    response_size = get_cache(uri, response);

    if (response_size) {
        /* uri in cache */
        Rio_writen(fd_client, response, response_size);

        Close(fd_client);

        access_node(uri);
    } else {
        /* uri not in cache */
        parse_uri(uri, host, port, query);

        construct_request(request, method, query, "HTTP/1.0",
                          user_agent_hdr, host, "close", "close", &rio);

        fd_server = Open_clientfd(host, port);

        printf("Sending request to server:\n%s\n", request);

        Rio_writen(fd_server, request, strlen(request));

        response_size = handle_server_response(fd_server, fd_client, response);

        Close(fd_client);

        if (response_size < MAX_OBJECT_SIZE) {
            put_cache(uri, response, response_size);
            access_node(uri);
        }
    }

    printf("Success");

    return NULL;
}

/*
 * handle_server_response - handles http response from server, returns response size
 */
int handle_server_response(int fd_server, int fd_client, char *response) {
    printf("\n\nhandling server response\n\n");
    rio_t rio;
    char buf[MAXBUF];
    int cur_size = 0, total_size = 0;

    Rio_readinitb(&rio, fd_server);

    while ((cur_size = Rio_readnb(&rio, buf, MAXBUF))) {
        printf("%s", buf);
        Rio_writen(fd_client, buf, cur_size);
        if (total_size + cur_size < MAX_OBJECT_SIZE) {
            memcpy(response + total_size, buf, cur_size);
        }
        total_size += cur_size;
    }

    Close(fd_server);

    return total_size;
}

/*
 * parse_uri - parses an uri to host, port and query
 */
void parse_uri(char *uri, char *host, char *port, char *query) {
    char *pos_host, *pos_port, *pos_query;

    pos_host = strstr(uri, "://");
    if (!pos_host) {
        pos_host = uri;
    } else {
        pos_host += 3;
    }

    pos_port = index(pos_host, ':');
    if (pos_port) {
        pos_port++;
    }

    pos_query = index(pos_host, '/');

    if (!pos_query) {
        strcpy(query, "/");
    } else {
        strcpy(query, pos_query);
    }

    if (!pos_port) {
        strcpy(port, "80");
    } else if (!pos_query) {
        strcpy(port, pos_port);
    } else {
        strncpy(port, pos_port, pos_query - pos_port);
    }

    if (pos_port) {
        strncpy(host, pos_host, pos_port - pos_host - 1);
    } else if (pos_query) {
        strncpy(host, pos_host, pos_query - pos_host);
    } else {
        strcpy(host, pos_host);
    }

    printf("host: %s\n", host);
    printf("port: %s\n", port);
    printf("query: %s\n", query);
    printf("parse_uri success\n");
}

/*
 * construct_request - constructs an http request
 */
void construct_request(char *request, const char *method, const char *query,
        const char *version, const char *user_agent, const char *host,
        const char *connection, const char *proxy_connection, rio_t *rio) {
    sprintf(request, "%s %s %s\r\n", method, query, version);
    sprintf(request, "%sUser-Agent: %s", request, user_agent);
    sprintf(request, "%sHost: %s\r\n", request, host);
    sprintf(request, "%sConnection: %s\r\n", request, connection);
    sprintf(request, "%sProxy-Connection: %s\r\n", request, proxy_connection);

    char buf[MAXLINE];

    if (!Rio_readlineb(rio, buf, MAXLINE)) {
        sprintf(request, "%s\r\n", request);
        return;
    }
    while (strcmp(buf, "\r\n")) {
        if (!memcmp(buf, user_agent_name, strlen(user_agent_name)) ||
            !memcmp(buf, host_name, strlen(host_name)) ||
            !memcmp(buf, connection_name, strlen(connection_name)) ||
            !memcmp(buf, proxy_connection_name, strlen(proxy_connection_name))) {
            Rio_readlineb(rio, buf, MAXLINE);
            continue;
        }
        printf("%s", buf);
        sprintf(request, "%s%s", request, buf);
        Rio_readlineb(rio, buf, MAXLINE);
    }
    sprintf(request, "%s\r\n", request);
}

/*
 * client_error - returns an error message to the client.
 */
void client_error(int fd, char *cause, char *errnum,
        char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Web Proxy</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

/* $end proxy.c */
