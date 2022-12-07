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

int get_head_tail_servers();

int disconnect_zookeeper();

struct rtree_t* get_head_server();

struct rtree_t* get_tail_server();

void update_head(char* address_port_head);

void update_tail(char* address_port_tail);

#endif
