#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "mysqldata.h"
#include "mysqlcredentials.h"
#include "logging.h"
#include "send_ready.h"

const char *field_names[5] = {
    "grupa",
    "przedmiot",
    "typ",
    "data",
    "opis"
};

send_ready* wrap_in_json(char ***data, int rowc) {
    //5 fields
    messlog("Wrapping..");

    send_ready* sr = sr_init_json(rowc);

    for (int row = 0; row < rowc; row++) {
        char send_ready_row[LINESIZE] = "";
        int send_ready_row_index = 0;

        send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "{");
        for (int i = 0; i < 5; i++) {
            send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, "\"%s\": \"%s\"", field_names[i], data[row][i]);
            if (i != 4)
                send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, ",");
        }
        strcat(send_ready_row, "}"); send_ready_row_index++;
        if (row != rowc-1)
            send_ready_row_index += sprintf(send_ready_row + send_ready_row_index, ",");
        sr_set_line(sr, send_ready_row, row + 1);
    }

    messlog("Wrapping done");
    return sr;
}

int verifyDataOdDataDoo(char* get_data) {
//dataOd=2024-09-30T22:00:00.000Z&dataDo=2024-10-31T22:59:59.999Z
    char temp[100] = "";
    strncpy(temp, get_data, 100);
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

send_ready* getData(char *http_request) {
    int zakresverification;
    if ((zakresverification = verifyDataOdDataDoo(strstr(http_request, "/") + 2)) != 1) {
        if (zakresverification == -1) {
            send_ready* sr = sr_init_error_json(500, "REGEX sie zepsul :(");
            return sr;
        }
        send_ready* sr = sr_init_error_json(422, "Dziennik zly format zakresu dat");
        return sr;
    }
    MYSQL *sql = mysql_init(NULL);
    messlog("CRED: %s %s %s %s", host, user, passwd, db);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        errorlog("cannot connect to the database");
        send_ready* sr = sr_init_error_json(503, "MYSQL CONNECT");
        mysql_close(sql);
        return sr;
    }

    char dataOd[200] = "";
    strncpy(dataOd, strstr(http_request, "=") + 1, sizeof(dataOd));
    char dataDo[100] = "";
    strncpy(dataDo, strstr(dataOd, "=")+1, sizeof(dataDo));
    *strstr(dataOd, "T") = '\0';
    *strstr(dataDo, "T") = '\0';
    messlog("dataOd: %s, dataDo: %s", dataOd, dataDo);
    char query[500] = "";
    sprintf(query,  "SELECT `grupa`, `przedmiot`, `typ`, `data`, `opis` FROM `na_ocene` WHERE `data` BETWEEN '%s' AND '%s'", dataOd, dataDo);
    mysql_query(conn, query);
    MYSQL_RES *res = mysql_store_result(conn);
    mysql_close(sql);

    if (res == NULL) {
        messlog("MYSQL query error");
        send_ready* sr = sr_init_error_json(500, "MYSQL query error");
        return sr;
    }
    int rowc = mysql_num_rows(res);
    char **data[rowc];
    for (int i = 0; i < rowc; i++) {
        data[i] = mysql_fetch_row(res);
    }
    send_ready* sr = wrap_in_json(data, rowc);
    mysql_free_result(res);
    return sr;
}

send_ready* actually_inserting_data(char data[5][200]) {
    MYSQL *sql = mysql_init(NULL);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        errorlog("cannot connect to the database");
        send_ready* sr = sr_init(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "MYSQL CONNECT", 0);
        //sr_set_line(sr, "{\"grupa\": \"gr1\", \"przedmiot\": \"j_ang\", \"typ\": \"zadanie\", \"data\": \"2024-10-10\", \"opis\":\"MYSQL CONNECT\"}", 1);
        mysql_close(sql);
        return sr;
    }
    char query[300];
    sprintf(query,  "INSERT INTO `na_ocene` (`grupa`, `przedmiot`, `typ`, `data`, `opis`) VALUES ('%s', '%s', '%s', '%s', '%s')", data[0], data[1], data[2], data[3], data[4]);

    int queryerr = mysql_query(sql, query);
    if (queryerr) {
        errorlog("Inserting not succesfull");
        send_ready* sr = sr_init(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "MYSQL INSERT", 0);
        //sr_set_line(sr, "{\"grupa\": \"gr1\", \"przedmiot\": \"j_ang\", \"typ\": \"zadanie\", \"data\": \"2024-10-10\", \"opis\":\"MYSQL INSERT\"}", 1);
        mysql_close(sql);
        return sr;
    }
    mysql_close(sql);
    send_ready* sr = sr_init(1);
    if (!queryerr) {
        sr_set_line(sr, "OK\0", 0);
    } 
    return sr;
}

send_ready* insertData(char *body) {
    //name=wartosc&name=wartosc&name=wartosc
    int isvalue = 0;    // 0 - property, 1 - value
    int propertyindex = 0;
    char mysql_ready[5][200] = {"", "", "", "", ""};
    char temppropertyname[20];
    memset(temppropertyname, 0, sizeof(temppropertyname));
    for (int i = 0; *body != '\0'; i++) {
        if (*body == '=') {
            messlog("property: %s, %s", temppropertyname, field_names[propertyindex]);
            if (strcmp(temppropertyname, field_names[propertyindex]) != 0) {
                errorlog("wrong format: %s", temppropertyname);
                send_ready* sr = sr_init(1);
                sr_set_http_code(sr, 422);
                return sr;
            }
            memset(temppropertyname, 0, sizeof(temppropertyname));
            i = 0;
            isvalue = 1;
            body++;
        }
        if (*body == '&') {
            isvalue = 0;
            propertyindex++;
            i = 0;
            body++;
        }
        if (!isvalue)
            temppropertyname[i] = *body;
        else
            mysql_ready[propertyindex][i] = *body;
        body++;
    }

    
    send_ready* sr = actually_inserting_data(mysql_ready);
    return sr;

}
