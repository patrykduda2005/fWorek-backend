#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "connection.h"
#include "cJSON.h"
#include "logging.h"
#include "send_ready.h"
#include "process_string.h"

struct data {
    char grupa[100];
    char przedmiot[100];
    char typ[100];
    char data[100];
    int count;
};

struct cookie {
    int error;
    char gr1[1024];
    char gr2[1024];
};


int getcookie(struct cookie* buffer) {
    FILE *fd = fopen("cookie", "r");
    if (fd == NULL) {
        errorlog("Plik 'cookie' nie istnieje");
        buffer->error = 1;
        //strcpy(buffer, "cokolwiek\0");
        return -1;
    }
    FILE *fd2 = fopen("cookie2", "r");
    if (fd2 == NULL) {
        fclose(fd);
        errorlog("Plik 'cookie2' nie istnieje");
        buffer->error = 1;
        //strcpy(buffer, "cokolwiek\0");
        return -1;
    }
    fgets(buffer->gr1, 1024, fd);
    fgets(buffer->gr2, 1024, fd2);
    char *new_line = strstr(buffer->gr1, "\n");
    if (new_line != NULL) memset(new_line, 0, 1);
    new_line = strstr(buffer->gr2, "\n");
    if (new_line != NULL) memset(new_line, 0, 1);
    fclose(fd);
    fclose(fd2);
    return 0;
}

void typtostring(char *buffer, int typ) {
    switch (typ) {
        case 1:
            strcpy(buffer, "sprawdzian");
            break;
        case 2:
            strcpy(buffer, "kartkowka");
            break;
        case 3:
            strcpy(buffer, "sprawdzian");
            break;
        case 4:
            strcpy(buffer, "zadanie");
            break;
        default:
            strcpy(buffer, "pytanie");
    }
}

void parsedate(char *date) {
    *strstr(date, "T") = '\0';
}

void getGrupa(char *buffer, char* przed, int grcookie) {
    if (strcmp(przed, "Język angielski") == 0
        || strcmp(przed, "Język rosyjski") == 0
        || strcmp(przed, "Wychowanie fizyczne") == 0
        || strcmp(przed, "Programowanie w języku Python") == 0
        || strcmp(przed, "Systemy zarządzające inteligentnym domem") == 0
    ) {
        if (grcookie == 1) 
            strcpy(buffer, "gr1");
        else 
            strcpy(buffer, "gr2");
    } else
        strcpy(buffer, "obie");
}

void renamePrzedmiot(char *buffer, char *olprzed) {
    if (strcmp(olprzed,  "Historia") == 0) {
        strcpy(buffer,  "historia");
    }
    else if (strcmp(olprzed, "Język angielski") == 0) {
        strcpy(buffer, "j_ang");
    }
    else if (strcmp(olprzed, "Język niemiecki") == 0) {
        strcpy(buffer, "j_niemiecki");
    }
    else if (strcmp(olprzed, "Język polski") == 0) {
    strcpy(buffer, "j_polski");
    }
    else if (strcmp(olprzed,  "Język rosyjski") == 0) {
    strcpy(buffer,  "j_rosyjski");
    }
    else if (strcmp(olprzed, "Matematyka") == 0) {
    strcpy(buffer, "matematyka");
    }
    else if (strcmp(olprzed, "Religia") == 0) {
    strcpy(buffer, "religia");
    }
    else if (strcmp(olprzed, "Wiedza o społeczeństwie") == 0) {
    strcpy(buffer, "wos");
    }
    else if (strcmp(olprzed,  "Wychowanie fizyczne") == 0) {
    strcpy(buffer,  "wf");
    }
    else if (strcmp(olprzed, "Programowanie w języku Python") == 0) {
        strcpy(buffer, "python");
    }
    else if (strcmp(olprzed, "Systemy zarządzające inteligentnym domem") == 0) {
        strcpy(buffer, "smart_dom");
    }
    else if (strcmp(olprzed, "J. polski") == 0) {
        strcpy(buffer, "j_polski");
    }
    else if (strcmp(olprzed, "Godzina z wychowawcą") == 0) {
        strcpy(buffer, "gzw");
    } else {
        errorlog("NOT IMPLEMENTED PRZEDMIOT: %s", olprzed);
    }
}


