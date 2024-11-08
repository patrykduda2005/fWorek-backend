#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include "connection.h"
#include "logging.h"
#include "signalhandling.h"
#include "send_ready.h"
#include "process_string.h"

char* get_up_to(char* beginning, char* upto) {
    char *temp_null_char_pointer;
    if ((temp_null_char_pointer = strstr(beginning, upto)) == NULL) {
        return NULL;
    }
    *temp_null_char_pointer = '\0';
    return beginning;
}

void requesting(SSL* ssl, char* path, char* headers) {
    // Create the GET request
    char request[4048];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Connection: close\r\n"
             "%s\r\n",
             //"Cookie: EfebSsoCookie=%s;\r\n\r\n",
             path, headers);


    messlog("REQUEST:\n %s", request);

    // Send the GET request over SSL
    SSL_write(ssl, request, strlen(request));
}

char* reading(SSL* ssl) {
    int string_length = 4096;
    char* string = calloc(1, sizeof(char));
    char buffer[4096];
    int bytes_received;
    while ((bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        string = realloc(string, string_length + 1);
        buffer[bytes_received] = '\0';  // Null-terminate the buffer
        //printf("%s\n", buffer);
        strcat(string, buffer);
        string_length += 4096;
    }

    char *body = strstr(string, "\r\n\r\n");
    if (body == NULL || body[4] != '[') {
        free(string);
        errorlog("No permissions for VULCAN (wrong cookie probably): %s", body);
        return NULL;
    }

    return string;
}

char* secure_fetch(char* url, char* headers) {
    //fetch("uczen.eduvulcan.pl/powiatlezajski/api/SprawdzianyZadaniaDomowe?key=TVRrMU5UVXRNak0wTnkweExUUT0&dataOd=2024-09-30T22:00:00.000Z&dataDo=2024-10-31T22:59:59.999Z",
    //    /*headers*/"Connection: close\r\nHost: uczen.eduvulcan.pl\r\nCookie: EfebSsoCookie=%s;\r\n");
    char url_copy[strlen(url) + 1];
    strcpy(url_copy, url);
    char *hostname;
    if ((hostname = get_up_to(url_copy, "/")) == NULL) {
        errorlog("Url wrong format: %s", url);
        char error[] = "422: url wrong format";
        char *merror = malloc(sizeof(error));
        strcpy(merror, error);
        return merror;
    }
    messlog("free???");
    char *path = url + strlen(hostname);

    struct SSL_connection* ssl = establish_secure_connection(hostname);
    if (ssl == NULL) {
        char error[] = "500: Cannot establish connection with VULCAN";
        char *merror = malloc(sizeof(error));
        strcpy(merror, error);
        return merror;
    }
    requesting(ssl->ssl, path, headers);
    char *response = reading(ssl->ssl);

    ending_connection(ssl);
    return response;
}

struct SSL_connection* establish_secure_connection(char* hostname) {
    messlog("Establishing connection with %s", hostname);
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    int port = 443;  // HTTPS port 443
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        errorlog("Unable to create SSL context");
        return NULL;
    }

    // Create a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        errorlog("Socket creation failed");
        return NULL;
    }

    // Resolve the hostname to an IP address
    server = gethostbyname(hostname);
    if (!server) {
        errorlog("No such host");
        return NULL;
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        errorlog("Connection failed");
        return NULL;
    }

    // Create an SSL connection over the socket
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0) {
        errorlog("SSL CONNECTION FAILED");
        return NULL;
    }
    struct SSL_connection* ssl_conn = malloc(sizeof(struct SSL_connection));
    ssl_conn->sockfd = sockfd;
    ssl_conn->ctx = ctx;
    ssl_conn->ssl = ssl;
    return ssl_conn;
}

void ending_connection(struct SSL_connection* ssl) {
    SSL_shutdown(ssl->ssl);
    SSL_free(ssl->ssl);
    close(ssl->sockfd);
    SSL_CTX_free(ssl->ctx);
    free(ssl);
    EVP_cleanup();
}

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
            process_get_method(&hr, rec);
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


