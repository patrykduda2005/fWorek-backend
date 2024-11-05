#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "mysqldata.h"
#include "mysqlcredentials.h"
#include "logging.h"
#include "send_ready.h"
#include "process_string.h"

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
        sr_set_json_line(sr, data[row][0], data[row][1], data[row][2], data[row][3], data[row][4], row + 1, row != (rowc - 1));
    }
    messlog("Wrapping done");
    return sr;
}

send_ready* getData(char *http_request) {
    int zakresverification;
    if ((zakresverification = verifyDataOdDataDo(strstr(http_request, "/") + 2)) != 1) {
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
        send_ready* sr = sr_init_error_json(503, "MYSQL CONNECT");
        mysql_close(sql);
        return sr;
    }
    char query[300];
    sprintf(query,  "INSERT INTO `na_ocene` (`grupa`, `przedmiot`, `typ`, `data`, `opis`) VALUES ('%s', '%s', '%s', '%s', '%s')", data[0], data[1], data[2], data[3], data[4]);

    int queryerr = mysql_query(sql, query);
    if (queryerr) {
        errorlog("Inserting not succesfull");
        send_ready* sr = sr_init_error_json(503, "MYSQL INSERT");
        mysql_close(sql);
        return sr;
    }
    mysql_close(sql);
    send_ready* sr = sr_init_json(1);
    if (!queryerr) {
        sr_set_line(sr, "{\"typ\": \"ok\", \"opis\": \"OK\"}", 1);
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
                send_ready* sr = sr_init_error_json(422, "wrong format");
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
