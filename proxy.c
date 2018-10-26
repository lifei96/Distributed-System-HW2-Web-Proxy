#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

static const char *content_length_name = "Content-length: ";

void handle_client_request(int connfd);
void handle_server_response(int fd_server, int fd_client);
void parse_uri(char *uri, char *host, char *port, char *query);
void construct_request(char *request, char *method, char *query,
        char *version,char *host, char *user_agent, char *connection,
        char *proxy_connection, rio_t *rio);
void client_error(int fd, char *cause, char *errnum,
        char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
    int listenfd, fd_client;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    while (true) {
        clientlen = sizeof(clientaddr);
        fd_client = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname,
                MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        handle_client_request(fd_client);
        Close(fd_client);
    }
}

/*
 * handle_client_request - handles http request from client
 */
void handle_client_request(int fd_client) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    char host[MAXLINE], port[MAXLINE], query[MAXLINE];
    char request[MAXBUF];
    rio_t rio;
    int fd_server;

    Rio_readinitb(&rio, fd_client);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) {
        return;
    }
    printf("Received HTTP request %s", buf);
    sscanf(buf, "%s %s", method, uri);
    if (strcasecmp(method, "GET")) {
        /* Not a GET request */
        client_error(fd_client, method, "501", "Not Implemented",
                     "Web Proxy does not implement this method");
        return;
    }

    parse_uri(uri, host, port, query);

    construct_request(request, method, query, "HTTP/1.0",
            host, user_agent_hdr, "close", "close", &rio);

    fd_server = Open_clientfd(host, port);

    Rio_writen(fd_server, request, strlen(request));

    handle_server_response(fd_server, fd_client);

    printf("Success");
}

/*
 * handle_server_response - handles http response from server
 *
 */
void handle_server_response(int fd_server, int fd_client) {
    rio_t rio;
    char buf[MAXLINE];
    int content_len = 0;

    Rio_readinitb(&rio, fd_server);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) {
        return;
    }
    printf("%s", buf);
    Rio_writen(fd_client, buf, strlen(buf));
    while (strcmp(buf, "\r\n")) {
        if (!memcmp(buf, content_length_name, sizeof(content_length_name))) {
            char *endptr;
            content_len = strtoll(buf + strlen(content_length_name), &endptr, 10);
        }
        Rio_readlineb(&rio, buf, MAXLINE);
        printf("%s", buf);
        Rio_writen(fd_client, buf, strlen(buf));
    }

    if (content_len) {
        Rio_readnb(&rio, buf, content_len);
        printf("%s", buf);
        Rio_writen(fd_client, buf, strlen(buf));
    }

    Close(fd_server);
}

/*
 * parse_uri - parses an uri to host, port and query
 */
void parse_uri(char *uri, char *host, char *port, char *query) {

}

/*
 * construct_request - constructs an http request
 */
void construct_request(char *request, char *method, char *query,
        char *version,char *host, char *user_agent, char *connection,
        char *proxy_connection, rio_t *rio) {

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
