#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include "connection.h"
#include "logging.h"
#include "signalhandling.h"
#include "send_ready.h"
#include "process_string.h"


enum HTTP_METHOD {
    GET_METHOD,
    POST_METHOD
};

void receive_http_request(int sock);
int determine_method(char* http_request);
void body_sending(int connected_socket, send_ready_line* sr);

int setup_bind_and_listen_on_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in adress;
    adress.sin_family = AF_INET;
    adress.sin_port = htons(port);
    adress.sin_addr.s_addr = INADDR_ANY;

    int err = bind(sock, (struct sockaddr*) &adress, sizeof(adress));
    messlog("Binding returned: %d\n", err);
    if (err < 0) {
        continue_execution = 0;
        return -1;
    }
    listen(sock, 5);
    return sock;
}

void receive_http_request(int sock) {
    int connected_socket = accept(sock, NULL, NULL);
    char rec[1000] = "";
    recv(connected_socket, rec, sizeof(rec), 0);
    messlog("Request:\n%s\n---------", rec);

    int req_method = determine_method(rec);

    struct http_response hr;
    memset(hr.header, 0, sizeof(hr.header));
    switch (req_method) {
        case GET_METHOD:
            process_get_method(&hr);
            break;
        case POST_METHOD:
            process_post_method(rec, &hr);
            break;
    
    }
    if (req_method >= 0 ) {
        send(connected_socket, hr.header, strlen(hr.header), 0);
        body_sending(connected_socket, hr.body);
    }

    close(connected_socket);
}

void body_sending(int connected_socket, send_ready_line* sr) {
    for (int i = 0; i < 50; i++) {
        if (sr[i][0] == '\0') break;
        send(connected_socket, sr[i], strlen(sr[i]), 0);
    }
    free(sr);
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
