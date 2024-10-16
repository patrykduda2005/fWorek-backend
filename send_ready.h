#ifndef SEND_READY_H
#define SEND_READY_H

#define LINELIMIT 300
#define LINESIZE (LINELIMIT + /*{},*/3 + /*,*/4 + /*\0*/1)


typedef void send_ready;
typedef char send_ready_line[LINESIZE];

send_ready* sr_init_json(int lines);
send_ready* sr_init(int lines);
void sr_set_http_code(send_ready* sr, int code);
void sr_set_line(send_ready* sr, char* line, int index);
void sr_print(send_ready* sr);
send_ready* sr_join_json(send_ready* lhs, send_ready* rhs);
void sr_free(send_ready* sr);
void sr_sending(send_ready* sr, int connected_socket);

#endif
