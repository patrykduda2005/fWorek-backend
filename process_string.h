#ifndef PROCESS_STRING_H
#define PROCESS_STRING_H
#include "send_ready.h"
struct http_response {
    char header[1000];
    send_ready_line* body;
};
enum HTTP_METHOD {
    GET_METHOD,
    POST_METHOD
};

void process_get_method(struct http_response* hr);
void process_post_method(char* http_request, struct http_response* hr);
int determine_method(char* http_request);

#endif
