#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
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


void getcookie(char *buffer) {
    FILE *fd = fopen("cookie", "r");
    fgets(buffer, 1024, fd);
    char *new_line = strstr(buffer, "\n");
    if (new_line != NULL) memset(new_line, 0, 1);
    fclose(fd);
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
    if (strcmp(przed, "Język angielski") == 0) {
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
    else if (strcmp(olprzed,  "J. rosyjski") == 0) {
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


struct data* reading(SSL* ssl) {
    int string_length = 4096;
    char* string = calloc(1, sizeof(char));
    char buffer[4096];
    int bytes_received;
    while ((bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1)) > 0) {
        messlog("fsda");
        string = realloc(string, string_length + 1);
        buffer[bytes_received] = '\0';  // Null-terminate the buffer
        //printf("%s\n", buffer);
        strcat(string, buffer);
        string_length += 4096;
    }


    char *body = strstr(string, "\r\n\r\n");
    if (body[4] != '[') {
        free(string);
        errorlog("No permissions for VULCAN (wrong cookie probably)");
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
        cJSON *typ = cJSON_GetObjectItemCaseSensitive(el, "typ");
        if (cJSON_IsNumber(typ)) {
            char buff[100];
            typtostring(buff, typ->valueint);
            strcpy(dane[i].typ, buff);
        }

        cJSON *przedmiot = cJSON_GetObjectItemCaseSensitive(el, "przedmiotNazwa");
        if (cJSON_IsString(przedmiot)) {
            char przedbuff[100] = "";
            char grupabuff[100];
            getGrupa(grupabuff, przedmiot->valuestring, 1);
            renamePrzedmiot(przedbuff, przedmiot->valuestring);
            strcpy(dane[i].przedmiot, przedbuff);
            strcpy(dane[i].grupa, grupabuff);
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


send_ready* getdziennik() {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    const char *hostname = "uczen.eduvulcan.pl";
    const char *path = "/powiatlezajski/api/a5371f1b-db92-48a5-88e8-604c716c36a3?key=TVRrMU5UVXRNak0wTnkweExUUT0";
    /*
     * 
     * dataOd=2024-09-30T22:00:00.000Z&dataDo=2024-10-31T22:59:59.999Z
     * 
    */
    const char *czas = "&dataOd=2024-09-30T22:00:00.000Z&dataDo=2024-10-31T22:59:59.999Z";
    int port = 443;  // HTTPS port 443
    char request[1024];

    // Initialize OpenSSL
    //SSL_load_error_strings();
    //OpenSSL_add_ssl_algorithms();

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        perror("Unable to create SSL context");
        return NULL;
    }

    // Create a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // Resolve the hostname to an IP address
    server = gethostbyname(hostname);
    if (!server) {
        fprintf(stderr, "No such host\n");
        return NULL;
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return NULL;
    }

    // Create an SSL connection over the socket
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) <= 0) {
        return NULL;
    }

    char cookie[1024] = "";
    getcookie(cookie);

    // Create the GET request
    snprintf(request, sizeof(request),
             "GET %s%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "Cookie: EfebSsoCookie=%s;\r\n\r\n",
             path, czas, hostname, cookie);

    //messlog("REQUEST:\n %s", request);

    // Send the GET request over SSL
    SSL_write(ssl, request, strlen(request));



    // Receive the response and print it
    struct data *dane = reading(ssl);
    if (dane == NULL) {
        free(dane);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(sockfd);
        SSL_CTX_free(ctx);
        EVP_cleanup();
        return NULL;
    }
    send_ready* sr = putdatatosr(dane);
    free(dane);


    // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    EVP_cleanup();

    return sr;
}


