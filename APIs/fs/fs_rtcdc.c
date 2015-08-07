#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs_rtcdc.h"


void upload_fs_on_message(struct rtcdc_data_channel *channel,
                          int datatype, void *data, 
                          size_t len, void *user_data)
{
    char *local_repo_path = (char *)user_data;

    //    printf("upload_fs_on_message: %p, %p, %p\n", user_data, channel->user_data, channel);
    struct sctp_packet_t* recvd_packet = (struct sctp_packet_t *)data;
    static FILE *pFILE;
    if(recvd_packet->index == 0)
    {
        char local_file_path[LOCAL_PATH_SIZE];
        memset(local_file_path, 0, LOCAL_PATH_SIZE);
        strcpy(local_file_path, local_repo_path);
        strcat(local_file_path, recvd_packet->buf);
        pFILE = fopen(local_file_path, "w");
        printf("filename:%s\n", local_file_path);
    }
    else if(recvd_packet->index == -1)
    {
        /*receive last buf*/
        printf("fininsh!!!!!!!!:%d\n", len);
        fclose(pFILE);
    }
    else if(recvd_packet->index > 0)
    {
        /*receive buf*/
        fwrite(recvd_packet->buf, sizeof(char), recvd_packet->buf_len, pFILE);
    }
    
}


void upload_fs_on_open(struct rtcdc_data_channel *channel, void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload fs on_open\n");
#endif
    struct sy_rtcdc_info_t *rtcdc_info = (struct sy_rtcdc_info_t *)user_data;
    rtcdc_info->data_channel = channel;
//    printf("upload_fs_on_open: %p, %p, %p\n", user_data, channel->user_data, channel);
}

void upload_fs_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc,
                void *user_data)
{
    dc->on_message = upload_fs_on_message;
    /* TODO: here is weird, u can observe the pointer addr of user_data, channel->user_data and channel 
     * the channel ptr addr is same with on_message's, but different with on_connect and on_open*/
    struct sy_rtcdc_info_t *rtcdc_info = (struct sy_rtcdc_info_t *)user_data;
    char *local_repo_path = rtcdc_info->local_repo_path;
    dc->user_data = (void *)local_repo_path;
    
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload fs on_channel\n");
#endif
//    printf("upload_fs_on_channel: %p, %p, %p\n", user_data, dc->user_data, dc);
}


void upload_fs_on_connect(struct rtcdc_peer_connection *peer
                        ,void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload fs on_connect\n");
    fprintf(stderr, "signaling_rtcdc: start create data channel\n");
#endif
    struct sy_rtcdc_info_t *rtcdc_info = (struct sy_rtcdc_info_t *)user_data;
    char local_repo_path[LOCAL_PATH_SIZE];
    memset(local_repo_path, 0, LOCAL_PATH_SIZE);
    strcpy(local_repo_path, rtcdc_info->local_repo_path);

    struct rtcdc_data_channel *upload_dc = rtcdc_create_data_channel(peer, "Upload Channel",
                                                                    "", upload_fs_on_open,
                                                                    upload_fs_on_message, NULL, (void *)user_data);
    if(upload_dc == NULL)
        fprintf(stderr, "signaling_rtcdc: fail create data channel\n");

//    printf("upload_fs_on_connect: %p, %p, %p\n", user_data, upload_dc->user_data, upload_dc);
}


