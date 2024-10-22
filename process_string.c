#include <stdlib.h>
#include <string.h>
#include "process_string.h"
#include "dziennik_fetch.h"
#include "logging.h"
#include "send_ready.h"
#include "mysqldata.h"

char* get_body(char* http_request) {
    for(; *http_request != '\0'; http_request++) {
        if (*http_request != '\r')
            continue;
        if(*(http_request+1) == '\n' && *(http_request+2) == '\r' && *(http_request+3) == '\n') {
            break;
        }
    }
    return http_request+4;
}

void get_header(char* header) {
    strcat(header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET, POST\r\n\r\n");
}

int verifyDataOdDataDo(char* body) {
//dataOd=2024-09-30T22:00:00.000Z&dataDo=2024-10-31T22:59:59.999Z
    int verification = 1;
    //verification = !strncmp(body, "dataOd=", 7);
    //body += 7;
    //body += 4;
    //verification = !strncmp(body, "-", 1);
    //body += 3;
    //verification = !strncmp(body, "-", 1);
    return verification;
}

enum {
    BOTHFAULT,
    VULCANFAULT,
    MYSQLFAULT,
    NOONEFAULT
};

void process_get_method(struct http_response* hr, char* http_request) {
    send_ready* sr;
    int fault = NOONEFAULT;
    send_ready* sr_mysql = getData();
    if (sr_mysql == NULL) {
        fault = MYSQLFAULT;
        sr_mysql = sr_init_json(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "{grupa: \"gr1\", przedmiot: \"j_ang\", typ: \"zadanie\", data: \"2024-10-10\", opis:\"MYSQL\"}", 1);
    }
//    if (!verifyDataOdDataDo(strstr(http_request, "\r\n\r\n") + 4)) {
//        sr = sr_init(1);
//        sr_set_http_code(sr, 422);
//        sr_set_line(sr, "DataOd DataDo wrong format", 0);
//        sr_free(sr_mysql);
//        goto set;
//    }
    send_ready* vulc = getdziennik();
    if (vulc == NULL) {
        if (fault == MYSQLFAULT)
            fault = BOTHFAULT;
        else
            fault = VULCANFAULT;
        vulc = sr_init_json(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "{grupa: \"gr1\", przedmiot: \"j_ang\", typ: \"zadanie\", data: \"2024-10-10\", opis:\"VULCAN\"}", 1);
    }
    // VULCAN
    // [{..}]
    // {grupa: fsdfa, }
    // r.text()
    // split(']')
    send_ready* join_sr;
    if (fault == BOTHFAULT) {
        join_sr = sr_init_json(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "{grupa: \"gr1\", przedmiot: \"j_ang\", typ: \"zadanie\", data: \"2024-10-10\", opis:\"MYSQL i VULCAN\"}", 1);
    } else if (fault != VULCANFAULT)
        join_sr = sr_join_json(sr_mysql, vulc);
    else 
        join_sr = sr_join_json(vulc, sr_mysql);

    if (join_sr == NULL) {
        sr = sr_init(1);
        sr_set_http_code(sr, 500);
        sr_set_line(sr, "Joining error", 0);
        sr_free(join_sr);
        goto set;
    }
    sr = join_sr;
set:
    messlog("FS:");
    sr_print(sr);
    get_header(hr->header);
    hr->body = (send_ready*)sr;
}

int verify_password(char *body) {
    messlog("verify: %s", body);
    if (strncmp(body, "YCWO", 4) == 0) {
        return 1;
    }
    return 0;
}

void process_post_method(char* http_request, struct http_response* hr) {
    send_ready* sr;
    if (!verify_password(strstr(http_request, "\r\n\r\n") + 4)) {
        sr = sr_init(1);
        sr_set_http_code(sr, 403);
    } else 
        sr = insertData(strstr(get_body(http_request), "\n") + 1);
    get_header(hr->header);
    hr->body = (send_ready*)sr;
}

int determine_method(char* http_request) {
    //-1 nothing, 0 GET, 1 POST
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
