/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <zookeeper/zookeeper.h>
#include "tree.h"
#include "entry.h"
#include "sdmessage.pb-c.h"
#include "tree_skel.h"
#include "tree_skel-private.h"
#include "network_server.h"
#include "client_stub.h"
#include "client_stub-private.h"

#define CHAIN_NODE "/chain"
#define CHILD_NODE_PATH "/chain/node"
#define CHILD_NODE_PATH_LEN 12

typedef struct String_vector zoo_string;

struct tree_t* tree;
struct op_proc* op_proc;
struct request_t* queue_head;
int last_assigned = 1;
int terminate = 0;
int* thread_counter;
pthread_t* thread_ids;
pthread_mutex_t request_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t max_proc_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t tree_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
static zhandle_t* zh;
struct rtree_t* next_server = NULL;
int zid;

int update_new_server() {
    char** keys = tree_get_keys(tree);
    if (keys == NULL) return -1;
    for (int i = 0; keys[i] != NULL; i++) {
        struct data_t* data = tree_get(tree, keys[i]);
        if (data == NULL) {
            tree_free_keys(keys);
            return -1;
        }
        struct entry_t* entry = entry_create(keys[i], data);
        if (entry == NULL) {
            tree_free_keys(keys);
            return -1;
        }
        if (rtree_put(next_server, entry) < 0) {
            free(entry);
            tree_free_keys(keys);
            return -1; 
        }
    }
    tree_free_keys(keys);
    return 0;
}

int get_zookeeper_id(char* node_path) {
    int i = CHILD_NODE_PATH_LEN;
    while (node_path[i++] == 0);
    return atoi(node_path+i);
}

static void children_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
    zoo_string* children_list =	malloc(sizeof(zoo_string));
    if (children_list == NULL) return;
    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_CHILD_EVENT) {
            if (zoo_wget_children(zh, CHAIN_NODE, children_watcher, watcher_ctx, children_list) != ZOK) {
                return;
            }
            int current_id_selected = zid;
            int current_selected_index = 0;
            for (int i = 0; i < children_list->count;i++) {
                int curr_node_id = get_zookeeper_id(children_list->data[i]);
                if (curr_node_id > zid && (curr_node_id < current_id_selected || current_id_selected == zid)) {
                    current_id_selected = curr_node_id;
                    current_selected_index = i;
                }
            }
            if (current_id_selected == zid) { //Caso em que o servidor tem id mais alto
                if (next_server != NULL) free(next_server);
                next_server = NULL;
                free(children_list);
                return;
            }
            
            if (next_server != NULL && next_server->path != NULL && 
                strcmp(next_server->path,children_list->data[current_selected_index]) == 0) {
                //Caso em que servidor ja esta conectado ao servidor com proximo id mais alto
                return;
            }           
            
            int buffer_next_server_len = CHILD_NODE_PATH_LEN + 10;
            char* buffer_next_server = malloc(buffer_next_server_len);
            if (buffer_next_server == NULL) {
                free(children_list);
                return;
            } 
            char selected_node_path[buffer_next_server_len];
            sprintf(selected_node_path,"%s/%s",CHAIN_NODE,children_list->data[current_selected_index]);
            if (zoo_get(zh,selected_node_path,0,buffer_next_server,&buffer_next_server_len,NULL) != ZOK) {
                free(children_list);
                free(buffer_next_server);
                return;
            }
            
            if (next_server != NULL) {
                free(next_server->path);
                free(next_server);
                //rtree_disconnect(next_server); //Se foi o next server que saiu, nao pode n ser preciso dar disconnect
            }

            printf("\n");
            printf("O meu next server vai ser: %s\n",buffer_next_server);
            printf("\n");
            
            next_server = rtree_connect(buffer_next_server);
            if (next_server == NULL) {
                free(children_list);
                free(buffer_next_server);
                return;
            }

            int path_len = strlen(children_list->data[current_selected_index]);
            next_server->path = malloc(path_len);
            memcpy(next_server->path,children_list->data[current_selected_index],path_len);
            free(buffer_next_server);
        }
    }
    free(children_list);
    return;
}

