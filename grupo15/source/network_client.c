/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network_client.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "message-private.h"
#include <netinet/in.h>
#include <unistd.h>

int network_connect(struct rtree_t *rtree) {
    int socketfd;

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }
    rtree->socketfd = socketfd;
    
    if (connect(rtree->socketfd, (struct sockaddr*)rtree->address, sizeof(*(rtree->address))) < 0) {
        close(rtree->socketfd);
        return -1;
    }
    return 0;
}

MessageT *network_send_receive(struct rtree_t * rtree, MessageT *msg) {
    int len = message_t__get_packed_size(msg);
    if (len < 0) {
        network_close(rtree);
        return NULL;
    }   
    
    char* buffer = malloc(sizeof(int) + len);
    if (buffer == NULL) {
        network_close(rtree);
        return NULL;
    }  
    memcpy(buffer, &len, sizeof(int));
    message_t__pack(msg, (uint8_t*) buffer + sizeof(int));
    
    if (write_all(rtree->socketfd, buffer, sizeof(int) + len) != (sizeof(int) + len)) {
        free(buffer);
        network_close(rtree);
        return NULL;
    }
    free(buffer);
    
    char* buf_size = malloc(sizeof(int));
    if (buf_size == NULL) {
        network_close(rtree);
        return NULL;
    }
    if (read_all(rtree->socketfd, buf_size, sizeof(int)) != sizeof(int)) {
        free(buf_size);
        network_close(rtree);
        return NULL;
    }

    int *buf_len = malloc(sizeof(int));
    if (buf_len == NULL) {
        free(buf_size);
        network_close(rtree);
        return NULL;
    }
    memcpy(buf_len, buf_size, sizeof(int));
    free(buf_size);
    
    char* rcv_buf = malloc(*buf_len);
    if (rcv_buf == NULL) {
        network_close(rtree);
        free(buf_len);
        return NULL;
    }   
    if (read_all(rtree->socketfd, rcv_buf, *buf_len) != *buf_len) {
        free(rcv_buf);
        free(buf_len);
        network_close(rtree);
        return NULL;
    }
    
    MessageT* rcv_msg = message_t__unpack(NULL, *buf_len, (uint8_t*) rcv_buf);
    free(rcv_buf);
    free(buf_len);
    
    return rcv_msg;
}

int network_close(struct rtree_t * rtree) {
    if (close(rtree->socketfd) < 0) return -1;
    return 0;
}