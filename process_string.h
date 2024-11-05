#ifndef PROCESS_STRING_H
#define PROCESS_STRING_H
#include "send_ready.h"
struct http_response {
    char header[1000];
    send_ready* body;
};
enum HTTP_METHOD {
    GET_METHOD,
    POST_METHOD
};

int verifyDataOdDataDo(char* body);
void process_get_method(struct http_response* hr, char* http_request);
void process_post_method(char* http_request, struct http_response* hr);
int determine_method(char* http_request);

#endif
