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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>


#define NUMBER_SEC_THREADS 1
#define LOCALHOST "127.0.0.1" 

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
        perror("Missing port and zookeeper address:port \n");
        exit(-1); 
    }
    
    //argv[2] endereco: portozookeeper
    
    int socket_escuta = network_server_init(atoi(argv[1]));
    if (socket_escuta == -1) {
        perror("Server connection failed\n");
        exit(-1);
    }
    
    if (tree_skel_init(NUMBER_SEC_THREADS) == -1) {
        perror("Server skeleton connection failed\n");
        exit(-1);
    }
    
    char* server_adr = get_server_address(socket_escuta);
    if (server_adr == NULL){
        perror("Get server address failed\n");
        exit(-1);
    }
    
    char server_addr_port[15]; // 9 for ip, 4 for port,1 for ':', and 1 for '\0'
    sprintf(server_addr_port,"%s:%s",server_adr ,argv[1]);
    free(server_adr);
    printf("MEU IP:PORT %s\n", server_addr_port);
    connect_zookeeper(argv[2], server_addr_port);
    
    
    network_main_loop(socket_escuta);
    interrupt_server_listen();
}


char* get_server_address(int socket) {
    struct ifaddrs *addrs,*tmp;

    char ip_address[15];
    struct ifreq ifr;

    ifr.ifr_addr.sa_family = AF_INET;
    
    getifaddrs(&addrs);
    tmp = addrs;
    char* res = malloc(15);
    if (res == NULL) return NULL;

    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET) {
            memcpy(ifr.ifr_name, tmp->ifa_name, IFNAMSIZ - 1);
            
            ioctl(socket, SIOCGIFADDR, &ifr);
            strcpy(ip_address, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
        }
        if (strcmp(ip_address,LOCALHOST) != 0) {
            strcpy(res,ip_address);
            break;
        }
        tmp = tmp->ifa_next;
    }   
    
    freeifaddrs(addrs);
    return res;
}