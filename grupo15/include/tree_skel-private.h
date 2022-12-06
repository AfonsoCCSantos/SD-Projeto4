/*
* Grupo nº: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378
*/

#ifndef _TREE_SKEL_PRIVATE_H
#define _TREE_SKEL_PRIVATE_H

#include "tree_skel.h"
#include "zookeeper/zookeeper.h"

/**
* Adiciona o request ao fim da fila
*/
void add_to_queue(struct request_t* to_add);

/**
* Remove e retorna o primeiro elemento da fila
*/
struct request_t* queue_pop();

/**
* Liberta a memoria alocada para a fila
*/
void free_queue();

/**
* Termina a execucao das threads secundarias criadas
*/
void server_kill();

int connect_zookeeper(char* zookeeper_addr_port, char* server_addr_port);

int create_znode();

//int children_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

int update_new_server();

#endif