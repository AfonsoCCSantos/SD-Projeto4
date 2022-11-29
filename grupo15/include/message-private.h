/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

/**
* Escreve len bytes de buf para a socket identificada por sock
*/
int write_all(int sock, char* buf, int len);

/**
* Le len bytes do buf para a socket identificado por sock
*/
int read_all(int sock, char* buf, int len);

#endif