int connect_zookeeper(char* zookeeper_addr_port, char* server_addr_port) {
    if (zookeeper_addr_port == NULL) return -1;
    zh = zookeeper_init(zookeeper_addr_port,NULL,2000,0,NULL,0);
    if (zh == NULL) return -1;
    if (create_znode(server_addr_port) == -1) {
        zookeeper_close(zh);
        return -1;
    }
    zoo_string* children_list = malloc(sizeof(zoo_string));
    if (children_list == NULL) {
        zookeeper_close(zh);
        return -1;
    }
    static char *watcher_ctx = "ZooKeeper Data Watcher";
    if (zoo_wget_children(zh, CHAIN_NODE, children_watcher, watcher_ctx, children_list) != ZOK) {
        free(children_list);
        zookeeper_close(zh);
        return -1;
    }
    
    int current_id_selected = zid;
    int current_selected_index = 0;
    for (int i = 0; i < children_list->count;i++) {
        int curr_node_id = get_zookeeper_id(children_list->data[i]);
        if (curr_node_id > zid && curr_node_id < current_id_selected) {
            current_id_selected = curr_node_id;
            current_selected_index = i;
        }
    }
    
    if (current_id_selected == zid) { //Caso em que o servidor tem id mais alto
        next_server = NULL;
        free(children_list);
        return 0;
    }
    
    int buffer_next_server_len = CHILD_NODE_PATH_LEN + 10;
    char* buffer_next_server = malloc(buffer_next_server_len);
    if (buffer_next_server == NULL) {
        zookeeper_close(zh);
        free(children_list);
        return -1;
    } 
    char selected_node_path[buffer_next_server_len];
    sprintf(selected_node_path,"%s/%s",CHAIN_NODE,children_list->data[current_selected_index]);
    if (zoo_get(zh,selected_node_path,0,buffer_next_server,&buffer_next_server_len,NULL) != ZOK) {
        zookeeper_close(zh);
        free(children_list);
        free(buffer_next_server); 
        return -1;
    }
    next_server = rtree_connect(buffer_next_server);
    if (next_server == NULL) {
        zookeeper_close(zh);
        free(children_list);
        free(buffer_next_server);
        return -1;
    }
    int path_len = strlen(children_list->data[current_selected_index]);
    next_server->path = malloc(path_len);
    memcpy(next_server->path,children_list->data[current_selected_index],path_len);

    // if (update_new_server() < 0) {
    //     free(buffer_next_server);
    //     free(children_list);
    //     return -1;
    // }
    free(buffer_next_server);
    free(children_list);

    return 0;   
}

int create_znode(char* server_addr_port) {
    if (zoo_exists(zh, CHAIN_NODE, 0, NULL) == ZNONODE) {
        if (zoo_create(zh, CHAIN_NODE, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0) != ZOK) {
            return -1;
        }        
    }
    int node_buffer_len = CHILD_NODE_PATH_LEN + 10;
    char* node_buffer = malloc(node_buffer_len);
    if (node_buffer == NULL) return -1;
    if (zoo_create(zh, CHILD_NODE_PATH, server_addr_port, strlen(server_addr_port), 
        &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, node_buffer, node_buffer_len) != ZOK) {
            free(node_buffer);
            return -1;
    }
    zid = get_zookeeper_id(node_buffer);
    return 0;
}

int tree_skel_init(int N) {
    op_proc = malloc(sizeof(struct op_proc));
    if (op_proc == NULL) {
        return -1;
    }
    int size = sizeof(int) * N;
    op_proc->in_progress = malloc(size);
    if (op_proc->in_progress == NULL) {
        free(op_proc);
        return -1;
    }
    memset(op_proc->in_progress,0,size);
    op_proc->n_threads = N;
    op_proc->max_proc = 0;
    tree = tree_create();
    
    thread_ids = malloc(sizeof(pthread_t) * N);
    thread_counter = malloc(sizeof(int) * N);
    if (tree == NULL || thread_ids == NULL || thread_counter == NULL) {
        free(op_proc->in_progress);
        free(op_proc);
        tree_destroy(tree);
        free(thread_ids);
        free(thread_counter);
        return -1;
    }
    for (int i = 0; i < N; i++) {
        thread_counter[i] = i;
        pthread_create(&thread_ids[i],NULL,process_request,&thread_counter[i]);
    }


    return 0;
}

void tree_skel_destroy() {
    tree_destroy(tree);
    free(thread_ids);
    free(thread_counter);
    pthread_mutex_destroy(&request_queue_mutex);
    pthread_mutex_destroy(&max_proc_mutex);
    pthread_mutex_destroy(&tree_mutex);
    pthread_cond_destroy(&cond);
    free(op_proc->in_progress);
    free(op_proc);
    free_queue();
    zookeeper_close(zh);
}

