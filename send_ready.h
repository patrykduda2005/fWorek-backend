#ifndef SEND_READY_H
#define SEND_READY_H

#define LINELIMIT 300
#define LINESIZE (LINELIMIT + /*{},*/3 + /*,*/4 + /*\0*/1)

//struct send_ready {
//    char (*data)[LINESIZE];
//};
typedef char send_ready_line[LINESIZE];

#endif
