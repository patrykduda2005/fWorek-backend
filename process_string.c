#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "process_string.h"
#include "dziennik_fetch.h"
#include "logging.h"
#include "send_ready.h"
#include "mysqldata.h"

int verifyDataOdDataDo(char* body) {
//dataOd=2024-09-30T22:00:00.000Z&dataDo=2024-10-31T22:59:59.999Z
    char temp[100] = "";
    strncpy(temp, body, 100);
    char* temp_end = strstr(temp, " ");
    *temp_end = '\0';
    regex_t regex;
    int reti = regcomp(&regex, 
        "dataOd=[0-9]\\{4\\}-[0-9]\\{2\\}-[0-9]\\{2\\}T[0-9]\\{2\\}:[0-9]\\{2\\}:[0-9]\\{2\\}\\.[0-9]\\{3\\}Z"
       "&dataDo=[0-9]\\{4\\}-[0-9]\\{2\\}-[0-9]\\{2\\}T[0-9]\\{2\\}:[0-9]\\{2\\}:[0-9]\\{2\\}\\.[0-9]\\{3\\}Z", 0);
    if (reti) {
        errorlog("Regex sie nie skompilowal");
        regfree(&regex);
        return -1;
    }

    reti = regexec(&regex, temp, 0, NULL, 0);
    if (reti == REG_NOMATCH) {
        errorlog("Brak matchu: %s", temp);
        regfree(&regex);
        return 0;
    } else if (!reti) {
        regfree(&regex);
        return 1;
    } else {
        errorlog("Cos sie innego stalo z regexem: %s", temp);
        regfree(&regex);
        return 0;
    }
}

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


enum {
    BOTHFAULT,
    VULCANFAULT,
    MYSQLFAULT,
    NOONEFAULT
};

void process_get_method(struct http_response* hr, char* http_request) {
    send_ready* sr;
    int fault = NOONEFAULT;
    send_ready* sr_mysql = getData(http_request);
    if (sr_mysql == NULL) {
        fault = MYSQLFAULT;
        sr_mysql = sr_init_error_json(503, "MYSQL sie zepsul");
    }
    if (sr_get_http_code(sr_mysql) != 200)
        fault = MYSQLFAULT;

    send_ready* vulc = getdziennik(strstr(http_request, "/") + 2);
    if (vulc == NULL) {
        if (fault == NOONEFAULT)
            fault = VULCANFAULT;
        else if (fault != VULCANFAULT)
            fault = BOTHFAULT;
        vulc = sr_init_error_json(503, "VULCAN sie zepsul");
    }
    if (sr_get_http_code(vulc) != 200) {
        if (fault == NOONEFAULT)
            fault = VULCANFAULT;
        else if (fault != VULCANFAULT)
            fault = BOTHFAULT;
    }
    switch (fault) {
        case BOTHFAULT: errorlog("BOTHFAULT"); break;
        case VULCANFAULT: errorlog("VULCANFAULT"); break;
        case MYSQLFAULT: errorlog("MYSQLFAULT"); break;
        case NOONEFAULT: messlog("NOONEFAULT"); break;
    }
    send_ready* join_sr;
    if (fault == BOTHFAULT) {
        join_sr = sr_init_error_json(503, "MYSQL i VULCAN");
    } else if (fault == MYSQLFAULT || fault == NOONEFAULT)
        join_sr = sr_join_json(sr_mysql, vulc);
    else 
        join_sr = sr_join_json(vulc, sr_mysql);

    if (join_sr == NULL) {
        sr = sr_init_error_json(500, "Joining error");
        sr_free(join_sr);
    } else
        sr = join_sr;

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
    return -1;
}
