#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sy_rtcdc.h"


void on_candidate(struct rtcdc_peer_connection *peer, 
                    char *candidate, 
                    void *user_data)
{
#ifdef DEBUG_RTCDC
    //fprintf(stderr, "signaling_rtcdc: on candidate\n");
#endif
    char *endl = "\n";
    char *candidate_set = ((struct sy_rtcdc_info_t *)user_data)->local_candidates;
    strcat(candidate_set, candidate);
    strcat(candidate_set, endl);
}


void uv_rtcdc_loop(uv_work_t *work)
{
    struct rtcdc_peer_connection *peer = (struct rtcdc_peer_connection *)work->data;
    rtcdc_loop(peer);
}

char *getline_candidates(char **string)
{
    char *line = (char *)calloc(1, 128*sizeof(char));
    int str_iter;
    if((*string)[0] == 0)
        return NULL;
    for (str_iter = 0; ; str_iter++)
    {
        if((*string)[str_iter] != '\n')
            line[str_iter] = (*string)[str_iter];
        else
        {    
            *string+=(str_iter+1); 
            break;
        }
    }
    return line;

}

void parse_candidates(struct rtcdc_peer_connection *peer, char *candidates)
{
    char *line;
    while((line = getline_candidates(&candidates)) != NULL){    
        rtcdc_parse_candidate_sdp(peer, line);
    }
}

