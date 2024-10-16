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
        sr_sending(hr.body, connected_socket);
    }

    close(connected_socket);
}


