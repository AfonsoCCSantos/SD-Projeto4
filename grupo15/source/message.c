/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "message-private.h"

int write_all(int sock, char* buf, int len) {
    int bufsize = len;
    while (len > 0) {
        int res = write(sock, buf, len);
        if (res <= 0) {
            if (errno == EINTR) continue;
            printf("Unable to write more data\n");
            return res;
        }
        buf += res;
        len -= res;
    }
    return bufsize;
}

int read_all(int sock, char* buf, int len) {
    int bufsize = len;
    while (len > 0) {
        int res = read(sock, buf, len);
        if (res <= 0) {
            if (errno == EINTR) continue;
            printf("Unable to read more data\n");
            return res;
        }
        buf += res;
        len -= res;
    }
    return bufsize;
}
