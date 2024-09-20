#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#include "signalhandling.h"
#include "connection.h"
#include "mysqldata.h"

void assembly_response(char *mess, char* body);
void process_get_method(int sock);
void get_body(char* body, char* http_request);
int determine_method(char* http_request);
void process_post_method(int connected_socket, char* http_request);
void receive_http_request(int sock);


#define CONNECT 1



void assembly_response(char *mess, char* body) {
    strcat(mess, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET\r\n\r\n");
    //char body[1000] = "";
    //getData(body);
    strcat(mess, body);
}

enum {
    GET_METHOD = 1,
    POST_METHOD = 2
};

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

void process_post_method(int connected_socket, char* http_request) {
    char body[1000] = "";
    get_body(body, http_request);
    //printf("BODY:\n%s\n", body);
    char mess[1000] = "";
    assembly_response(mess, "");
    insertData(body);
    send(connected_socket, &mess, strlen(mess), 0);
}


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


int main() {
    init_signal();

    if (CONNECT) {
        int sock = setup_bind_and_listen_on_socket(21339);
        while(continue_execution) {
            receive_http_request(sock);
        }
        close(sock);
        return 0;
    }

    char mess[1000] = "POST / HTTP/1.1 Host: frog01.mikr.us:21339\r\n Connection: keep-alive\r\n Content-Length: 68\r\n Cache-Control: max-age=0\r\n Upgrade-Insecure-Requests: 1\r\n Origin: http://localhost:3000\r\n Content-Type: application/x-www-form-urlencoded\r\n User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36\r\n Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n Referer: http://localhost:3000/\r\n Accept-Encoding: gzip, deflate\r\n Accept-Language: pl-PL,pl;q=0.9,en-US;q=0.8,en;q=0.7\r\n\r\ngrupa=obie&przedmiot=j_ang&typ=zadanie&data=2024-09-10&opis=andrzej duda lubi w dupe";

    char body[1000] = "";
    get_body(body, mess);
    insertData(body);
    //printf("Body:\n%s\n", body);

    return 0;
}
