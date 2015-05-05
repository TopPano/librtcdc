#include <stdio.h>
#include <stdint.h>
#include <rtcdc.h>


struct peer_info{
    struct rtcdc_peer_connection* rtcdc_peer;
    char* local_peer;
    char* remote_peer;
    char* local_file_path;
    char* remote_file_path;
};


//type_name specify the file is candidate of SDP
char* signal_get_SDP_filename(char* peer_name, char* type_name);

// generate local SDP as file
void signal_gen_local_SDP_file(struct rtcdc_peer_connection* peer, struct peer_info* peer_sample);


void signal_get_remote_SDP_file(struct rtcdc_peer_connection* peer, struct peer_info* peer_sample);

void signal_get_remote_candidate_file(struct rtcdc_peer_connection* peer, struct peer_info* peer_sample);