int invoke(MessageT *msg) {
    if (msg == NULL || msg->opcode == MESSAGE_T__OPCODE__OP_ERROR || tree == NULL) return -1;
    
    MessageT__Opcode opcode = msg->opcode;
    
    if (opcode == MESSAGE_T__OPCODE__OP_SIZE) {
        msg->opcode = MESSAGE_T__OPCODE__OP_SIZE+1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        pthread_mutex_lock(&tree_mutex);
        msg->num = tree_size(tree);
        pthread_mutex_unlock(&tree_mutex);
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_HEIGHT) {
        msg->opcode = MESSAGE_T__OPCODE__OP_HEIGHT+1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        pthread_mutex_lock(&tree_mutex);
        msg->num = tree_height(tree);
        pthread_mutex_unlock(&tree_mutex);
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_DEL) {
        struct request_t* request = malloc(sizeof(struct request_t));
        if (request == NULL) return -1;
        msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->num = last_assigned;
        request->op_n = last_assigned++;
        request->op = 0;
        request->next = NULL;
        request->key = malloc(msg->data[0].len);
        if (request->key == NULL) {
            free(request);
            return -1;
        }
        memcpy(request->key, msg->data[0].data, msg->data[0].len);
        request->data = NULL;
        pthread_mutex_lock(&request_queue_mutex);
        add_to_queue(request);
        pthread_mutex_unlock(&request_queue_mutex);
        pthread_cond_broadcast(&cond);
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_GET) {
        char* key = malloc(msg->data[0].len);
        if (key == NULL) return -1;
        memcpy(key, msg->data[0].data, msg->data[0].len); 
        
        pthread_mutex_lock(&tree_mutex);
        struct data_t* res = tree_get(tree,key);
        pthread_mutex_unlock(&tree_mutex);
        free(key);
        if (res == NULL) {
            res = malloc(sizeof(struct data_t));
            if (res == NULL) {
               free(msg->data[0].data);
               free(msg->data); 
               return -1;
            }
            res->datasize = 0;
            res->data = NULL; 
        }
        msg->opcode = MESSAGE_T__OPCODE__OP_GET+1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
        
        msg->n_data = 1;
        free(msg->data[0].data);
        free(msg->data);
        msg->data = malloc(sizeof(ProtobufCBinaryData));
        if(msg->data == NULL) {
            data_destroy(res);
            return -1;
        }
        msg->data[0].len = res->datasize;
        msg->data[0].data = malloc(res->datasize);
        if (msg->data[0].data == NULL) {
            data_destroy(res);
            free(msg->data);
            return -1;
        }
        memcpy(msg->data[0].data,res->data,res->datasize);
        data_destroy(res);
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_PUT) {
        struct request_t* request = malloc(sizeof(struct request_t));
        if (request == NULL) return -1;
        msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->num = last_assigned;
        request->op_n = last_assigned++;
        request->op = 1;
        request->next = NULL;
        request->key = malloc(strlen(msg->entry->key) + 1);
        if (request->key == NULL) {
            free(request); 
            return -1;
        }
        strcpy(request->key, msg->entry->key);
        char* data = malloc(msg->entry->data.len);
        if (data == NULL) {
            free(request->key);
            free(request);
            return -1;
        }
        memcpy(data, msg->entry->data.data, msg->entry->data.len); 
        request->data = data_create2(msg->entry->data.len,data);
        if (request->data == NULL) {
            free(request->key);
            free(request);
            free(data);
            return -1; 
        } 
        pthread_mutex_lock(&request_queue_mutex);
        add_to_queue(request);
        pthread_mutex_unlock(&request_queue_mutex);
        pthread_cond_broadcast(&cond);
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_GETKEYS) {
        pthread_mutex_lock(&tree_mutex);
        char** res = tree_get_keys(tree);
        pthread_mutex_unlock(&tree_mutex);
        if (res == NULL) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        }
        else {
            msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS+1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
            
            int n_keys;
            for (n_keys = 0; res[n_keys] != NULL; n_keys++); 
            
            msg->n_data = n_keys;
            msg->data = malloc(sizeof(ProtobufCBinaryData) * n_keys);
            if (msg->data == NULL) return -1;
            for (int i = 0; i < n_keys; i++) {
                msg->data[i].len = strlen(res[i]) + 1;
                msg->data[i].data = malloc(msg->data[i].len);
                if (msg->data[i].data == NULL) {
                    for (int j = 0; j < i; j++) {
                        free(msg->data[j].data);
                    }
                    free(msg->data);
                    return -1;
                }
                memcpy(msg->data[i].data,res[i],msg->data[i].len);
            }
            tree_free_keys(res);
        }
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_GETVALUES) {
        pthread_mutex_lock(&tree_mutex);
        void** res = tree_get_values(tree);
        pthread_mutex_unlock(&tree_mutex);
        if (res == NULL) {
            msg->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        } 
        else {
            msg->opcode = MESSAGE_T__OPCODE__OP_GETVALUES+1;
            msg->c_type = MESSAGE_T__C_TYPE__CT_VALUES;
            
            int n_values;
            for (n_values = 0; res[n_values] != NULL; n_values++); 
            
            msg->n_data = n_values;
            msg->data = malloc(sizeof(ProtobufCBinaryData) * n_values);
            if (msg->data == NULL) return -1;
            for (int i = 0; i < n_values; i++) {
                msg->data[i].len = ((struct data_t*) res[i])->datasize;
                msg->data[i].data = malloc(msg->data[i].len);
                if (msg->data[i].data == NULL) {
                    for (int j = 0; j < i; j++) {
                        free(msg->data[j].data);
                    }
                    free(msg->data);
                    return -1;
                }
                memcpy(msg->data[i].data, ((struct data_t*) res[i])->data, msg->data[i].len);
            }
            tree_free_values(res);  
        }
    }
    else if (opcode == MESSAGE_T__OPCODE__OP_VERIFY) {
        int op = msg->num;
        msg->opcode = MESSAGE_T__OPCODE__OP_VERIFY+1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->num = verify(op);
    }
    return 0;
}

