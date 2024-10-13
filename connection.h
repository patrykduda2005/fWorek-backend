#ifndef CONNECTION_H
#define CONNECTION_H

int setup_bind_and_listen_on_socket(int port);
void receive_http_request(int sock);

#endif
