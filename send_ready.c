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
};

send_ready* sr_init_json(int lines) {
    int brackets_amount = 2;
    struct send_ready* sr = malloc(sizeof(struct send_ready));

    sr->lines_count = lines + brackets_amount;
    sr->srl = (send_ready_line*)calloc(
        sr->lines_count,
        sizeof(char) * LINESIZE
    );
    strcpy(sr->srl[0], "[");
    strcpy(sr->srl[lines + 1], "]\0");
    return sr;
}

send_ready* sr_init(int lines) {
    struct send_ready* sr = malloc(sizeof(struct send_ready));

    sr->lines_count = lines;
    sr->srl = (send_ready_line*)calloc(
        sr->lines_count,
        sizeof(char) * LINESIZE
    );
    return sr;
}

void sr_set_line(send_ready* sr, char* line, int index) {
    struct send_ready* real_sr = (struct send_ready*) sr;

    if (strlen(line) > LINESIZE) {
        errorlog("Line too lengthy for 'send_ready'");
        return;
    }

    if (index >= real_sr->lines_count || index < 0) {
        errorlog("Cannot change that index of 'send_ready'");
        return;
    }

    if (index == 0 && real_sr->srl[0][0] == '[') {
        errorlog("Cannot change that index of 'send_ready'");
        return;
    }

    strcpy(real_sr->srl[index], line);
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

void sr_sending(send_ready* sr, int connected_socket) {
    struct send_ready* real_sr = (struct send_ready*) sr;
    if (real_sr != NULL) {
        for (int i = 0; i < real_sr->lines_count; i++) {
            //messlog("LINE: %s", real_sr->srl[i]);
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
    sprintf(sr->srl[lhs_lines - 2], "%s,", sr->srl[lhs_lines - 2]);
    for (int i = 1; i < real_rhs->lines_count; i++) {
        sr_set_line(sr, real_rhs->srl[i], lhs_lines - 2 + i);
    }
    sr_free(real_rhs);
    return sr;
}
