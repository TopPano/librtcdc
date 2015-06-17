#include <string.h>
#include <libwebsockets.h>
#include <rtcdc.h>
#include "../signaling.h"
#include "sy.h"




void on_channel(){}
void on_connect(){}

int callback_SDP(){}

static struct libwebsocket_protocols protocols[] = {
    {
        "SDP-protocol",
        callback_SDP,
        sizeof(struct signal_SDP_data),
        10
    },  
    {NULL, NULL, 0, 0 }
};


void on_candidate(rtcdc_peer_connection *peer, char *candidate, void *user_data){

    char *candidate_sets = (char *)user_data;
    if (candidate_sets == NULL)
        candidate_sets = candidate;
    else{
        int new_candidate_sets_size = strlen(candidate_sets) + strlen(candidate)+1;
        char *new_candidate_set = (char *)calloc(1, SDP_SIZE*sizeof(char));
        strcpy(new_candidate_set, candidate_sets);
        strcpy(new_candidate_set, candidate);
        free (candidate_sets);
        candidate_sets = new_candidate_set;
    }
}

SDP_t *sy_create_SDP(){

    SDP_t *local_SDP = NULL;  
    char *candidate_sets = NULL;
    rtcdc_peer_connection *client = rtcdc_create_peer_connection((void *)on_channel, (void *)on_candidate, (void *)on_connect, "stun.services.mozilla.com", 0, (void *)candidate_sets); 
    strcpy(local_SDP->sdp, rtcdc_generate_offer_sdp(client));
    strcpy(local_SDP->candidate, candidate_sets);

    return local_SDP;
}

SYcode sy_init(sy_session_t *session, char *repo_name, char *apikey, char *token)
{
    /*
     * connect to signal server
     * signal_initial(protocols);
     *
     * SDP_t *local_SDP = sy_create_SDP();
     * signal_exchange_SDP(sy_SDP);
     * 
     */

}


int main()
{
    SDP_t *local_SDP = sy_create_SDP();
    return 0;
}
