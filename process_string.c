#include <stdlib.h>
#include <string.h>
#include "process_string.h"
#include "dziennik_fetch.h"
#include "send_ready.h"
#include "mysqldata.h"

char* get_body(char* http_request) {
    for(; *http_request != '\0'; http_request++) {
        if (*http_request != '\r')
            continue;
        if(*(http_request+1) == '\n' && *(http_request+2) == '\r' && *(http_request+3) == '\n') {
            break;
        }
    }
    return http_request+4;
}

void get_header(char* header) {
    strcat(header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET, POST\r\n\r\n");
}

void process_get_method(struct http_response* hr) {
    send_ready_line* sr_mysql = getData();
    send_ready_line* vulc = getdziennik();
    send_ready_line* sr = realloc(sr_mysql, sizeof(sr_mysql) + sizeof(vulc));
    memcpy(sr + sizeof(sr_mysql), vulc, sizeof(vulc));
    free(sr_mysql);
    free(vulc);
    get_header(hr->header);
    hr->body = (send_ready_line*)sr;
}

void process_post_method(char* http_request, struct http_response* hr) {
    send_ready_line* sr = insertData(get_body(http_request));
    get_header(hr->header);
    hr->body = (send_ready_line*) sr;
}

int determine_method(char* http_request) {
    //-1 nothing, 0 GET, 1 POST
    char method[10];
    memset(method, 0, sizeof(method));
    int i = 0;
    for (; http_request[i] != ' ' && i < 10; i++);
    if (i == 10) return -1;
    strncpy(method, http_request, i);
    if (strcmp(method, "GET") == 0) return GET_METHOD;
    if (strcmp(method, "POST") == 0) return POST_METHOD;
    return 0;
}
