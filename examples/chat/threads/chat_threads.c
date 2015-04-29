#include <stdio.h>
#include <stdint.h>
#include <rtcdc.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <pthread.h>
typedef struct rtcdc_peer_connection rtcdc_peer_connection;
typedef struct rtcdc_data_channel rtcdc_data_channel;
typedef struct peer_info{
    char* local_name;
    char* remote_name;
} peer_info;

struct rtcdc_data_channel *offerer_dc;

char* get_line(FILE* file){
    char *line = calloc(1, 256);
    char *line_iter = line;
    char ch = fgetc(file);
    
    if(ch == EOF){
        return NULL;
    }

    while(ch != '\n'){
        *line_iter =ch;
        line_iter++;
        ch = fgetc(file);
    }
    *line_iter = '\n';
    return line; 
}


void on_open(){
    fprintf(stderr, "on_open:open\n");
}

void on_message(rtcdc_data_channel* dc, int datatype, void* data, size_t len, void* user_data){
    char *msg = (char*)calloc(1, len*sizeof(char));
    strncpy(msg, (char*)data,len); 
    printf("len:%d, msg:%s\n", len, msg);   
}

void on_close(){
    printf("close!\n");
}

void on_channel(rtcdc_peer_connection *peer, rtcdc_data_channel* dc, void* user_data){
    dc->on_message = on_message;
    printf("on_channel:success channel!\n");
}

void on_candidate(rtcdc_peer_connection *peer, char *candidate, void *user_data ){
    peer_info* peer_sample = (peer_info*) user_data;
    
    FILE *f_local_candidate;
    char *abs_file_path = calloc(1,256);
    strcat(abs_file_path, "/home/uniray/Project/librtcdc/examples/chat/threads/");
    strcat(abs_file_path, peer_sample->local_name);
    strcat(abs_file_path, "/local_candidates");
    f_local_candidate = fopen(abs_file_path, "a");
    if(f_local_candidate == NULL){
        fprintf(stderr, "Error while opening the file.\n");
        exit(1);
    }
    fprintf(f_local_candidate, "%s\r\n", candidate);
    free(abs_file_path);
    fclose(f_local_candidate);  

    printf("%s\n", candidate);
}

void on_connect(){
    printf("on_connect:success connect!\n");
}



// generate local candidate as file
/*void gen_local_candidate_file(rtcdc_peer_connection* peer, char* file_path)
{
    char *local_candidate;
    FILE *f_local_candidate;
    char *abs_file_path = calloc(1,256);
    strcat(abs_file_path, "/home/uniray/Project/WebRTC/librtc_thread/");
    strcat(abs_file_path, file_path);
    strcat(abs_file_path, "/local_candidates");
    f_local_candidate = fopen(abs_file_path, "w");
    if(f_local_candidate == NULL){
        fprintf(stderr, "Error while opening the file.\n");
        exit(1);
    }
    local_candidate = rtcdc_generate_local_candidate_sdp(peer);
    fprintf(f_local_candidate, "%s", local_candidate);
    free(abs_file_path);
    fclose(f_local_candidate);  
}
*/

// generate local SDP as file
void gen_local_SDP_file(rtcdc_peer_connection* peer, char* file_path)
{
    char *local_sdp;
    FILE *f_local_sdp;
    char *abs_file_path = calloc(1,256);
    strcat(abs_file_path, "/home/uniray/Project/librtcdc/examples/chat/threads/");
    strcat(abs_file_path, file_path);
    strcat(abs_file_path, "/local_sdp");
    f_local_sdp = fopen(abs_file_path, "w");
    if(f_local_sdp == NULL){
        fprintf(stderr, "Error while opening the file.\n");
        exit(1);
    }
    local_sdp  = rtcdc_generate_offer_sdp(peer);
    fprintf(f_local_sdp, "%s", local_sdp);
    fprintf(stderr, "%s", local_sdp);
    free(abs_file_path);
    fclose(f_local_sdp);
}


void get_remote_SDP_file(rtcdc_peer_connection* peer, char* file_path){
    FILE *f_remote_sdp;
    char *abs_file_path = calloc(1,256);
    strcat(abs_file_path, "/home/uniray/Project/librtcdc/examples/chat/threads/");
    strcat(abs_file_path, file_path);
    strcat(abs_file_path, "/local_sdp");
    f_remote_sdp = fopen(abs_file_path, "r");
    if(f_remote_sdp == NULL){
      fprintf(stderr, "Error while opening the file.\n");
      exit(1);
    }
    char* remote_sdp;
    long file_size;
    fseek(f_remote_sdp, 0L, SEEK_END);
    file_size = ftell(f_remote_sdp);
    rewind(f_remote_sdp);
    remote_sdp = calloc(1, file_size+1);
    fread(remote_sdp, file_size, 1, (FILE*)f_remote_sdp);
    fclose(f_remote_sdp);  

    int res  = rtcdc_parse_offer_sdp(peer, remote_sdp);
    if (res >= 0){
        fprintf(stderr, "success parse remote SDP\n");
    }else{
        fprintf(stderr, "failed parse remote SDP\n");
        exit(1);
    }
}


