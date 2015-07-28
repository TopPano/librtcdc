#include <stdio.h>
#include <rtcdc.h>
#include <string.h>
#include "signaling_rtcdc.h"

#define STUN_IP "stun.services.mozilla.com"

void upload_client_on_message()
{}


void upload_fs_on_message()
{}


void on_open_channel()
{}


void on_candidate(struct rtcdc_peer_connection *peer, 
                    char *candidate, 
                    void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: on candidate\n");
#endif
    char *endl = "\n";
    char *candidate_set = (char *)user_data;
    strcat(candidate_set, candidate);
    strcat(candidate_set, endl);
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


void upload_client_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc,
                void *user_data)
{
    dc->on_message = upload_client_on_message;
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload client on_channel\n");
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
                                                                    "", on_open_channel,
                                                                    NULL, NULL, NULL);
    if(upload_dc == NULL)
        fprintf(stderr, "signaling_rtcdc: fail create data channel\n");
}


void upload_client_on_connect(struct rtcdc_peer_connection *peer
                        ,void *user_data)
{
#ifdef DEBUG_RTCDC
    fprintf(stderr, "signaling_rtcdc: upload client on_connect\n");
    fprintf(stderr, "signaling_rtcdc: start create data channel\n");
#endif
    struct rtcdc_data_channel *upload_dc = rtcdc_create_data_channel(peer, "Upload Channel",
                                                                    "", on_open_channel,
                                                                    NULL, NULL, NULL);
    if(upload_dc == NULL)
        fprintf(stderr, "signaling_rtcdc: fail create data channel\n");
}
