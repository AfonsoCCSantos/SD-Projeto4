/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "client_stub.h"
#include "data.h"
#include "entry.h"
#include "tree.h"
#include "tree_client-private.h"

#define BAD_FORMAT "Bad format\n"

struct rtree_t* client_stub;

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, disconnect_client);
    if (argc == 1) {
        perror("Missing server and port\n");
        exit(-1);
    }
    const char* address_port = argv[1];
    client_stub = rtree_connect(address_port);
    if (client_stub == NULL) {
        perror("Connection failed\n");
        exit(-1);
    }
    
    char line[100];
    const char separator[2] = " ";
    while (1) {
        if (fgets(line, 100, stdin) == NULL) {
            perror("Failed to read op");
            exit(-1);
        }
        line[strlen(line)-1] = 0;
        char* op = strtok(line,separator);
        
        if (op == NULL) {
            printf(BAD_FORMAT);
        }
        else if (strcmp("put",op) == 0) {
            char* key = strtok(NULL,separator); 
            char* data = strtok(NULL,"\n");
            if (key == NULL || data == NULL) {
                printf(BAD_FORMAT);
            }
            else {
                struct data_t* data_to_send = data_create2(strlen(data) + 1, data);
                if (data_to_send == NULL) {
                    perror("Data create failed\n");
                    exit(-1);
                }
                struct entry_t* entry = entry_create(key,data_to_send);
                if (entry == NULL) {
                    perror("Entry create failed\n");
                    exit(-1);
                }
                if(rtree_put(client_stub,entry) == -1) {
                    perror("Tree put failed\n");
                    exit(-1);
                }
                free(entry->value);
                free(entry);
            }
        }
        else if (strcmp("get",op) == 0) {
            char* key = strtok(NULL,separator);
            if (key == NULL){
                printf(BAD_FORMAT);
            }
            else {
                struct data_t* data_get = rtree_get(client_stub,key);
                if (data_get == NULL) {
                    perror("Tree get failed\n");
                    exit(-1); 
                }
                if (data_get->datasize == 0) {
                    printf("Key not found\n");
                }
                else {
                    printf("\tSize of the data: %d bytes\n", data_get->datasize);
                    printf("\tData: %s\n", (char*) data_get->data);
                }  
                data_destroy(data_get);
            }
        }
        else if (strcmp("del",op) == 0) {
            char* key = strtok(NULL,separator);
            if(key == NULL){
                printf(BAD_FORMAT);
            }
            else {
                if (rtree_del(client_stub,key) == -1) {
                    printf("Key not found\n");
                }
            }
        }
        else if (strcmp("size",op) == 0) {
            printf("%d\n",rtree_size(client_stub));
        }
        else if (strcmp("height",op) == 0) {
            printf("%d\n",rtree_height(client_stub));
        }
        else if (strcmp("getkeys",op) == 0) {
            char** keys = rtree_get_keys(client_stub);
            if(keys == NULL) {
                perror("Getkeys failed");
                exit(-1);
            }
            if (keys[0] == NULL) {
                printf("There are no keys\n");
            }
            else {
                printf("Keys: ");
                for (int i = 0; keys[i] != NULL; i++) {
                    printf("%s ",keys[i]);
                }
                printf("\n");
            } 
            tree_free_keys(keys);
        } 
        else if (strcmp("getvalues",op) == 0) {
            void** values = rtree_get_values(client_stub);
             if (values == NULL) {
                perror("Getvalues failed\n");
                exit(-1);
            }
            if (values[0] == NULL) {
                printf("There are no values\n");
            }
            else {
                printf("Values:\n");
                for(int i = 0; values[i] != NULL; i++) {
                    printf("\t%s\n",(char*) values[i]);
                }
            }
            tree_free_values(values);
        }
        else if (strcmp("quit",op) == 0) { 
            disconnect_client();
        }
        else if (strcmp("verify",op) == 0) {
            char* op_n = strtok(NULL,separator);
            if (op_n == NULL) {
                printf(BAD_FORMAT);
            }
            else {
                printf("%s\n",rtree_verify(client_stub, atoi(op_n)) ? 
                       "The request has been processed" : "The request has not been processed");
            }
        } 
        else {
            printf("Invalid command\n");
        }
    }   
}

void disconnect_client() {
    if (rtree_disconnect(client_stub) == -1) {
        perror("Error disconnecting the client stub\n");
    }
    exit(0);
}