#include "signal.h"
#include <stdlib.h>
#include <string.h>

//type_name specify the file is candidate of SDP
char* signal_get_SDP_filename(char* peer_name, char* type_name){
    char *abs_file_path = (char*)calloc(1,256*sizeof(char));
    strcat(abs_file_path, "/home/uniray/Project/librtcdc/examples/file_trans/");
    strcat(abs_file_path, peer_name);
    strcat(abs_file_path, type_name);
    return abs_file_path;
}


// generate local SDP as file
void signal_gen_local_SDP_file(struct rtcdc_peer_connection* peer, struct peer_info* peer_sample)
{
    char *local_sdp;
    local_sdp  = rtcdc_generate_offer_sdp(peer);
    //=====================start signaling====================================
    FILE *f_local_sdp;
    char *abs_file_path = signal_get_SDP_filename(peer_sample->local_peer, "/sdp");

    f_local_sdp = fopen(abs_file_path, "w");
    if(f_local_sdp == NULL){
        fprintf(stderr, "Error while opening the file.\n");
        exit(1);
    }
    fprintf(f_local_sdp, "%s", local_sdp);
    fprintf(stderr, "%s", local_sdp);
    free(abs_file_path);
    fclose(f_local_sdp);
    //=====================finish signaling====================================

}


void signal_get_remote_SDP_file(struct rtcdc_peer_connection* peer, struct peer_info* peer_sample){

    //=====================start signaling====================================
    FILE *f_remote_sdp;
    char *abs_file_path = signal_get_SDP_filename(peer_sample->remote_peer, "/sdp");
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
    //=====================finish signaling====================================

    int res  = rtcdc_parse_offer_sdp(peer, remote_sdp);
    if (res >= 0){
        fprintf(stderr, "success parse remote SDP\n");
    }else{
        fprintf(stderr, "failed parse remote SDP\n");
        exit(1);
    }
}


char* get_line(FILE* file){
    char *line = (char*)calloc(1, 256*sizeof(char));
    char *line_iter = line;
    char ch = fgetc(file);

    if(ch == EOF){
        return NULL;
    }

    while(ch != '\n'){
        *line_iter = ch;
        line_iter++;
        ch = fgetc(file);
    }
    *line_iter = '\n';
    return line; 
}



void signal_get_remote_candidate_file(struct rtcdc_peer_connection* peer, struct peer_info* peer_sample){

    //=====================start signaling====================================
    FILE *f_remote_candidate;
    char *abs_file_path = signal_get_SDP_filename(peer_sample->remote_peer, "/candidates");
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
    //=====================finish signaling====================================
}


