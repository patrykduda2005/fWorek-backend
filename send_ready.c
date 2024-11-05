#include "send_ready.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef char send_ready_line[LINESIZE];

struct send_ready{
    send_ready_line* srl;
    int lines_count;
    int response_code;
};

send_ready* sr_init_json(int lines) {
    int brackets_amount = 2;
    struct send_ready* sr = malloc(sizeof(struct send_ready));

    sr->lines_count = lines + brackets_amount;
    sr->srl = (send_ready_line*)calloc(
        sr->lines_count,
        sizeof(char) * LINESIZE
    );
    sr->response_code = 200;
    strcpy(sr->srl[0], "[");
    strcpy(sr->srl[lines + 1], "]\0");
    return sr;
}

send_ready* sr_init(int lines) {
    if (lines <= 0)
        errorlog("TOO LITTLE LINES");
    struct send_ready* sr = malloc(sizeof(struct send_ready));

    sr->lines_count = lines;
    sr->srl = (send_ready_line*)calloc(
        sr->lines_count,
        sizeof(char) * LINESIZE
    );
    sr->response_code = 200;
    return sr;
}

void sr_set_http_code(send_ready* sr, int code) {
    if (code < 100 || code >= 600) {
        errorlog("Wrong http code: %d", code);
        return;
    }
    ((struct send_ready*)sr)->response_code = code;
}
int sr_get_http_code(send_ready* sr) {
    if (sr != NULL)
        return ((struct send_ready*)sr)->response_code;
    else return -1;
}

int sr_set_line(send_ready* sr, char* line, int index) {
    struct send_ready* real_sr = (struct send_ready*) sr;

    if (strlen(line) > LINESIZE) {
        errorlog("Line too lengthy for 'send_ready'");
        return -1;
    }

    if (index >= real_sr->lines_count || index < 0) {
        errorlog("Cannot change that index of 'send_ready', %d", real_sr->lines_count);
        return -1;
    }

    if (index == 0 && real_sr->srl[0][0] == '[') {
        errorlog("Cannot change that index of 'send_ready'");
        return -1;
    }

    strcpy(real_sr->srl[index], line);
    return 0;
}

int sr_set_json_line(send_ready* sr, char* grupa, char* przedmiot, char* typ, char* data, char* opis, int index, int iscomma) {
    struct send_ready* real_sr = sr;

    if (index >= real_sr->lines_count || index < 0) {
        errorlog("Cannot change that index of 'send_ready', %d", real_sr->lines_count);
        return -1;
    }

    if (index == 0 && real_sr->srl[0][0] == '[') {
        errorlog("Cannot change that index of 'send_ready'");
        return -1;
    }

    char temp[LINESIZE] = "";
    sprintf(temp, "{\"grupa\": \"%s\", \"przedmiot\": \"%s\", \"typ\": \"%s\", \"data\":\"%s\", \"opis\": \"%s\"}",
            grupa, przedmiot, typ, data, opis);


    if (iscomma) {
        if (strlen(temp) + 1 > LINESIZE) {
            errorlog("json_line is too lengthy: %s", temp);
            return -1;
        }
        strcat(temp, ",");
    }

    return sr_set_line(sr, temp, index); 
}

void sr_print(send_ready* sr) {
    if (sr == NULL) {
        errorlog("Cannot print 'send_ready' which is NULL");
        return;
    }
    struct send_ready* real_sr = (struct send_ready*) sr;
    for (int i = 0; i < real_sr->lines_count; i++) {
        printf("LINE: %s\n", real_sr->srl[i]);
    }
}

void sr_free(send_ready* sr) {
    free(((struct send_ready*)sr)->srl);
    free((struct send_ready*)sr);
}

send_ready* sr_init_error_json(int errorcode, char* errordesc) {
    int templatesize = 92;
    send_ready* sr = sr_init_json(1);
    if (templatesize + strlen(errordesc) > LINESIZE) {
        errorlog("Error (%s) is to long. Max length is %d", errordesc, LINESIZE - templatesize);
        sr_set_http_code(sr, 500);
        sr_set_line(sr, "{ \"typ\": \"error\" \"opis\": \"Error description too long\"}", 1);
        return sr;
    }
    sr_set_http_code(sr, errorcode);
    char errorline[LINESIZE] = "";
    sprintf(errorline, "{\"typ\": \"error\", \"opis\": \"%s\"}", errordesc);
    sr_set_line(sr, errorline, 1);
    return sr;
}


void code_to_statusText(char *buffer, int code) {
    switch (code) {
        case 200:
            strcpy(buffer, "OK");
            break;
        case 403:
            strcpy(buffer, "Forbidden");
            break;
        case 422:
            strcpy(buffer, "Unprocessable Content");
            break;
        case 500:
            strcpy(buffer, "Internal Server Error");
            break;
        case 503:
            strcpy(buffer, "Service Unavailable");
            break;
        default:
            errorlog("STATUSTEXT NOT IMPLEMENTED: %d", code);
    }
}

void sr_sending(send_ready* sr, int connected_socket) {
    struct send_ready* real_sr = (struct send_ready*) sr;
    if (real_sr != NULL) {
        char statusText[50] = "";
        code_to_statusText(statusText, real_sr->response_code);
        char header[500] = "";
        sprintf(header, "HTTP/1.1 %d %s\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET, POST\r\n\r\n", real_sr->response_code, statusText);
        send(connected_socket, header, strlen(header), 0);
        for (int i = 0; i < real_sr->lines_count; i++) {
            send(connected_socket, real_sr->srl[i], strlen(real_sr->srl[i]), 0);
        }
    }
    sr_free(sr);
}

send_ready* sr_join_json(send_ready* lhs, send_ready* rhs) {
    if (lhs == NULL || rhs == NULL) {
        errorlog("send_ready is NULL");
        return NULL;
    }

    struct send_ready* real_lhs = (struct send_ready*) lhs;
    int lhs_lines = real_lhs->lines_count;
    struct send_ready* real_rhs = (struct send_ready*) rhs;

    if (real_lhs->srl[0][0] != '[' || real_rhs->srl[0][0] != '[') {
        sr_free(real_rhs);
        errorlog("Cannot join not-json in sr_join_json");
        return NULL;
    }

    struct send_ready* sr = realloc(real_lhs, sizeof(struct send_ready));
    sr->srl = realloc(sr->srl,
                      (sr->lines_count + real_rhs->lines_count - 2) * LINESIZE * sizeof(char));
    sr->lines_count += real_rhs->lines_count - 2;
    char temp[LINESIZE] = "";
    if (sr->srl[lhs_lines - 2][0] != '[' && real_rhs->lines_count > 2) {
        sprintf(temp, "%s,", sr->srl[lhs_lines - 2]);
        strcpy(sr->srl[lhs_lines - 2], temp);
    }
    for (int i = 1; i < real_rhs->lines_count; i++) {
        sr_set_line(sr, real_rhs->srl[i], lhs_lines - 2 + i);
    }
    sr_free(real_rhs);
    return sr;
}
