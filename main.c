#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "mysqldata.h"
#include "process_string.h"
#include "send_ready.h"
#include "signalhandling.h"
#include "connection.h"


#define CONNECT 0

int main() {
    init_signal();

    if (CONNECT) {
        int sock = setup_bind_and_listen_on_socket(21339);
        while(continue_execution) {
            receive_http_request(sock);
        }
        close(sock);
        return 0;
    }

    char post_mess[1000] = "POST / HTTP/1.1 Host: frog01.mikr.us:21339\r\n Connection: keep-alive\r\n Content-Length: 68\r\n Cache-Control: max-age=0\r\n Upgrade-Insecure-Requests: 1\r\n Origin: http://localhost:3000\r\n Content-Type: application/x-www-form-urlencoded\r\n User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36\r\n Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n Referer: http://localhost:3000/\r\n Accept-Encoding: gzip, deflate\r\n Accept-Language: pl-PL,pl;q=0.9,en-US;q=0.8,en;q=0.7\r\n\r\ngrupa=obie&przedmiot=j_ang&typ=zadanie&data=2024-09-10&opis=andrzej duda lubi w dupe";
    char get_mess[1000] = "GET /?dataOd=2024-11-10T17:48:44.969Z&dataDo=2024-11-12T18:48:44.969Z HTTP/1.1 Host: frog01.mikr.us:21339\r\n Connection: keep-alive\r\n Content-Length: 68\r\n Cache-Control: max-age=0\r\n Upgrade-Insecure-Requests: 1\r\n Origin: http://localhost:3000\r\n Content-Type: application/x-www-form-urlencoded\r\n User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36\r\n Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n Referer: http://localhost:3000/\r\n Accept-Encoding: gzip, deflate\r\n Accept-Language: pl-PL,pl;q=0.9,en-US;q=0.8,en;q=0.7";

    char body[1000] = "";

    //send_ready* sr = getData();
    //send_ready* sr_dwa = sr_init_json(3);
    //sr_set_line(sr_dwa, "{KANAPA: fsda},", 1);
    //sr_set_line(sr_dwa, "{KANAPA: fsdaf},", 2);
    //sr_set_line(sr_dwa, "{KANAPA: fsda}", 3);
    //send_ready* wow = sr_join_json(sr, sr_dwa);
    //sr_print(wow);
    //sr_free(sr);
    //get_body(body, mess);
    //insertData(body);
    struct http_response hr;
    process_get_method(&hr, get_mess);
    //sr_print(hr.body);
    //send_ready_line* sr = hr.body;
    //int rak = 0;
    //if (sr != NULL) {
    //    for (int i = 0; rak < 2; i++) {
    //        if (sr[i][0] == '\0') rak++;
    //        printf("%s\n", sr[i]);
    //    }
    //}
    //free(sr);
    //send_ready_line* data = getData(body);
    //for (int i = 0; ; i++) {
    //    printf("%s\n", data[i]);
    //    if (data[i][0] == ']') break;
    //}
    //free(data);
    //printf("Body:\n%s\n", body);

    return 0;
}