struct data* pullOutJSON(char* req_response, int grupa) {
    char* body = strstr(req_response, "\r\n\r\n");
    cJSON *json = cJSON_Parse(body);
    int count = cJSON_GetArraySize(json);
    struct data *dane = calloc(count, sizeof(struct data));
    dane[0].count = count;
    //printf("JSON: %s\n", cJSON_PrintUnformatted(json));
    cJSON *el = NULL;
    int i = 0;
    cJSON_ArrayForEach(el, json) {
        cJSON *przedmiot = cJSON_GetObjectItemCaseSensitive(el, "przedmiotNazwa");
        if (cJSON_IsString(przedmiot)) {
            char przedbuff[100] = "";
            char grupabuff[100];
            getGrupa(grupabuff, przedmiot->valuestring, grupa);
            if (grupa != 1 && strcmp(grupabuff, "obie") == 0) {
                dane[0].count--;
                continue;
            }
            renamePrzedmiot(przedbuff, przedmiot->valuestring);
            strcpy(dane[i].przedmiot, przedbuff);
            strcpy(dane[i].grupa, grupabuff);
        }

        cJSON *typ = cJSON_GetObjectItemCaseSensitive(el, "typ");
        if (cJSON_IsNumber(typ)) {
            char buff[100];
            typtostring(buff, typ->valueint);
            strcpy(dane[i].typ, buff);
        }


        cJSON *date = cJSON_GetObjectItemCaseSensitive(el, "data");
        if (cJSON_IsString(date)) {
            parsedate(date->valuestring);
            strcpy(dane[i].data, date->valuestring);
        }


        i++;
    }
    cJSON_Delete(json);
    free(req_response);
    return dane;
}

send_ready* putdatatosr(struct data* data) {
    int rowc = data[0].count;
    messlog("Wrapping..vulcan...");

    send_ready* sr = sr_init_json(rowc);

    for (int row = 0; row < rowc; row++) {
        sr_set_json_line(sr, data[row].grupa, data[row].przedmiot, data[row].typ, data[row].data, "Z VULCANA", row + 1, row != (rowc - 1));
    }

    messlog("Wrapping done");
    return sr;
}

struct url {
    char hostname[100];
    char path[100];
    char key[100];
    char dataOdDo[100];
};

void url_to_string(char* buffer, struct url url) {
    sprintf(buffer, "%s%s?key=%s&%s", url.hostname, url.path, url.key, url.dataOdDo);
}


send_ready* getdziennik(char* body) {
    messlog("Getting dziennik...");
    int zakresverification;
    if ((zakresverification = verifyDataOdDataDo(body)) != 1) {
        if (zakresverification == -1) {
            send_ready* sr = sr_init_error_json(500, "REGEX sie zepsul :(");
            return sr;
        }
        send_ready* sr = sr_init_error_json(422, "VULCAN zly format zakresu dat");
        return sr;
    }



    //path
    struct url gr1_url;
    strcpy(gr1_url.hostname, "uczen.eduvulcan.pl");
    strcpy(gr1_url.path, "/powiatlezajski/api/SprawdzianyZadaniaDomowe");
    strcpy(gr1_url.key, "TVRrMU5UVXRNak0wTnkweExUUT0");
    strncpy(gr1_url.dataOdDo, body, strstr(body, "H") - body - 1); //HTTP 1.1

    struct url gr2_url;
    memcpy(&gr2_url, &gr1_url, sizeof(struct url));
    strcpy(gr2_url.key, "TVRrMU5UTXRNak0wTnkweExUUT0");

    char gr_url_buffer[sizeof(struct url)];


    //cookies
    struct cookie cookie;
    strcpy(cookie.gr1, "");
    strcpy(cookie.gr2, "");
    if (getcookie(&cookie) == -1) {
        send_ready* sr = sr_init_error_json(500, "Plik z tokenem nie istnieje");
        return sr;
    }
    char header[2048] = "";


    url_to_string(gr_url_buffer, gr1_url);
    messlog("%s", gr_url_buffer);
    sprintf(header, "Host: uczen.eduvulcan.pl\r\nCookie: EfebSsoCookie=%s;\r\n", cookie.gr1);
    struct data *dane2 = pullOutJSON(secure_fetch(gr_url_buffer, header), 1);


    url_to_string(gr_url_buffer, gr2_url);
    memset(header, 0, sizeof(header));
    sprintf(header, "Host: uczen.eduvulcan.pl\r\nCookie: EfebSsoCookie=%s;\r\n", cookie.gr2);
    struct data *dane = pullOutJSON(secure_fetch(gr_url_buffer, header), 2);

    if (dane == NULL || dane2 == NULL) {
        free(dane);
        free(dane2);
        send_ready* sr = sr_init_error_json(503, "No permission for VULCAN");
        return sr;
    }
    send_ready* sr = putdatatosr(dane);
    send_ready* sr2 = putdatatosr(dane2);
    send_ready* joined_sr = sr_join_json(sr, sr2);


    free(dane);
    free(dane2);

    return joined_sr;
}
