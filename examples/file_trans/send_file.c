#include "signal.h"
#include "common.h"
#include <stdio.h>
#include <stdint.h>
#include <rtcdc.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <pthread.h>
#include <time.h>

clock_t start_time1, start_time2, end_time;

typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;
typedef struct peer_info peer_info;

typedef struct sctp_packet sctp_packet;

struct rtcdc_data_channel *offerer_dc;
static uv_loop_t *main_loop;
static uv_buf_t iov; 
static uv_fs_t open_req,read_req; 
char *local_file_path, *remote_file_path;
struct conn_info__SDP *signal_conn;
char *candidate_sets;
void on_open_file();



void on_open_channel(){
    fprintf(stderr, "open channel!\n");
    uv_fs_open(main_loop, &open_req, local_file_path, O_RDONLY, 0, on_open_file);
}

void on_message(rtcdc_data_channel* dc, int datatype, void* data, size_t len, void* user_data){
    sctp_packet* packet_instance = (sctp_packet*)data;

    char *msg = (char*)calloc(1, (packet_instance->data_len)*sizeof(char));
    strncpy(msg, (char*)packet_instance->data, packet_instance->data_len); 

    int index = packet_instance->index;
    printf("index:%d\n", index);
}

void on_close_channel(){
    fprintf(stderr, "close channel!\n");
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
    printf("on connect!\n");
}


void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}


void on_close_file(uv_fs_t* req){
    end_time = clock();
    clock_t gap_time = start_time2 - start_time1;
    float total_secs = ((float)((end_time-gap_time)-start_time2)/CLOCKS_PER_SEC);        
    printf("close file\n");
    printf("total_sec: %f\n", total_secs);
}


void on_read_file(uv_fs_t *req) {
    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
    }else if (req->result == 0) {
        uv_fs_t close_req;
        // synchronous
        uv_fs_close(uv_default_loop(), &close_req, open_req.result, on_close_file);
    }else if (req->result > 0) {

        iov.len = req->result;
        
        static int packet_index = 0;
        sctp_packet* packet_instance = (sctp_packet*)calloc(1, sizeof(sctp_packet));
       /* 
        packet_instance->data_len = iov.len;
        strcpy(packet_instance->data, iov.base);
        packet_instance->index = packet_index;
        rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)packet_instance, sizeof(sctp_packet));
        usleep(5);

        packet_index++;
        */
        if(packet_index == 0 && iov.len == BUFFER_SIZE)
        {
            //send the first packet which contains destination file path
            packet_instance->index = packet_index;
            strcpy(packet_instance->data, remote_file_path);
            packet_instance->data_len = strlen(remote_file_path);
            printf("data_len:%d\n", (int)strlen(remote_file_path));
            rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)packet_instance, sizeof(sctp_packet));
            packet_index++;

            packet_instance->index = packet_index;
            strcpy(packet_instance->data,iov.base);
            packet_instance->data_len = iov.len;
            rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)packet_instance, sizeof(sctp_packet));
            packet_index++;
        }else if(packet_index == 0 && iov.len < BUFFER_SIZE){
            //if the file is so small that need to send only one time
            //send the first packet which contains destination file path
            packet_instance->index = packet_index;
            strcpy(packet_instance->data, remote_file_path);
            packet_instance->data_len = strlen(remote_file_path);      
            rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)packet_instance, sizeof(sctp_packet));
            packet_index++;                      

            packet_instance->index = -1;
            strcpy(packet_instance->data,iov.base);
            packet_instance->data_len = iov.len; 
            rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)packet_instance, sizeof(sctp_packet));

            printf("Finish sending\n");

        }else if (packet_index > 0 && iov.len == BUFFER_SIZE){
            packet_instance->index = packet_index;
            strcpy(packet_instance->data,iov.base);
            packet_instance->data_len = iov.len;
            rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)packet_instance, sizeof(sctp_packet));
            packet_index++;
        }else if (packet_index > 0 && iov.len < BUFFER_SIZE){
            //send the last packet
            packet_instance->index = -1;
            strcpy(packet_instance->data,iov.base);
            packet_instance->data_len = iov.len;
            rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)packet_instance, sizeof(sctp_packet));

            printf("Finish sending\n");
        }
        usleep(5);
        memset(iov.base, '\0', iov.len);
        uv_fs_read(main_loop, req, open_req.result, &iov, 1, -1, on_read_file);

    }
}


void on_open_file(uv_fs_t *req){
    if (req->result >= 0){
        printf("open file\n");
        start_time1 = clock();
        start_time2 = clock();
        char* buffer = (char*)calloc(1, BUFFER_SIZE*sizeof(char));
        iov = uv_buf_init(buffer, BUFFER_SIZE);
        int res = uv_fs_read(main_loop, &read_req, req->result, &iov, 1, -1, on_read_file);
    }
    else {
        fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
    }
}


void trans_file(uv_work_t *work){
    //TODO infinite retry
    sleep(5);

    rtcdc_peer_connection* offerer = (rtcdc_peer_connection*)work->data;
    offerer_dc = rtcdc_create_data_channel(offerer, "Demo Channel", "", on_open_channel, on_message, NULL, NULL); 
    if(offerer_dc != NULL){
        //uv_fs_open(main_loop, &open_req, local_file_path, O_RDONLY, 0, on_open_file);
    }
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

    candidate_sets = (char *)calloc(1, DATASIZE*sizeof(char));
    // initial signaling
    const char *address = "140.112.90.37";
    int port = 7681;
    signal_conn = signal_initial(address, port);
    struct libwebsocket_context *context = signal_conn->context;
    pthread_t signal_thread;
    pthread_create(&signal_thread, NULL, &signal_connect, (void *)context);

    rtcdc_peer_connection* offerer = rtcdc_create_peer_connection((void*)on_channel, (void*)on_candidate, on_connect, "stun.services.mozilla.com", 0, (void*)peer_sample);


    // signaling send local sdp
    char *local_sdp = rtcdc_generate_offer_sdp(offerer);
    signal_send_SDP(signal_conn, argv[1], local_sdp);

    printf("ready to connect?(y/n)");
    if(fgetc(stdin)=='y'){
        // signaling get remote sdp
        // signaling get remote candidate
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
        uv_work_t work[2];
        work[0].data = (void*)offerer;
        work[1].data = (void*)offerer;
        uv_queue_work(main_loop, &work[0], sync_loop, NULL);
        uv_queue_work(main_loop, &work[1], trans_file, NULL);
        return uv_run(main_loop, UV_RUN_DEFAULT);
    }

    return 0;
}

