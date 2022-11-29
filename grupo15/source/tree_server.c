/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "tree_skel.h"
#include "network_server.h"
#include "tree_server-private.h"
#include "tree_skel-private.h"

#define NUMBER_SEC_THREADS 1

void interrupt_server_listen() {
    server_kill();
    network_server_close();
    tree_skel_destroy();
    exit(0);
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, interrupt_server_listen);

    if (argc == 1) {
        perror("Missing port\n");
        exit(-1); 
    }
    
    int socket_escuta = network_server_init(atoi(argv[1]));
    if (socket_escuta == -1) {
        perror("Server connection failed\n");
        exit(-1);
    }

    if (tree_skel_init(NUMBER_SEC_THREADS) == -1) {
        perror("Server skeleton connection failed\n");
        exit(-1);
    }
    
    network_main_loop(socket_escuta);
    interrupt_server_listen();
}
