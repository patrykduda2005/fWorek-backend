#ifndef MYSQLDATA_H
#define MYSQLDATA_H

#include "send_ready.h"

send_ready* getData(char* http_request);
send_ready* insertData(char* body);

#endif
