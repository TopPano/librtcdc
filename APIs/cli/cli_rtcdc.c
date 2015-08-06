#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli_rtcdc.h"
#include "sy.h"


void uv_upload(uv_work_t *work)
{
    struct sy_rtcdc_info_t *rtcdc_info = (struct sy_rtcdc_info_t *)work->data;
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
        if(file[i].dirty == FILE_FS_LACK || file[i].dirty == FILE_DIRTY)
        {
            FILE *pFile;
            pFile = fopen(local_file_path, "r");
            if(pFile == NULL)
                fprintf(stderr, "fail open file: %s\n", local_file_path);
            else{
                packet_index = 0;
                memset(&packet, 0, sizeof(packet));
                packet.index = packet_index;
                strcpy(packet.buf, local_file_path);

                rtcdc_send_message(data_channel, RTCDC_DATATYPE_STRING, (void *)&packet, sizeof(packet));

                while(!feof(pFile))
                {
                    packet_index++;
                    memset(&packet, 0, sizeof(packet));
                    if(fgets(packet.buf, SCTP_BUF_SIZE, pFile) == NULL)
                        break;
                    packet.index = packet_index;
                    packet.buf_len = strlen(packet.buf);
                    rtcdc_send_message(data_channel, RTCDC_DATATYPE_STRING, (void *)&packet, sizeof(packet));
                    usleep(100);
                }
                fclose(pFile);
            }
            printf("file_path: %s\n", local_file_path);
        }
    }
}



void upload_client_on_message(struct rtcdc_data_channel *channel,
                              int datatype, void *data, 
                              size_t len, void *user_data)
{
    printf("receive from fs\n");
}


void upload_client_on_open(struct rtcdc_data_channel *channel, void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload client on_open\n");
#endif
    /* allocate a uv work to upload files */
    struct sy_rtcdc_info_t *rtcdc_info = (struct sy_rtcdc_info_t *)user_data; 
    rtcdc_info->data_channel = channel;
    uv_loop_t *uv_loop = (uv_loop_t *)rtcdc_info->uv_loop;
    
    /* TODO: uniray: i dont know why it will segamentation fault in uv_upload if without declaring tt */
    struct sy_rtcdc_info_t tt;

    uv_work_t upload_work;
    upload_work.data = (void *)rtcdc_info;
    uv_queue_work(uv_loop, &upload_work, uv_upload, NULL);
}


void upload_client_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc,
                void *user_data)
{
    dc->on_message = upload_client_on_message;
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload client on_channel\n");
#endif
}


void upload_client_on_connect(struct rtcdc_peer_connection *peer
                        ,void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload client on_connect\n");
    fprintf(stderr, "signaling_rtcdc: start create data channel\n");
#endif
    struct rtcdc_data_channel *upload_dc = rtcdc_create_data_channel(peer, "Upload Channel",
                                                                    "", upload_client_on_open,
                                                                    NULL, NULL, user_data);
    if(upload_dc == NULL)
        fprintf(stderr, "signaling_rtcdc: fail create data channel\n");
}


