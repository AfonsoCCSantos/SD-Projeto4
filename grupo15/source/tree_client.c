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
#include "client_stub-private.h"

#define BAD_FORMAT "Bad format\n"

//struct rtree_t* client_stub;
// struct rtree_t* head;
// struct rtree_t* tail;

// void update_head(char* address_port_head) {
//     free(head);
//     //rtree_disconnect(head); //dar free ao outro anterior
//     head = rtree_connect(address_port_head); 
//     if (head == NULL) {
//         perror("Connection failed\n");
//         exit(-1);
//     }
// }

// void update_tail(char* address_port_tail) {
//     free(tail);
//     //rtree_disconnect(tail); //dar free ao outro anterior
//     tail = rtree_connect(address_port_tail); 
//     if (tail == NULL) {
//         perror("Connection failed\n");
//         exit(-1);
//     }
// }

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, disconnect_client);
    if (argc == 1) {
        perror("Missing server and port\n");
        exit(-1);
    }
    //address_port (ip:porto) do zookeeper
    char* address_port = argv[1];
    if (connect_zookeper(address_port) < 0) {
        perror("Error connecting to zookeeper");
        exit(-1);
    }
    
    if (get_head_tail_servers() < 0) {
        //desconectar de zookeeper
        perror("Error getting head and tail servers");
        exit(-1);
    }

    // client_stub = rtree_connect(address_port);
    // if (client_stub == NULL) {
    //     perror("Connection failed\n");
    //     exit(-1);
    // }
    
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
                if(rtree_put(get_head_server(),entry) == -1) {
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
                struct data_t* data_get = rtree_get(get_tail_server(),key);
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
                if (rtree_del(get_head_server(),key) == -1) {
                    printf("Key not found\n");
                }
            }
        }
        else if (strcmp("size",op) == 0) {
            printf("%d\n",rtree_size(get_tail_server()));
        }
        else if (strcmp("height",op) == 0) {
            printf("%d\n",rtree_height(get_tail_server()));
        }
        else if (strcmp("getkeys",op) == 0) {
            char** keys = rtree_get_keys(get_tail_server());
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
            void** values = rtree_get_values(get_tail_server());
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
                printf("%s\n",rtree_verify(get_tail_server(), atoi(op_n)) ? 
                       "The request has been processed" : "The request has not been processed");
            }
        } 
        else {
            printf("Invalid command\n");
        }
    }   
}

void disconnect_client() {
    
    if (rtree_disconnect(get_head_server()) == -1) {
        perror("Error disconnecting the head server\n");
    }
    if (rtree_disconnect(get_tail_server()) == -1) {
        perror("Error disconnecting the tail server\n");
    }
    if (disconnect_zookeeper() < 0) exit(-1);
    exit(0);
}