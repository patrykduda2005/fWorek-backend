#ifndef MYSQLDATA_H
#define MYSQLDATA_H

#include "send_ready.h"

send_ready_line* getData(char *mess);
void insertData(char* data);

#endif
