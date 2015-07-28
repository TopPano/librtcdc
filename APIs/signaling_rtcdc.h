#include <rtcdc.h>
#define STUN_IP "stun.services.mozilla.com"

void on_candidate(struct rtcdc_peer_connection *peer, 
                    char *candidate, 
                    void *user_data);

void upload_fs_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc, 
                void *user_data);

void upload_client_on_channel(struct rtcdc_peer_connection *peer,
                struct rtcdc_data_channel *dc, 
                void *user_data);

void upload_fs_on_connect(struct rtcdc_peer_connection *peer,
                void *user_data);

void upload_client_on_connect(struct rtcdc_peer_connection *peer,
                void *user_data);
