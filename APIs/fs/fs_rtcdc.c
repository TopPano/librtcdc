#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs_rtcdc.h"


void upload_fs_on_message(struct rtcdc_data_channel *channel,
                          int datatype, void *data, 
                          size_t len, void *user_data)
{
//    struct sy_rtcdc_info_t *tt = (struct sy_rtcdc_info_t *)user_data;
//    printf("RTCDC:upload_fs_on_message:local_repo_path:%s\n", tt->local_repo_path);
//    printf("receive from client: %s\n", (char *)data);
}


void upload_fs_on_open_channel(struct rtcdc_data_channel *channel, void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload fs on_open_channel\n");
#endif

}

void upload_fs_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc,
                void *user_data)
{
    dc->on_message = upload_fs_on_message;
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload fs on_channel\n");
#endif
}


void upload_fs_on_connect(struct rtcdc_peer_connection *peer
                        ,void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload fs on_connect\n");
    fprintf(stderr, "signaling_rtcdc: start create data channel\n");
#endif
    struct rtcdc_data_channel *upload_dc = rtcdc_create_data_channel(peer, "Upload Channel",
                                                                    "", upload_fs_on_open_channel,
                                                                    NULL, NULL, NULL);

    if(upload_dc == NULL)
        fprintf(stderr, "signaling_rtcdc: fail create data channel\n");
}


