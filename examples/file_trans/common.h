#include <rtcdc.h>

#define BUFFER_SIZE 1200

struct sctp_packet{
    int index;
    char data[BUFFER_SIZE];
    int data_len;
};

struct peer_info{
    struct rtcdc_peer_connection* rtcdc_peer;
    char* local_peer;
    char* remote_peer;
    char* local_file_path;
    char* remote_file_path;
};
