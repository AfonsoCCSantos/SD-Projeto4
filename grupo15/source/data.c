/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378;
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"

struct data_t *data_create(int size) {
    if(size <= 0) return NULL;
    
    struct data_t* data_struct = malloc(sizeof(struct data_t));
    if (data_struct == NULL) return NULL;
    
    data_struct->data = malloc(size);
    
    if (data_struct->data == NULL) {
        free(data_struct);
        return NULL;
    }

    data_struct->datasize = size;
    return data_struct;
}

struct data_t *data_create2(int size, void *data) {
    if(size <= 0 || data == NULL) return NULL;

    struct data_t* data_struct = malloc(sizeof(struct data_t));

    if (data_struct == NULL) return NULL;

    data_struct->data = data;
    data_struct->datasize = size;
    return data_struct;
}

void data_destroy(struct data_t *data) {
    if (data == NULL) return;
    free(data->data);
    free(data);
}

struct data_t *data_dup(struct data_t *data) {
    if(data == NULL || data->data == NULL || data->datasize <= 0) return NULL;
    
    struct data_t* data_dup = malloc(sizeof(struct data_t));
    if(data_dup == NULL) return NULL;

    data_dup->data = malloc(data->datasize);   
    if(data_dup->data == NULL) {
        free(data_dup);
        return NULL;
    }
    
    data_dup->datasize = data->datasize;
    memcpy(data_dup->data, data->data, data->datasize);
    return data_dup;
}

void data_replace(struct data_t *data, int new_size, void *new_data) {
    if (data == NULL || new_size <= 0 || new_data == NULL) return;
    free(data->data);
    data->datasize = new_size;
    data->data = new_data;
}