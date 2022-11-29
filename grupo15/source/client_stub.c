/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include "client_stub.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"

struct rtree_t *rtree_connect(const char *address_port) {
    char* server = strtok((char*) address_port,":");
    char* port = strtok(NULL,":");
    if (server == NULL || port == NULL) {
        printf("Bad format of arguments.\n");
        exit(-1);
    } 
    
    struct sockaddr_in server_socket;

    server_socket.sin_family = AF_INET;
    server_socket.sin_port = htons(atoi(port));
    if (inet_pton(AF_INET, server, &server_socket.sin_addr) < 1) {
        return NULL;
    }
    
    struct rtree_t* rtree = malloc(sizeof(struct rtree_t));
    if (rtree == NULL) {
        return NULL;
    }    
    rtree->address = &server_socket;
    
    if (network_connect(rtree) < 0) {
        free(rtree);
        return NULL;
    }
    
    return rtree;
}

int rtree_disconnect(struct rtree_t *rtree) {
    if (network_close(rtree) < 0 ) {
        free(rtree);
        return -1; 
    }
    free(rtree);
    return 0;
}

int rtree_put(struct rtree_t *rtree, struct entry_t *entry) {
    MessageT msg;   
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;
    
    MessageT__EntryMessage entry_message;
    message_t__entry_message__init(&entry_message);
    entry_message.key = malloc(strlen(entry->key) + 1);
    if (entry_message.key == NULL) return -1;
    strcpy(entry_message.key,entry->key);
    ProtobufCBinaryData entry_message_data;
    entry_message_data.len = entry->value->datasize;
    entry_message_data.data = malloc(entry->value->datasize);
    if (entry_message_data.data == NULL) {
        free(entry_message.key);
        return -1;
    }
    memcpy(entry_message_data.data, entry->value->data, entry->value->datasize);
    entry_message.data = entry_message_data;
    msg.entry = &entry_message;
    
    MessageT* rcv_msg = network_send_receive(rtree,&msg);
    free(entry_message.key);
    free(entry_message_data.data);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return -1;
    }
    printf("Your operation's code: %d\n", rcv_msg->num);
    message_t__free_unpacked(rcv_msg, NULL);
    return 0;
}

struct data_t *rtree_get(struct rtree_t *rtree, char *key) {
    MessageT msg;
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    
    msg.n_data = 1;
    msg.data = malloc(msg.n_data * sizeof(ProtobufCBinaryData));
    if (msg.data == NULL) return NULL;
    int size = strlen(key) + 1;
    msg.data[0].len = size;
    msg.data[0].data = malloc(size);
    if (msg.data[0].data == NULL) {
        free(msg.data);
        return NULL;
    }
    memcpy(msg.data[0].data,key,msg.data[0].len);
    
    MessageT* rcv_msg = network_send_receive(rtree,&msg);
    free(msg.data[0].data);
    free(msg.data);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return NULL;
    }
    
    struct data_t* res = malloc(sizeof(struct data_t));
    if (res == NULL) {
        message_t__free_unpacked(rcv_msg, NULL);    
        return NULL;
    }
    res->datasize = rcv_msg->data[0].len;
    res->data = malloc(rcv_msg->data[0].len);
    if (res->data == NULL) {
        message_t__free_unpacked(rcv_msg, NULL);
        return NULL;
    }
    memcpy(res->data, rcv_msg->data[0].data, rcv_msg->data[0].len);

    message_t__free_unpacked(rcv_msg, NULL);
    return res;
}

int rtree_del(struct rtree_t *rtree, char *key) {
    MessageT msg;
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    
    msg.n_data = 1;
    msg.data = malloc(msg.n_data * sizeof(ProtobufCBinaryData));
    if (msg.data == NULL) return -1;
    int size = strlen(key) + 1;
    
    msg.data[0].len = size;
    msg.data[0].data = malloc(size);
    if (msg.data[0].data == NULL) {
        free(msg.data);
        return -1;
    }
    memcpy(msg.data[0].data,key,msg.data[0].len);
    
    MessageT* rcv_msg = network_send_receive(rtree, &msg);
    free(msg.data[0].data);
    free(msg.data);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return -1;
    }
    printf("Your operation's code: %d\n", rcv_msg->num);

    message_t__free_unpacked(rcv_msg, NULL);
    return 0;
}

int rtree_size(struct rtree_t *rtree) {
    MessageT msg;
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT* rcv_msg = network_send_receive(rtree,&msg);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return -1;
    }
    
    int res = rcv_msg->num;

    message_t__free_unpacked(rcv_msg, NULL);
    return res;
}

int rtree_height(struct rtree_t *rtree) {
    MessageT msg;
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    MessageT* rcv_msg = network_send_receive(rtree,&msg);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return -1;
    }

    int res = rcv_msg->num;
    message_t__free_unpacked(rcv_msg, NULL);
    return res;
} 

char **rtree_get_keys(struct rtree_t *rtree) {
    MessageT msg;
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    MessageT* rcv_msg = network_send_receive(rtree,&msg);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return NULL;
    }
    
    int n_keys = rcv_msg->n_data;
    //Plus one since we have to put the last element as NULL
    char** res = malloc((n_keys + 1) * sizeof(char*));
    if (res == NULL) {
        message_t__free_unpacked(rcv_msg, NULL);
        return NULL;
    }
    for (int i = 0; i < n_keys; i++) {
        res[i] = malloc(rcv_msg->data[i].len);
        if (res[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(res[j]);
            }
            free(res);
            message_t__free_unpacked(rcv_msg, NULL);
            return NULL;
        }
        memcpy(res[i],rcv_msg->data[i].data,rcv_msg->data[i].len);
    }
    res[n_keys] = NULL;
    message_t__free_unpacked(rcv_msg, NULL);
    return res;
}

void **rtree_get_values(struct rtree_t *rtree) {
    MessageT msg;
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GETVALUES;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    MessageT* rcv_msg = network_send_receive(rtree,&msg);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return NULL;
    }
    
    int n_values = rcv_msg->n_data;
    //Plus one since we have to put the last element as NULL
    void** res = malloc((n_values+1) * sizeof(void*));
    if (res == NULL) {
        message_t__free_unpacked(rcv_msg, NULL);
        return NULL;
    }
    for (int i = 0; i < n_values; i++) {
        res[i] = malloc(rcv_msg->data[i].len);
        if (res[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(res[j]);
            }
            free(res);
            message_t__free_unpacked(rcv_msg, NULL);
            return NULL;
        }
        memcpy(res[i],rcv_msg->data[i].data, rcv_msg->data[i].len);
    }
    res[n_values] = NULL;
    
    message_t__free_unpacked(rcv_msg, NULL);
    return res;
}

int rtree_verify(struct rtree_t *rtree, int op_n) {
    MessageT msg;
    
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_VERIFY;
    msg.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg.num = op_n;

    MessageT* rcv_msg = network_send_receive(rtree,&msg);
    if (rcv_msg == NULL || rcv_msg->opcode == MESSAGE_T__OPCODE__OP_ERROR) {
        message_t__free_unpacked(rcv_msg, NULL);
        return -1;
    }
    
    int res = rcv_msg->num;

    message_t__free_unpacked(rcv_msg, NULL);
    return res;
}
