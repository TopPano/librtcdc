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
/*
    struct rtcdc_data_channel *upload_dc = rtcdc_create_data_channel(peer, "Upload Channel",
                                                                    "", upload_fs_on_open,
                                                                    upload_fs_on_message, NULL, (void *)user_data);
    if(upload_dc == NULL)
        fprintf(stderr, "signaling_rtcdc: fail create data channel\n");
*/
//    printf("upload_fs_on_connect: %p, %p, %p\n", user_data, upload_dc->user_data, upload_dc);
}


/* download_fs part */
void uv_fs_download(void *work)
{
    
    struct sy_rtcdc_info_t *rtcdc_info = (struct sy_rtcdc_info_t *)work;
    struct rtcdc_data_channel *data_channel = rtcdc_info->data_channel; 
    const struct sy_diff_t *sy_session_diff = rtcdc_info->sy_session_diff;
    struct diff_info_t *file = sy_session_diff->files_diff;
    char local_file_path[LOCAL_PATH_SIZE];
    int files_num = sy_session_diff->num;
    int i, packet_index;
    struct sctp_packet_t packet;
    for(i = 0; i<files_num; i++)
    {
        memset(local_file_path, 0, LOCAL_PATH_SIZE);
        strncpy(local_file_path, rtcdc_info->local_repo_path, strlen(rtcdc_info->local_repo_path));
        strcat(local_file_path, file[i].filename);
        if(file[i].dirty == FILE_CLIENT_LACK || file[i].dirty == FILE_DIRTY)
        {
            FILE *pFile;
            pFile = fopen(local_file_path, "r");
            if(pFile == NULL)
                fprintf(stderr, "fail open file: %s\n", local_file_path);
            else{
                packet_index = 0;
                memset(&packet, 0, sizeof(packet));
                packet.index = packet_index;
                strcpy(packet.buf, file[i].filename);

                printf("file_path: %s\n", local_file_path);
                rtcdc_send_message(data_channel, RTCDC_DATATYPE_STRING, (void *)&packet, sizeof(packet));

                while(!feof(pFile))
                {
                    packet_index++;
                    memset(&packet, 0, sizeof(packet));
                    if(fgets(packet.buf, SCTP_BUF_SIZE, pFile) == NULL)
                        break;
                    //printf("buf: %s\n", packet.buf);
                    packet.index = packet_index;
                    packet.buf_len = strlen(packet.buf);
                    rtcdc_send_message(data_channel, RTCDC_DATATYPE_STRING, (void *)&packet, sizeof(packet));
                    usleep(200);
                }
                memset(&packet, 0, sizeof(packet));
                packet.index = -1;
                packet.buf_len = 0;
                rtcdc_send_message(data_channel, RTCDC_DATATYPE_STRING, (void *)&packet, sizeof(packet));
                
                fclose(pFile);
            }
        }
    }
}



void download_fs_on_message(struct rtcdc_data_channel *channel,
                          int datatype, void *data, 
                          size_t len, void *user_data)
{
/* nothing to do */
}


void download_fs_on_open(struct rtcdc_data_channel *channel, void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: download fs on_open\n");
#endif
    /*TODO: uniray: I wanna use uv_queue_work to run uv_fs_download(). 
     * Howerver, it results in core dump and I still dont know why.
     * I use uv_thread instead
     * */
    /*
    uv_loop_t download_loop; 
    uv_loop_init(&download_loop);
    uv_work_t download_work;
    download_work.data = (void *)user_data;
    uv_queue_work(&download_loop, &download_work, uv_fs_download, NULL);
    */
    uv_thread_t download_thread;
    uv_thread_create(&download_thread, uv_fs_download, user_data);
}


void download_fs_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc,
                void *user_data)
{
    dc->on_message = download_fs_on_message;
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: download fs on_channel\n");
#endif
}


void download_fs_on_connect(struct rtcdc_peer_connection *peer
                        ,void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: download fs on_connect\n");
    fprintf(stderr, "signaling_rtcdc: start create data channel\n");
#endif
    struct rtcdc_data_channel *download_dc = rtcdc_create_data_channel(peer, "Download Channel",
                                                                        "", download_fs_on_open,
                                                                        NULL, NULL, user_data);
    if(download_dc == NULL)
        fprintf(stderr, "signaling_rtcdc: fail create data channel\n");

    struct sy_rtcdc_info_t *rtcdc_info = (struct sy_rtcdc_info_t *)user_data;
    rtcdc_info->data_channel = download_dc;
}


