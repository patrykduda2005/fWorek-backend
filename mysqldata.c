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
//struct send_ready wrap_in_json(char ***data, int rowc) {
//    //5 fields
//    messlog("Wrapping..");
//
//    char (*send_ready)[LINESIZE] = malloc(sizeof(char) * LINESIZE * (rowc + 2));
//    memset(send_ready, 0, sizeof(send_ready));
//
//    strcat(send_ready[0], "[");
//    for (int row = 0; row < rowc; row++) {
//        char* send_ready_row = send_ready[row + 1];
//        send_ready_row += sprintf(send_ready_row, "{");
//        //strcat(send_ready_row, "{"); send_ready_row++;
//        for (int i = 0; i < 5; i++) {
//            send_ready_row += sprintf(send_ready_row, "\"%s\": \"%s\"", field_names[i], data[row][i]);
//            if (i != 4)
//                send_ready_row += sprintf(send_ready_row, ",");
//        }
//        strcat(send_ready_row, "}"); send_ready_row++;
//        if (row != rowc-1)
//            send_ready_row += sprintf(send_ready_row, ",");
//    }
//    sprintf(send_ready[rowc+1], "]");
//
//    messlog("Wrapping done");
//    struct send_ready records;
//    records.data = send_ready;
//    return records;
//}


send_ready_line* wrap_in_json(char ***data, int rowc) {
    //5 fields
    messlog("Wrapping..");

    send_ready_line* sr = malloc(sizeof(char) * LINESIZE * (rowc + 2));
    memset(sr, 0, sizeof(sr));

    strcat(sr[0], "[");
    for (int row = 0; row < rowc; row++) {
        char* send_ready_row = sr[row + 1];
        send_ready_row += sprintf(send_ready_row, "{");
        //strcat(send_ready_row, "{"); send_ready_row++;
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

    messlog("Wrapping done");
    return sr;
}

send_ready_line* getData(char *mess) {
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
    mysql_free_result(res);
    return wrap_in_json(data, rowc);
}

void actually_inserting_data(char data[5][200]) {
    MYSQL *sql = mysql_init(NULL);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        messlog("cannot connect to the database\n");
        mysql_close(sql);
        return;
    }
    char query[300];
    sprintf(query,  "INSERT INTO `na_ocene` (`grupa`, `przedmiot`, `typ`, `data`, `opis`) VALUES ('%s', '%s', '%s', '%s', '%s')", data[0], data[1], data[2], data[3], data[4]);

    int queryerr = mysql_query(sql, query);
    if (queryerr) {
        messlog("Inserting not succesfull");
    }
    mysql_close(sql);
}

void insertData(char *data) {
    //name=wartosc&name=wartosc&name=wartosc
    int isvalue = 0;    // 0 - property, 1 - value
    int propertyindex = 0;
    char mysql_ready[5][200] = {"", "", "", "", ""};
    char temppropertyname[20];
    for (int i = 0; *data != '\0'; i++) {
        if (*data == '=') {
            memset(temppropertyname, 0, sizeof(temppropertyname));
            i = 0;
            isvalue = 1;
            data++;
        }
        if (*data == '&') {
            isvalue = 0;
            propertyindex++;
            i = 0;
            data++;
        }
        if (!isvalue)
            temppropertyname[i] = *data;
        else
            mysql_ready[propertyindex][i] = *data;
        data++;
    }

    actually_inserting_data(mysql_ready);

    printf("\n");
    for (int i = 0; i < 5; i++) {
        messlog("%d: %s\n", i, mysql_ready[i]);
    }
    printf("\n");
}
