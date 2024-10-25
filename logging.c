#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void messlog(char* format, ...) {
    char mess[2000];
    va_list args;
    va_start(args, format);
    vsnprintf(mess, sizeof(mess), format, args);
    va_end(args);
    const time_t the_time = time(0);
    struct tm* curr = localtime(&the_time);
    printf("[%d.%d.%d %d:%d]: %s\n", curr->tm_mday, curr->tm_mon + 1, curr->tm_year + 1900, curr->tm_hour, curr->tm_min, mess);
}

void errorlog(char* format, ...) {
    char mess[2000];
    va_list args;
    va_start(args, format);
    vsnprintf(mess, sizeof(mess), format, args);
    va_end(args);
    const time_t the_time = time(0);
    struct tm* curr = localtime(&the_time);
    printf("\e[31mERROR: [%d.%d.%d %d:%d]: %s\e[0m\n", curr->tm_mday, curr->tm_mon + 1, curr->tm_year + 1900, curr->tm_hour, curr->tm_min, mess);
}
