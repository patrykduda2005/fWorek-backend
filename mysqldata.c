#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

send_ready* getData() {
    MYSQL *sql = mysql_init(NULL);
    messlog("CRED: %s %s %s %s", host, user, passwd, db);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        errorlog("cannot connect to the database");
        send_ready* sr = sr_init(1);
        sr_set_http_code(sr, 503);
        sr_set_line(sr, "MYSQL CONNECT", 0);
        mysql_close(sql);
        return sr;
    }
    mysql_query(conn, "SELECT `grupa`, `przedmiot`, `typ`, `data`, `opis` FROM `na_ocene`");
    MYSQL_RES *res = mysql_store_result(conn);
    mysql_close(sql);

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
