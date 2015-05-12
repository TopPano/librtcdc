#include "signal.h"
#include <stdio.h>
#include <stdint.h>
#include <rtcdc.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <pthread.h>

#define BUFFER_SIZE 1100

typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;
typedef struct peer_info peer_info;

typedef struct sctp_packet{
    int index;
    char data[BUFFER_SIZE];
    int data_len;
} sctp_packet;

struct rtcdc_data_channel *offerer_dc;
static uv_loop_t* trans_loop, *main_loop;

char *local_file_path, *remote_file_path;


void on_open(){
    fprintf(stderr, "on open!!\n");
}

void on_message(rtcdc_data_channel* dc, int datatype, void* data, size_t len, void* user_data){
    sctp_packet* packet_instance = (sctp_packet*)data;

    char *msg = (char*)calloc(1, (packet_instance->data_len)*sizeof(char));
    strncpy(msg, (char*)packet_instance->data, packet_instance->data_len); 

    int index = packet_instance->index;
    printf("index:%d\n", index);
    static FILE* recved_file;
    if(index == 0){
        recved_file = fopen(msg, "w");
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
    peer_info* peer_sample = (peer_info*) user_data;

    //=====================start signaling====================================

    FILE *f_local_candidate;
    char *abs_file_path = signal_get_SDP_filename(peer_sample->local_peer, "/candidates");
    f_local_candidate = fopen(abs_file_path, "a");
    if(f_local_candidate == NULL){
        fprintf(stderr, "Error while opening the file.\n");
        exit(1);
    }
    fprintf(f_local_candidate, "%s\r\n", candidate);
    free(abs_file_path);
    fclose(f_local_candidate);  
    //=====================finish signaling====================================

    printf("%s\n", candidate);
}


void on_connect(){
    printf("on_connect:success connect!\n");
}


void sync_loop(uv_work_t *work){
    rtcdc_peer_connection* offerer = (rtcdc_peer_connection*)work->data;
    rtcdc_loop(offerer);
}


int main(int argc, char *argv[]){
    if(argc!=5){
        fprintf(stderr, "argument error!");
        return 0;
    }

    // argv[1] is local peer name
    // argv[2] is remote peer name

    peer_info* peer_sample = (peer_info*)calloc(1, sizeof(peer_info));
    peer_sample->local_peer = argv[1];
    peer_sample->remote_peer = argv[2];
    local_file_path = argv[3];
    remote_file_path = argv[4];


    // clear the content of local_candidate file
    FILE *f_local_candidate;
    char *abs_file_path = signal_get_SDP_filename(peer_sample->local_peer, "/candidates");
    f_local_candidate = fopen(abs_file_path, "w");
    if(f_local_candidate == NULL){
        fprintf(stderr, "Error while clear the file.\n");
        exit(1);
    }
    fclose(f_local_candidate);  

    // create rtcdc peer
    // the peer_sample is the local&remote infomation

    // on_candidate will be called if create peer connection successfully
    // on_candidate store the candidate in the local_candidate file
    rtcdc_peer_connection* offerer = rtcdc_create_peer_connection((void*)on_channel, (void*)on_candidate, on_connect, "stun.services.mozilla.com", NULL, (void*)peer_sample);

    // generate local SDP and store in the file
    signal_gen_local_SDP_file(offerer, peer_sample);

    printf("ready to connect?(y/n)");
    if(fgetc(stdin)=='y'){
        signal_get_remote_SDP_file(offerer, peer_sample); 

        // clear the content of local_candidate file
        f_local_candidate = fopen(abs_file_path, "w");
        if(f_local_candidate == NULL){
            fprintf(stderr, "Error while clear the file.\n");
            exit(1);
        }
        free(abs_file_path);
        fclose(f_local_candidate);  

        signal_get_remote_candidate_file(offerer, peer_sample);
        signal_gen_local_SDP_file(offerer, peer_sample);

        main_loop = uv_default_loop();         
        uv_work_t work[1];
        work[0].data = (void*)offerer;
        uv_queue_work(main_loop, &work[0], sync_loop, NULL);
        return uv_run(main_loop, UV_RUN_DEFAULT);
    }

    return 0;
}

