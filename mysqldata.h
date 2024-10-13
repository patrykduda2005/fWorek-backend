#ifndef MYSQLDATA_H
#define MYSQLDATA_H

#include "send_ready.h"

send_ready_line* getData();
send_ready_line* insertData(char* body);

#endif
