#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include "processhttprequests.h"
#include "mysqldata.h"

static void process_get_method(int connected_socket);
static void process_post_method(int connected_socket, char* http_request);
static void get_body(char* body, char* http_request);
static void assembly_response(char *mess, char* body);
static int determine_method(char* http_request);

enum {
    GET_METHOD = 1,
    POST_METHOD = 2
};
void receive_http_request(int sock) {
    int connected_socket = accept(sock, NULL, NULL);
    char rec[1000] = "";
    recv(connected_socket, rec, sizeof(rec), 0);
    printf("Request:\n%s\n---------", rec);

    int req_method = determine_method(rec);

    switch (req_method) {
        case GET_METHOD:
            process_get_method(connected_socket);
            break;
        case POST_METHOD:
            process_post_method(connected_socket, rec);
            break;
    
    }

    close(connected_socket);
}
void process_get_method(int connected_socket) {
    char mess[1000] = "";
    char body[1000] = "";
    getData(body);
    assembly_response(mess, "");
    send(connected_socket, &mess, strlen(mess), 0);
    send(connected_socket, &body, strlen(body), 0);
}

void process_post_method(int connected_socket, char* http_request) {
    char body[1000] = "";
    get_body(body, http_request);
    //printf("BODY:\n%s\n", body);
    char mess[1000] = "";
    assembly_response(mess, "");
    insertData(body);
    send(connected_socket, &mess, strlen(mess), 0);
}
void assembly_response(char *mess, char* body) {
    strcat(mess, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET\r\n\r\n");
    //char body[1000] = "";
    //getData(body);
    strcat(mess, body);
}
int determine_method(char* http_request) {
    //-1 nothing, 1 GET, 2 POST
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
