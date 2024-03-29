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
#define CHILD_NODE_PATH_LEN 15

typedef struct String_vector zoo_string;

static zhandle_t* zh;
struct rtree_t* head;
struct rtree_t* tail;


struct rtree_t* get_head_server() {
    return head;
}

struct rtree_t* get_tail_server() {
    return tail;
}

void update_head(char* address_port_head) {
    char* new_path = malloc(strlen(head->path)+1);
    memcpy(new_path,head->path,strlen(head->path)+1);
    rtree_disconnect(head); //dar free ao outro anterior
    head = rtree_connect(address_port_head);
    head->path = new_path; 
    if (head == NULL) {
        perror("Connection failed\n");
        exit(-1);
    }
}

void update_tail(char* address_port_tail) {
    char* new_path = malloc(strlen(tail->path)+1);
    memcpy(new_path,tail->path,strlen(tail->path)+1);
    rtree_disconnect(tail); //dar free ao outro anterior
    tail = rtree_connect(address_port_tail); 
    tail->path = new_path;
    if (tail == NULL) {
        perror("Connection failed\n");
        exit(-1);
    }
}

static void children_watcher_client(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    zoo_string* children_list = malloc(sizeof(zoo_string));
    if (children_list == NULL) return;
    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_CHILD_EVENT) {
            if (zoo_wget_children(zh, CHAIN_NODE, children_watcher_client, watcher_ctx, children_list) != ZOK) {
                return;
            }
            if (children_list->count == 0) {
                printf("All servers disconnected, client will close!\n");
                if (rtree_disconnect(get_head_server()) == -1) {
                    perror("Error disconnecting the head server\n");
                }
                if (rtree_disconnect(get_tail_server()) == -1) {
                    perror("Error disconnecting the tail server\n");
                }
                if (disconnect_zookeeper() < 0) exit(-1);
                exit(0);
            }
            char* head_path = children_list->data[0];
            char* tail_path = children_list->data[0];
            for (int i = 0; i < children_list->count;i++) {
                char* curr_node_path = children_list->data[i];
                if (strcmp(curr_node_path,head_path) < 0) {
                    head_path = curr_node_path;
                }
                else if (strcmp(curr_node_path,tail_path) > 0) {
                    tail_path = curr_node_path;
                }
            }
            if (strcmp(head_path,head->path) != 0) {
                memcpy(head->path, head_path, CHILD_NODE_PATH_LEN);
                int head_buffer_len = 22;
                char* head_buffer = malloc(head_buffer_len);
                memset(head_buffer, 0, head_buffer_len);
                if (head_buffer == NULL) {
                    free(children_list);
                    return;
                }
                char selected_node_path[head_buffer_len];
                sprintf(selected_node_path,"%s/%s",CHAIN_NODE,head->path);
                if (zoo_get(zh,selected_node_path,0,head_buffer,&head_buffer_len,NULL) != ZOK) {
                    free(children_list);
                    free(head_buffer);
                    return;
                }
                update_head(head_buffer);
                free(head_buffer);
            }
            else if (strcmp(tail_path,tail->path) != 0) {
                memcpy(tail->path, tail_path, CHILD_NODE_PATH_LEN);
                int tail_buffer_len = 22;
                char* tail_buffer = malloc(tail_buffer_len);
                if (tail_buffer == NULL) {
                    free(children_list);
                    return;
                }
                char selected_node_path[tail_buffer_len];
                sprintf(selected_node_path,"%s/%s",CHAIN_NODE,tail->path);
                if (zoo_get(zh,selected_node_path,0,tail_buffer,&tail_buffer_len,NULL) != ZOK) {
                    free(children_list);
                    free(tail_buffer);
                    return;
                }
                update_tail(tail_buffer);
                free(tail_buffer);
            }
        }
    }
    free(children_list);
}

int connect_zookeper(char* zookeeper_addr_port) {
    if (zookeeper_addr_port == NULL) return -1;
    zh = zookeeper_init(zookeeper_addr_port,NULL,2000,0,NULL,0);
    if (zh == NULL) return -1;
    return 0;
}

int get_head_tail_servers() {
    zoo_string* children_list = malloc(sizeof(zoo_string));
    if (children_list == NULL) return -1;
    static char *watcher_ctx = "ZooKeeper Data Watcher";
    if (zoo_wget_children(zh, CHAIN_NODE, children_watcher_client, watcher_ctx, children_list) != ZOK) {
        return -1;
    }
    char* head_path = children_list->data[0];
    char* tail_path = children_list->data[0];
    for (int i = 0; i < children_list->count;i++) {
        char* curr_node_path = children_list->data[i];
        if (strcmp(curr_node_path,head_path) < 0) {
            head_path = curr_node_path;
        }
        else if (strcmp(curr_node_path,tail_path) > 0) {
            tail_path = curr_node_path;
        }
    }
    // printf("GET HEAD TAIL SERVERS\n");
    int head_buffer_len = 22; //IP:PORTO
    char* head_buffer = malloc(head_buffer_len);
    if (head_buffer == NULL) {
        free(children_list);
        return -1;
    }
    memset(head_buffer, 0, head_buffer_len);
    char selected_node_path[head_buffer_len];
    sprintf(selected_node_path,"%s/%s",CHAIN_NODE,head_path);
    if (zoo_get(zh,selected_node_path,0,head_buffer,&head_buffer_len,NULL) != ZOK) {
        free(children_list);
        free(head_buffer);
        return -1;
    }
    printf("\n");
    printf("HEAD SERVER:  %s\n", head_buffer);
    printf("\n");
    int tail_buffer_len = 22;//IP:PORTO
    char* tail_buffer = malloc(tail_buffer_len);
    if (tail_buffer == NULL) {
        free(children_list);
        free(head_buffer);
        return -1;
    }
    memset(tail_buffer, 0, tail_buffer_len);
    sprintf(selected_node_path,"%s/%s",CHAIN_NODE,tail_path);
    if (zoo_get(zh,selected_node_path,0,tail_buffer,&tail_buffer_len,NULL) != ZOK) {
        free(children_list);
        free(head_buffer);
        free(tail_buffer);
        return -1;
    }
    printf("\n");
    printf("TAIL SERVER:  %s\n", tail_buffer);
    printf("\n");
    head = rtree_connect(head_buffer);
    tail = rtree_connect(tail_buffer);
    head->path = malloc(CHILD_NODE_PATH_LEN);
    tail->path = malloc(CHILD_NODE_PATH_LEN);
    memcpy(head->path, head_path, CHILD_NODE_PATH_LEN);
    memcpy(tail->path, tail_path, CHILD_NODE_PATH_LEN);
    free(head_buffer);
    free(tail_buffer);
    free(children_list);
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
        free(rtree->path);
        free(rtree);
        return -1; 
    }
    free(rtree->path);
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
    int op_n = rcv_msg->num;
    message_t__free_unpacked(rcv_msg, NULL);
    return op_n;
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
    
    int op_n = rcv_msg->num;
    message_t__free_unpacked(rcv_msg, NULL);
    return op_n;
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
