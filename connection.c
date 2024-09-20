#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include "connection.h"
#include "signalhandling.h"

int setup_bind_and_listen_on_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in adress;
    adress.sin_family = AF_INET;
    adress.sin_port = htons(port);
    adress.sin_addr.s_addr = INADDR_ANY;

    int err = bind(sock, (struct sockaddr*) &adress, sizeof(adress));
    printf("Binding returned: %d\n", err);
    if (err < 0) {
        continue_execution = 0;
        return -1;
    }
    listen(sock, 5);
    return sock;
}
