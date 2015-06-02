#include "signal.h"
#include "common.h"
#include <stdio.h>
#include <stdint.h>
#include <rtcdc.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <pthread.h>


typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;
typedef struct peer_info peer_info;

typedef struct sctp_packet sctp_packet;

struct rtcdc_data_channel *offerer_dc;
static uv_loop_t* trans_loop, *main_loop;

char *candidate_sets, *recvd_dir;
struct conn_info__SDP *signal_conn;

void on_open(){
    fprintf(stderr, "on open!!\n");
}

void on_message(rtcdc_data_channel* dc, int datatype, void* data, size_t len, void* user_data){

    sctp_packet* packet_instance = (sctp_packet*)data;

    int index = packet_instance->index;
    int data_len = (packet_instance->data_len);
    char *msg = (char*)calloc(1, (packet_instance->data_len)*sizeof(char));
    strncpy(msg, (char*)packet_instance->data, packet_instance->data_len); 

    static FILE* recved_file;
    if(index == 0){
        char recvd_file_path[128];
        memset(recvd_file_path, 0, 128);
        strcat(recvd_file_path, recvd_dir);
        strcat(recvd_file_path, msg);

        recved_file = fopen(recvd_file_path, "w");
    }else if(index>0){
        fwrite(msg, sizeof(char), packet_instance->data_len, recved_file);
    }else if(index == -1){
        fwrite(msg, sizeof(char), packet_instance->data_len, recved_file);
        fclose(recved_file);
        printf("Finish receiving\n");
    }
}

void on_close(){
    fprintf(stderr, "close!\n");
}

void on_channel(rtcdc_peer_connection *peer, rtcdc_data_channel* dc, void* user_data){
    dc->on_message = on_message;
    fprintf(stderr, "on channel!!\n");
}

void on_candidate(rtcdc_peer_connection *peer, char *candidate, void *user_data ){
    char *endl = "\n";
    peer_info* peer_sample = (peer_info*) user_data;
    candidate_sets = strcat(candidate_sets, candidate);
    candidate_sets = strcat(candidate_sets, endl);
    // signaling send local candidate
    signal_send_candidate(signal_conn, peer_sample->local_peer, candidate_sets);
}


void on_connect(){
    printf("on_connect:success connect!\n");
}


void sync_loop(uv_work_t *work){
    rtcdc_peer_connection* offerer = (rtcdc_peer_connection*)work->data;
    rtcdc_loop(offerer);
}


int main(int argc, char *argv[]){
    if(argc!=4){
        fprintf(stderr, "argument error!");
        return 0;
    }

    // argv[1] is local peer name
    // argv[2] is remote peer name
    // argv[3] is the directory store the received file
    peer_info* peer_sample = (peer_info*)calloc(1, sizeof(peer_info));
    peer_sample->local_peer = argv[1];
    peer_sample->remote_peer = argv[2];
    recvd_dir = argv[3];
    candidate_sets = (char *)calloc(1, DATASIZE*sizeof(char));
    // initial signaling
    const char *address = "140.112.90.37";
    int port = 7681;
    signal_conn = signal_initial(address, port);
    struct libwebsocket_context *context = signal_conn->context;
    pthread_t signal_thread;
    pthread_create(&signal_thread, NULL, &signal_connect, (void *)context);
 
    // create rtcdc peer
    // the peer_sample is the local&remote infomation

    // on_candidate will be called if create peer connection successfully
    // on_candidate store the candidate in the local_candidate file
    rtcdc_peer_connection* offerer = rtcdc_create_peer_connection((void*)on_channel, (void*)on_candidate, on_connect, "stun.services.mozilla.com", 0, (void*)peer_sample);

    // generate local SDP and store in the file
    //signal_gen_local_SDP_file(offerer, peer_sample);
    
    
    // signaling send local sdp
    char *local_sdp = rtcdc_generate_offer_sdp(offerer);
    signal_send_SDP(signal_conn, argv[1], local_sdp);

    printf("ready to connect?(y/n)");
    if(fgetc(stdin)=='y'){
        
        char *remote_sdp, *remote_candidate;
        struct SDP_context *recvd_SDP_context = signal_req(signal_conn, argv[2]);
        remote_sdp = recvd_SDP_context->SDP;
        remote_candidate = recvd_SDP_context->candidate;
        rtcdc_parse_offer_sdp(offerer, remote_sdp);
        char *line;
        while((line = signal_getline(&remote_candidate))!=NULL)
        {    
            rtcdc_parse_candidate_sdp(offerer, line);
            printf("%s\n", line);
        }
        
        
        // signaling send local sdp
        local_sdp = rtcdc_generate_offer_sdp(offerer);
        signal_send_SDP(signal_conn, argv[1], local_sdp);

        main_loop = uv_default_loop();         
        uv_work_t work[1];
        work[0].data = (void*)offerer;
        uv_queue_work(main_loop, &work[0], sync_loop, NULL);
        return uv_run(main_loop, UV_RUN_DEFAULT);
    }

    return 0;
}

