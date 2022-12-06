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
#include <zookeeper/zookeeper.h>
#include "client_stub.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"
#include "tree_client-private.h"

#define CHAIN_NODE "/chain"
#define CHILD_NODE_PATH_LEN 12

typedef struct String_vector zoo_string;

static zhandle_t* zh;
char* curr_path_head;
char* curr_path_tail;

static void children_watcher_client(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    zoo_string* children_list = malloc(sizeof(zoo_string));
    if (children_list == NULL) return;
    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_CHILD_EVENT) {
            if (zoo_wget_children(zh, CHAIN_NODE, children_watcher_client, watcher_ctx, children_list) != ZOK) {
                return;
            }
            char* head_path = *(children_list[0].data);
            char* tail_path = *(children_list[0].data);
            for (int i = 0; i < children_list->count;i++) {
                char* curr_node_path = *(children_list[i].data);
                if (strcmp(curr_node_path,head_path) < 0) {
                    head_path = curr_node_path;
                }
                else if (strcmp(curr_node_path,tail_path) > 0) {
                    tail_path = curr_node_path;
                }
            }
            if (strcmp(head_path,curr_path_head) != 0) {
                memcpy(curr_path_head, head_path, CHILD_NODE_PATH_LEN);
                int head_buffer_len = 22;
                char* head_buffer = malloc(head_buffer_len);
                if (head_buffer == NULL) {
                    free(children_list);
                    return;
                }
                if (zoo_get(zh,head_path,0,head_buffer,&head_buffer_len,NULL) != ZOK) {
                    free(children_list);
                    free(head_buffer);
                    return;
                }
                update_head(head_buffer);
            }
            else if (strcmp(tail_path,curr_path_tail) != 0) {
                memcpy(curr_path_tail, tail_path, CHILD_NODE_PATH_LEN);
                int tail_buffer_len = 22;
                char* tail_buffer = malloc(tail_buffer_len);
                if (tail_buffer == NULL) {
                    free(children_list);
                    return;
                }
                if (zoo_get(zh,head_path,0,tail_buffer,&tail_buffer_len,NULL) != ZOK) {
                    free(children_list);
                    free(tail_buffer);
                    return;
                }
                update_tail(tail_buffer);
            }
        }
    }
}

int connect_zookeper(char* zookeeper_addr_port) {
    if (zookeeper_addr_port == NULL) return -1;
    zh = zookeeper_init(zookeeper_addr_port,NULL,2000,0,NULL,0);
    if (zh == NULL) return -1;
    return 0;
}

int get_head_tail_servers(struct rtree_t** head, struct rtree_t** tail) {
    curr_path_head = malloc(CHILD_NODE_PATH_LEN);
    curr_path_tail = malloc(CHILD_NODE_PATH_LEN);
    zoo_string* children_list = malloc(sizeof(zoo_string));
    if (children_list == NULL) return -1;
    static char *watcher_ctx = "ZooKeeper Data Watcher";
    if (zoo_wget_children(zh, CHAIN_NODE, children_watcher_client, watcher_ctx, children_list) != ZOK) {
        return -1;
    }
    char* head_path = *(children_list[0].data);
    char* tail_path = *(children_list[0].data);
    for (int i = 0; i < children_list->count;i++) {
        char* curr_node_path = *(children_list[i].data);
        if (strcmp(curr_node_path,head_path) < 0) {
            head_path = curr_node_path;
        }
        else if (strcmp(curr_node_path,tail_path) > 0) {
            tail_path = curr_node_path;
        }
    }
    memcpy(curr_path_head, head_path, CHILD_NODE_PATH_LEN);
    memcpy(curr_path_tail, tail_path, CHILD_NODE_PATH_LEN);
    int head_buffer_len = 22; //IP:PORTO
    char* head_buffer = malloc(head_buffer_len);
    if (head_buffer == NULL) {
        free(children_list);
        return -1;
    }
    if (zoo_get(zh,head_path,0,head_buffer,&head_buffer_len,NULL) != ZOK) {
        free(children_list);
        free(head_buffer);
        return -1;
    }
    int tail_buffer_len = 22;//IP:PORTO
    char* tail_buffer = malloc(tail_buffer_len);
    if (tail_buffer == NULL) {
        free(children_list);
        free(head_buffer);
        return -1;
    }
    if (zoo_get(zh,head_path,0,tail_buffer,&tail_buffer_len,NULL) != ZOK) {
        free(children_list);
        free(head_buffer);
        free(tail_buffer);
        return -1;
    }
    *head = rtree_connect(head_buffer);
    *tail = rtree_connect(tail_buffer);
    return 0;
}

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

int disconnect_zookeeper() {
    if (zookeeper_close(zh) != ZOK) return -1;
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
