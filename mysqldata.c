#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mysqldata.h"
#include "mysqlcredentials.h"
#include "logging.h"

const char *field_names[5] = {
    "grupa",
    "przedmiot",
    "typ",
    "data",
    "opis"
};

send_ready_line* wrap_in_json(char ***data, int rowc) {
    int field_lenght = strlen(field_names[0]) + strlen(field_names[1]) + strlen(field_names[2]) 
        + strlen(field_names[3]) + strlen(field_names[4]);
    //5 fields
    messlog("Wrapping..");

    send_ready_line* sr = malloc(sizeof(char) * LINESIZE * (rowc + 2 + 1));
    memset(sr, 0, sizeof(*sr));

    strcat(sr[0], "[");
    for (int row = 0; row < rowc; row++) {
        char** row_of_data = data[row];

        int line_length = field_lenght + strlen(row_of_data[0]) + strlen(row_of_data[1]) 
            + strlen(row_of_data[2]) + strlen(row_of_data[3]) + strlen(row_of_data[4]) + 8;
        //if (LINESIZE > line_length) return NULL;

        char* send_ready_row = sr[row + 1];
        send_ready_row += sprintf(send_ready_row, "{");
        for (int i = 0; i < 5; i++) {
            send_ready_row += sprintf(send_ready_row, "\"%s\": \"%s\"", field_names[i], data[row][i]);
            if (i != 4)
                send_ready_row += sprintf(send_ready_row, ",");
        }
        strcat(send_ready_row, "}"); send_ready_row++;
        if (row != rowc-1)
            send_ready_row += sprintf(send_ready_row, ",");
    }
    sprintf(sr[rowc+1], "]");
    sprintf(sr[rowc+2], "\0");

    messlog("Wrapping done");
    return sr;
}

send_ready_line* getData() {
    MYSQL *sql = mysql_init(NULL);
    messlog("CRED: %s %s %s %s", host, user, passwd, db);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        messlog("cannot connect to the database");
        mysql_close(sql);
        return NULL;
    }
    mysql_query(conn, "SELECT `grupa`, `przedmiot`, `typ`, `data`, `opis` FROM `na_ocene`");
    MYSQL_RES *res = mysql_store_result(conn);
    mysql_close(sql);

    int rowc = mysql_num_rows(res);
    char **data[rowc];
    for (int i = 0; i < rowc; i++) {
        data[i] = mysql_fetch_row(res);
    }
    send_ready_line* sr = wrap_in_json(data, rowc);
    mysql_free_result(res);
    return sr;
}

send_ready_line* actually_inserting_data(char data[5][200]) {
    MYSQL *sql = mysql_init(NULL);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        messlog("cannot connect to the database\n");
        mysql_close(sql);
        return NULL;
    }
    char query[300];
    sprintf(query,  "INSERT INTO `na_ocene` (`grupa`, `przedmiot`, `typ`, `data`, `opis`) VALUES ('%s', '%s', '%s', '%s', '%s')", data[0], data[1], data[2], data[3], data[4]);

    int queryerr = mysql_query(sql, query);
    if (queryerr) {
        messlog("Inserting not succesfull");
    }
    mysql_close(sql);
    send_ready_line* sr = malloc(sizeof(char) * LINESIZE * 2);
    memset(sr, 0, sizeof(sr));
    if (!queryerr) {
        sprintf(sr[0], "OK\0");
    } else {
        sprintf(sr[0], "NOT OK\0");
    }
    strcat(sr[1], "\0");
    return sr;
}

send_ready_line* insertData(char *body) {
    //name=wartosc&name=wartosc&name=wartosc
    int isvalue = 0;    // 0 - property, 1 - value
    int propertyindex = 0;
    char mysql_ready[5][200] = {"", "", "", "", ""};
    char temppropertyname[20];
    for (int i = 0; *body != '\0'; i++) {
        if (*body == '=') {
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

    
    send_ready_line* sr = actually_inserting_data(mysql_ready);
    return sr;

}