int verify(int op_n) {
    if (op_n <= 0 || op_proc->max_proc < op_n ) return 0;
    if (op_proc->max_proc == op_n) return 1;

    // Verificar se esta na lista
    for (int i = 0; i < op_proc->n_threads; i++) {
        if (op_proc->in_progress[i] == op_n) return 0;
    }
    
    return 1;
}

void *process_request (void *params) {
    while(1) {
        // Buscar o primeiro request da lista
        pthread_mutex_lock(&request_queue_mutex);
        while (queue_head == NULL) {
            pthread_cond_wait(&cond,&request_queue_mutex);
            if (terminate == 1) {
                pthread_mutex_unlock(&request_queue_mutex);
                pthread_exit(0);
            }
        }
        struct request_t* request = queue_pop();
        pthread_mutex_unlock(&request_queue_mutex);
        op_proc->in_progress[*((int*) params)] = request->op_n;
        
        if (request->op == 0) {
            pthread_mutex_lock(&tree_mutex);
            tree_del(tree,request->key);
            pthread_mutex_unlock(&tree_mutex);
            if (next_server != NULL) {
                rtree_del(next_server, request->key);
            }
        }
        else {
            pthread_mutex_lock(&tree_mutex);
            tree_put(tree,request->key,request->data);
            pthread_mutex_unlock(&tree_mutex);
            struct entry_t* entry = entry_create(request->key, request->data);
            if (next_server != NULL) {
                if (rtree_put(next_server, entry) < 0) free(entry);   
            }
        }
        pthread_mutex_lock(&max_proc_mutex);
        op_proc->max_proc = request->op_n > op_proc->max_proc ? request->op_n : op_proc->max_proc;  
        pthread_mutex_unlock(&max_proc_mutex);
        op_proc->in_progress[*((int*) params)] = 0;
        data_destroy(request->data);
        free(request->key);
        free(request);
   }
}

void add_to_queue(struct request_t* to_add) {
    if (queue_head == NULL) {
        queue_head = to_add;
    }
    else {
        struct request_t* curr = queue_head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = to_add;
    }
}

struct request_t* queue_pop() {
    struct request_t* old_head = queue_head;
    queue_head = queue_head->next;
    return old_head;
} 

void free_queue() {
    while (queue_head != NULL) {
        struct request_t* old_head = queue_pop();
        data_destroy(old_head->data);
        free(old_head->key);
        free(old_head);  
    }
}

void server_kill() {
    terminate = 1;
    pthread_cond_broadcast(&cond);
    for(int i = 0; i < op_proc->n_threads; i++) {
        pthread_join(thread_ids[i],NULL);
    }
}