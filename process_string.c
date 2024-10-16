#include <stdlib.h>
#include <string.h>
#include "process_string.h"
#include "dziennik_fetch.h"
#include "logging.h"
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
    send_ready* sr;
    send_ready* sr_mysql = getData();
    if (sr_mysql == NULL) {
        sr = sr_init(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "MYSQL", 0);
        goto set;
    }
    send_ready* vulc = getdziennik();
    if (vulc == NULL) {
        sr = sr_init(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "VULCAN", 0);
        goto set;
    }
    send_ready* join_sr = sr_join_json(sr_mysql, vulc);
    if (join_sr == NULL) {
        sr = sr_init(1);
        sr_set_http_code(sr, 500);
        sr_set_line(sr, "Joining error", 0);
        sr_free(join_sr);
        goto set;
    }
    sr = join_sr;
set:
    messlog("FS:");
    sr_print(sr);
    get_header(hr->header);
    hr->body = (send_ready*)sr;
}

int verify_password(char *body) {
    messlog("verify: %s", body);
    if (strncmp(body, "YCWO", 4) == 0) {
        return 1;
    }
    return 0;
}

void process_post_method(char* http_request, struct http_response* hr) {
    send_ready* sr;
    if (!verify_password(strstr(http_request, "\r\n\r\n") + 4)) {
        sr = sr_init(1);
        sr_set_http_code(sr, 403);
    } else 
        sr = insertData(strstr(get_body(http_request), "\n") + 1);
    get_header(hr->header);
    hr->body = (send_ready*)sr;
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
