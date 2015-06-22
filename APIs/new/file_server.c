#include "signal.h"
#include <stdio.h>


struct conn_info *signal_conn;

int main(int argc, char *argv[]){
    // initial signaling
    const char *address = "140.112.90.37";
    int port = 7681;
    signal_conn = signal_initial(address, port);
    struct libwebsocket_context *context = signal_conn->context;
    signal_connect(context);

    return 0;
}