void get_remote_candidate_file(rtcdc_peer_connection* peer, char* file_path){
    FILE *f_remote_candidate;
    char *abs_file_path = calloc(1,256);
    strcat(abs_file_path, "/home/uniray/Project/librtcdc/examples/chat/threads/");
    strcat(abs_file_path, file_path);
    strcat(abs_file_path, "/local_candidates");
f_remote_candidate = fopen(abs_file_path, "r");
    if(f_remote_candidate ==NULL){
        fprintf(stderr, "Error while opening the file.\n");
        exit(1);
    } 
    char *line;
    int res;
    while((line = get_line(f_remote_candidate))!=NULL){
        res = rtcdc_parse_candidate_sdp(peer, line);
        free(line);
        if (res > 0){
            //fprintf(stderr, "success parse remote candidate\n");
        }else{
            fprintf(stderr, "failed parse remote candidate\n");
            exit(1);
        }
    }
    printf("success parse remote candidates!\n");
    fclose(f_remote_candidate);
}


void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}


void read_stdin(uv_stream_t *stream, ssize_t nread, const uv_buf_t *stdin_buf){
    if (nread>1){
        char *msg = stdin_buf->base;
        printf("len:%d, msg:%s\n", strlen(msg), msg);
        rtcdc_send_message(offerer_dc, RTCDC_DATATYPE_STRING, (void *)msg, strlen(msg));
      }
    memset((stdin_buf->base),'\0', 100);
    free(stdin_buf->base);
}


void sync_loop(void *peer){
    rtcdc_peer_connection* offerer = (rtcdc_peer_connection*)peer;
    rtcdc_loop(offerer);
}


void send_msg(void *peer){
    sleep(5);
    printf("try create data channel(y/n)?\n");
    rtcdc_peer_connection* offerer = (rtcdc_peer_connection*)peer;
    offerer_dc = rtcdc_create_data_channel(offerer, "Demo Channel", "", on_open, on_message, NULL, NULL); 

    uv_loop_t *send_loop = uv_default_loop();
    uv_pipe_t stdin_pipe;
    uv_pipe_init(send_loop, &stdin_pipe, 0);
    uv_pipe_open(&stdin_pipe, 0);
    uv_read_start((uv_stream_t*)&stdin_pipe, alloc_buffer, read_stdin);
    uv_run(send_loop, UV_RUN_DEFAULT);
}




int main(int argc, char *argv[]){
    // argv[1] is local file name
    // argv[2] is remote file name

    if(argc!=3){
        fprintf(stderr, "argument error!");
        return 0;
    }
    
    // clear the content of local_candidate file
    FILE *f_local_candidate;
    char *abs_file_path = calloc(1,256);
    strcat(abs_file_path, "/home/uniray/Project/librtcdc/examples/chat/threads/");
    strcat(abs_file_path, argv[1]);
    strcat(abs_file_path, "/local_candidates");
    f_local_candidate = fopen(abs_file_path, "w");
    if(f_local_candidate == NULL){
        fprintf(stderr, "Error while clear the file.\n");
        exit(1);
    }
    fclose(f_local_candidate);  


    // argv[1] is local file name
    // argv[2] is remote file name

    peer_info* peer_sample = (peer_info*)calloc(1, sizeof(peer_info));
    peer_sample->local_name = argv[1];
    peer_sample->remote_name = argv[2];


    // create rtcdc peer
    // the peer_sample is the local&remote infomation

    // on_candidate will be called if create peer connection successfully
    // on_candidate store the candidate in the local_candidate file
    rtcdc_peer_connection* offerer = rtcdc_create_peer_connection(on_channel, on_candidate, on_connect, "stun.services.mozilla.com", NULL, (void*)peer_sample);
    
    // generate local SDP and store in the file
    gen_local_SDP_file(offerer, argv[1]);

    printf("ready to connect?(y/n)");
    if(fgetc(stdin)=='y'){
        get_remote_SDP_file(offerer,argv[2]); 
        
        // clear the content of local_candidate file
        f_local_candidate = fopen(abs_file_path, "w");
        if(f_local_candidate == NULL){
            fprintf(stderr, "Error while clear the file.\n");
            exit(1);
        }
        free(abs_file_path);
        fclose(f_local_candidate);  
       

        get_remote_candidate_file(offerer, argv[2]);
        gen_local_SDP_file(offerer, argv[1]);
        
        pthread_t thread_1, thread_2;
        pthread_create(&thread_1, NULL, (void*)sync_loop, (void*) offerer);
        
        pthread_create(&thread_2, NULL, (void*)send_msg, (void*) offerer);
        pthread_join(thread_1, NULL);
        pthread_join(thread_2, NULL);
    }
    
    return 0;
}

