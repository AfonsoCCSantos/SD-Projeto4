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
#include <string.h>
#include <signal.h>
#include <poll.h>
#include "network_server.h"
#include "message-private.h"
#include "sdmessage.pb-c.h"
#include "network_server-private.h"

int sockfd;
int nfds = 1;
struct pollfd* connects;

void list_remove(struct pollfd target, int size) {
    struct pollfd* new_p_head = malloc(sizeof(struct pollfd) * (size-1));
    if (new_p_head == NULL) return;
    int count_pos = 0;
    for (int i = 0; i < size ; i++) {
        if (connects[i].fd != target.fd) {
            new_p_head[count_pos++] = connects[i];
        }
    }
    free(connects);
    connects = new_p_head;
}

int network_server_init(short port) {
    struct sockaddr_in server;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creating socket\n");
        return -1;
    }
    
    if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) < 0) {
        perror("Error setsockopt\n");
        close(sockfd);
        return -1;
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) { 
        perror("Error while binding\n");
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 0) < 0) {
        close(sockfd);
        return -1;     
    } 

    return sockfd;
}

int network_main_loop(int listening_socket) {
    connects = malloc(sizeof(struct pollfd));
    if (connects == NULL) return -1;
    connects[0].fd = listening_socket;
    connects[0].events = POLLIN;
    connects[0].revents = 0;
    
    while (poll(connects, nfds, -1) >= 0) {
        if (connects[0].revents & POLLIN) {
            struct sockaddr_in client;
            socklen_t size_client = 0;
            int connsockfd;
            if ((connsockfd = accept(listening_socket, (struct sockaddr*) &client, &size_client)) != -1) {
                nfds++;
                connects = realloc(connects, sizeof(struct pollfd) * nfds);
                if (connects == NULL) return -1;
                connects[nfds-1].fd = connsockfd;
                connects[nfds-1].events = POLLIN;
                connects[nfds-1].revents = 0;
            }
        }
        for (int i = 1; i < nfds; i++) {
            struct pollfd curr_pfd = connects[i];
            if (curr_pfd.revents & POLLIN) {
                MessageT* msg = network_receive(curr_pfd.fd);
                if (msg == NULL) {
                    close(curr_pfd.fd);
                    list_remove(curr_pfd,nfds);
                    nfds--;
                }
                else {
                    if (invoke(msg) < 0) {
                        close(curr_pfd.fd);
                        list_remove(curr_pfd,nfds);
                        nfds--;
                    }
                    else {
                        network_send(curr_pfd.fd, msg);
                    }   
                }
            }
            if (curr_pfd.events & POLL_ERR || curr_pfd.events & POLL_HUP) { 
                close(curr_pfd.fd);
                list_remove(curr_pfd,nfds);
                nfds--;
            }
        }
    }
    return 0;
}

MessageT *network_receive(int client_socket) {
    MessageT* rcv_msg;
    char* buf_size = malloc(sizeof(int));
    if (buf_size == NULL) return NULL;
    if (read_all(client_socket, buf_size, sizeof(int)) != sizeof(int)) {
        free(buf_size);
        return NULL;
    }    
    int* size = malloc(sizeof(int));
    if (size == NULL) {
        free(buf_size);
        return NULL;
    }
    memcpy(size, buf_size, sizeof(int));
    free(buf_size);
    char* buf = malloc(*size); 
    if (buf == NULL) {
        free(size);
        return NULL;
    }
    if (read_all(client_socket,  buf, *size) != *size) {
        free(size);
        free(buf);
        return NULL;
    }
    
    rcv_msg = message_t__unpack(NULL, *size, (uint8_t*) buf);
    if (rcv_msg == NULL) {
        perror("error unpacking message\n");
    }
    free(size); 
    free(buf);
    return rcv_msg;
}

int network_send(int client_socket, MessageT *msg) {
    int len = message_t__get_packed_size(msg);
    if (len <= 0) {
        message_t__free_unpacked(msg, NULL);
        return -1;
    } 
    char* buffer = malloc(sizeof(int) + len);
    if (buffer == NULL) {
        message_t__free_unpacked(msg, NULL);
        return -1;
    }
    memcpy(buffer,&len,sizeof(int));
    message_t__pack(msg,(uint8_t*) buffer + sizeof(int));
    
    if (write_all(client_socket,buffer,sizeof(int) + len) != (sizeof(int) + len)) {
        free(buffer);
        message_t__free_unpacked(msg, NULL);
        return -1;
    }
    free(buffer);

    message_t__free_unpacked(msg, NULL);
    return 0;
}

int network_server_close() {
    for (int i = 0; i < nfds; i++) {
        close(connects[i].fd);
    }
    free(connects);
    if (close(sockfd) < 0) return -1;
    return 0;
}