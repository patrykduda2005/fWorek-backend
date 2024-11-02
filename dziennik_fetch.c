#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <regex.h>
#include "cJSON.h"
#include "logging.h"
#include "send_ready.h"

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
    strtok(date, "T");
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

static const char *field_names[5] = {
    "grupa",
    "przedmiot",
    "typ",
    "data",
    "opis"
};
send_ready* putdatatosr(struct data* data) {
    int rowc = data[0].count;
    //int field_lenght = strlen(field_names[0]) + strlen(field_names[1]) + strlen(field_names[2]) 
    //      + strlen(field_names[3]) + strlen(field_names[4]);
    //5 fields
    messlog("Wrapping..vulcan...");

    send_ready* sr = sr_init_json(rowc);

    for (int row = 0; row < rowc; row++) {
        char send_ready_row[LINESIZE] = "";
        int send_ready_row_index = 0;
        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "{");
        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "\"%s\": \"%s\",", field_names[0], data[row].grupa);
        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "\"%s\": \"%s\",", field_names[1], data[row].przedmiot);
        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "\"%s\": \"%s\",", field_names[2], data[row].typ);
        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "\"%s\": \"%s\",", field_names[3], data[row].data);
        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "\"%s\": \"%s\"", field_names[4], "Z VULCANA");
        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "}");
        //for (int i = 0; i < 5; i++) {
        //    send_ready_row += sprintf(send_ready_row, "\"%s\": \"%s\"", field_names[i], data[row][i]);
        //    if (i != 4)
        //        send_ready_row += sprintf(send_ready_row, ",");
        //}
        //strcat(send_ready_row, "}"); send_ready_row_index++;
        if (row != rowc-1)
            send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, ",");
        sr_set_line(sr, send_ready_row, row + 1);
    }

    messlog("Wrapping done");
    return sr;
}

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

struct SSL_connection {
    SSL* ssl;
    int sockfd;
    SSL_CTX* ctx;
};

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

void ending_connection(struct SSL_connection* ssl) {
    SSL_shutdown(ssl->ssl);
    SSL_free(ssl->ssl);
    close(ssl->sockfd);
    SSL_CTX_free(ssl->ctx);
    free(ssl);
    EVP_cleanup();
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
