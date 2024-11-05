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


struct data* reading(SSL* ssl, int grupa) {
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
    free(string);
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

    struct SSL_connection* ssl = establish_secure_connection("uczen.eduvulcan.pl");
    if (ssl == NULL) {
        send_ready* sr = sr_init_error_json(500, "Cannot establish connection with VULCAN");
        return sr;
    }

    //path
    char path[400] = "/powiatlezajski/api/SprawdzianyZadaniaDomowe?";
    char key[100] = "TVRrMU5UVXRNak0wTnkweExUUT0";
    char key2[100] ="TVRrMU5UTXRNak0wTnkweExUUT0";
    char dataOdDo[200] = "";
    strncpy(dataOdDo, body, strstr(body, "H") - body - 1);

    strcat(path, dataOdDo);
    strcat(path, "&");
    strcat(path, "key=");
    char *keyinpath = path + strlen(path);

    //cookies
    struct cookie cookie;
    strcpy(cookie.gr1, "");
    strcpy(cookie.gr2, "");
    if (getcookie(&cookie) == -1) {
        send_ready* sr = sr_init_error_json(500, "Plik z tokenem nie istnieje");
        return sr;
    }
    char header[2048] = "";


    strcpy(keyinpath, key2);
    sprintf(header, "Host: uczen.eduvulcan.pl\r\nCookie: EfebSsoCookie=%s;\r\n", cookie.gr2);
    requesting(ssl->ssl, path, header);
    struct data *dane2 = reading(ssl->ssl, 2);

    ending_connection(ssl);
    ssl = establish_secure_connection("uczen.eduvulcan.pl");
    if (ssl == NULL) {
        send_ready* sr = sr_init_error_json(500, "Cannot establish connection with VULCAN");
        return sr;
    }

    strcpy(keyinpath, key);
    memset(header, 0, sizeof(header));
    sprintf(header, "Host: uczen.eduvulcan.pl\r\nCookie: EfebSsoCookie=%s;\r\n", cookie.gr1);
    requesting(ssl->ssl, path, header);
    struct data *dane = reading(ssl->ssl, 1);

    if (dane == NULL || dane2 == NULL) {
        free(dane);
        free(dane2);
        ending_connection(ssl);
        send_ready* sr = sr_init_error_json(503, "No permission for VULCAN");
        return sr;
    }
    send_ready* sr = putdatatosr(dane);
    send_ready* sr2 = putdatatosr(dane2);
    send_ready* joined_sr = sr_join_json(sr, sr2);


    free(dane);
    free(dane2);
    ending_connection(ssl);

    return sr;
}
