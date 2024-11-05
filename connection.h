#ifndef CONNECTION_H
#define CONNECTION_H
#include <openssl/ssl.h>
#include <openssl/err.h>

struct SSL_connection {
    SSL* ssl;
    int sockfd;
    SSL_CTX* ctx;
};

struct SSL_connection* establish_secure_connection(char* hostname);
void ending_connection(struct SSL_connection* ssl);
int setup_bind_and_listen_on_socket(int port);
void receive_http_request(int sock);

#endif
