/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#ifndef _TREE_CLIENT_PRIVATE_H
#define _TREE_CLIENT_PRIVATE_H

/**
* Termina o programa cliente
*/
void disconnect_client();

int update_tail(char* address_port_tail);

int update_head(char* address_port_head);
#endif