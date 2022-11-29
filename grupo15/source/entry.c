/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378
*/
#include <stdio.h>
#include <stdlib.h>
#include "entry.h"
#include "data.h"
#include <string.h>

struct entry_t *entry_create(char *key, struct data_t *data) {
    if (key == NULL || data == NULL) return NULL;
    struct entry_t* entry = malloc(sizeof(struct entry_t));
    if (entry == NULL) return NULL;
    entry->key = key;
    entry->value = data;
    return entry;
}

void entry_destroy(struct entry_t *entry) {
    if (entry == NULL) return;
    data_destroy(entry->value);
    free(entry->key);
    free(entry);
}

struct entry_t *entry_dup(struct entry_t *entry){
    if (entry == NULL || entry->key == NULL) return NULL;
    struct entry_t* entry_dup = malloc(sizeof(struct entry_t));
    if (entry_dup == NULL) return NULL;
    entry_dup->key = malloc(strlen(entry->key)+1);
    entry_dup->value = data_dup(entry->value);
    if (entry_dup->key == NULL || entry_dup->value == NULL) {
        entry_destroy(entry_dup);
        return NULL;
    }
    strcpy(entry_dup->key, entry->key);
    return entry_dup;
}

void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value) {
    if (entry == NULL || new_key == NULL || new_value == NULL) return;
    data_destroy(entry->value);
    free(entry->key);
    entry->key = new_key;
    entry->value = new_value;
}

int entry_compare(struct entry_t *entry1, struct entry_t *entry2){
    int comp_result = strcmp(entry1->key, entry2->key);
    return comp_result > 0 ? 1 : comp_result == 0 ? 0 : -1;
}
