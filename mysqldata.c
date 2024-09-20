#include "mysqldata.h"
#include "mysqlcredentials.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

const char *field_names[5] = {
    "grupa",
    "przedmiot",
    "typ",
    "data",
    "opis"
};
void wrap_in_json(char *output, char ***data, int rowc) {
    //5 fields
    strcat(output, "["); output++;
    for (int row = 0; row < rowc; row++) {
        strcat(output, "{"); output++;
        //output += sprintf(output, "{");
        for (int i = 0; i < 5; i++) {
            output += sprintf(output, "\"%s\": \"%s\"", field_names[i], data[row][i]);
            if (i != 4)
                output += sprintf(output, ",");
        }
        strcat(output, "}"); output++;
        if (row != rowc-1)
            output += sprintf(output, ",");
    }
    strcat(output, "]");

}

void getData(char *mess) {
    MYSQL *sql = mysql_init(NULL);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        printf("cannot connect to the database");
        mysql_close(sql);
        return;
    }
    mysql_query(conn, "SELECT `grupa`, `przedmiot`, `typ`, `data`, `opis` FROM `na_ocene`");
    MYSQL_RES *res = mysql_store_result(conn);

    int rowc = mysql_num_rows(res);
    char **data[rowc];
    for (int i = 0; i < rowc; i++) {
        data[i] = mysql_fetch_row(res);
    }
    wrap_in_json(mess, data, rowc);
    mysql_free_result(res);
    mysql_close(sql);
}

void actually_inserting_data(char data[5][200]) {
    MYSQL *sql = mysql_init(NULL);
    MYSQL *conn = mysql_real_connect(sql, host, user, passwd, db, 0, NULL, 0);
    if (conn == NULL) {
        printf("cannot connect to the database");
        mysql_close(sql);
        return;
    }
    char query[300];
    sprintf(query,  "INSERT INTO `na_ocene` (`grupa`, `przedmiot`, `typ`, `data`, `opis`) VALUES ('%s', '%s', '%s', '%s', '%s')", data[0], data[1], data[2], data[3], data[4]);

    int queryerr = mysql_query(sql, query);
    if (queryerr) {
        printf("Inserting not succesfull");
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

    actually_inserting_data(&mysql_ready[0]);

    printf("\n");
    for (int i = 0; i < 5; i++) {
        printf("%d: %s\n", i, mysql_ready[i]);
    }
    printf("\n");
}
