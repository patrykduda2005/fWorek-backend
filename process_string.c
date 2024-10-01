#include <string.h>
#include "process_string.h"
#include "send_ready.h"
#include "mysqldata.h"

void get_body(char* body, char* http_request) {
    for(; *http_request != '\0'; http_request++) {
        if (*http_request != '\r')
            continue;
        if(*(http_request+1) == '\n' && *(http_request+2) == '\r' && *(http_request+3) == '\n') {
            break;
        }
    }
    strcpy(body, http_request+4);
}
void assembly_response(char *mess, char* body) {
    strcat(mess, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET, POST\r\n\r\n");
    //char body[1000] = "";
    //getData(body);
    strcat(mess, body);
}

void process_get_method(struct http_response* hr) {
    char body[1000] = "";
    send_ready_line* sr = getData(body);
    assembly_response(hr->header, "");
    hr->body = (send_ready_line*)sr;
}

void process_post_method(char* http_request, struct http_response* hr) {
    char body[1000] = "";
    get_body(body, http_request);
    send_ready_line* sr = insertData(body);
    assembly_response(hr->header, "");
    hr->body = (send_ready_line*) sr;
}

