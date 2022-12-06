/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "zookeeper/zookeeper.h"

struct rtree_t {
    struct sockaddr_in* address;
    char* path;
    int socketfd;
};

int connect_zookeper(char* zookeeper_addr_port);

int get_head_tail_servers(struct rtree_t** head, struct rtree_t** tail);

int disconnect_zookeeper();

#endif
