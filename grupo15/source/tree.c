/*
* Grupo n: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "tree-private.h"
#include "entry.h"


struct tree_t *tree_create() {
    struct tree_t* tree = malloc(sizeof(struct tree_t));
    if (tree == NULL) return NULL;
    
    tree->entry = NULL;
    tree->left = NULL;
    tree->right = NULL;
    return tree;
}

void tree_destroy(struct tree_t *tree) {
    if (tree == NULL) return;
    tree_destroy(tree->left);
    tree_destroy(tree->right);
    node_destroy(tree);
}

int tree_put(struct tree_t *tree, char *key, struct data_t *value) {
    if (key == NULL || value == NULL || tree == NULL) return -1;
    char* key_cpy = malloc(strlen(key)+1);
    struct data_t* value_cpy = data_dup(value);
    if (key_cpy == NULL || value_cpy == NULL) {
        free(key_cpy);
        data_destroy(value_cpy);
        return -1;
    }
    strcpy(key_cpy,key);
    struct entry_t* new_entry = entry_create(key_cpy,value_cpy);
    if (new_entry == NULL) {
        free(key_cpy);
        data_destroy(value_cpy);
        return -1;
    }
    return tree_put_aux(tree,new_entry) == NULL ? -1 : 0;
}

struct data_t *tree_get(struct tree_t *tree, char *key) {
    if (tree == NULL || tree->entry == NULL) return NULL;
    if (strcmp(key,tree->entry->key) < 0) return tree_get(tree->left,key);
    if (strcmp(key,tree->entry->key) > 0) return tree_get(tree->right,key);
    else return data_dup(tree->entry->value);
}

struct tree_t* tree_del_aux( struct tree_t *curr, char *key, int* flag) {
    if (curr == NULL || curr->entry == NULL) return NULL;
    
    if (strcmp(key,curr->entry->key) < 0) curr->left = tree_del_aux(curr->left, key, flag);
    else if (strcmp(key,curr->entry->key) > 0) curr->right = tree_del_aux(curr->right, key, flag);
    else { //found to delete
        if (curr->right == NULL && curr->left == NULL) { //No children
            node_destroy(curr);
            *flag = 0;
            return NULL; 
        }
        
        //One child
        else if (curr->right == NULL) {
            entry_replace(curr->entry, curr->left->entry->key, curr->left->entry->value);
            struct tree_t* temp = curr->left;
            curr->right = curr->left->right;
            curr->left = curr->left->left;
            free(temp->entry);
            free(temp);
            *flag = 0;
            return curr;
        }
        else if (curr->left == NULL) {
            entry_replace(curr->entry, curr->right->entry->key, curr->right->entry->value);
            struct tree_t* temp = curr->right;
            curr->left = curr->right->left;
            curr->right = curr->right->right;
            free(temp->entry);
            free(temp);
            *flag = 0;
            return curr;
        }

        //Two children
        else {
            if (curr->left->right == NULL) {
                entry_replace(curr->entry, curr->left->entry->key, curr->left->entry->value);
                struct tree_t* temp = curr->left;
                curr->left = curr->left->left;
                free(temp->entry);
                free(temp);
                *flag = 0;
                return curr;
            }
            else {
                struct entry_t* entry = find_and_remove_largest_child(curr->left);
                entry_replace(curr->entry, entry->key, entry->value);
                *flag = 0;
                free(entry);
                return curr;
            }
        } 
    }
    return curr;
}

int tree_del(struct tree_t *tree, char *key) {
    if (tree == NULL || tree->entry == NULL) return -1;
    if(strcmp(tree->entry->key,key) == 0 && tree->right == NULL && tree->left == NULL) {
        entry_destroy(tree->entry);
        tree->entry = NULL;
        return 0;
    }
    int flag = -1;
    tree_del_aux(tree, key, &flag);
    return flag;
}

int tree_size(struct tree_t *tree) {
    if (tree == NULL || tree->entry == NULL) return 0;
    return 1 + tree_size(tree->left) + tree_size(tree->right);
}

int tree_height(struct tree_t *tree) {
    if (tree == NULL || tree->entry == NULL) return 0;
    int height_left = tree_height(tree->left); 
    int height_right = tree_height(tree->right); 
    return 1 + (height_left < height_right ? height_right : height_left);
}

char **tree_get_keys(struct tree_t *tree) {
    if (tree == NULL) return NULL; 
    if (tree->entry == NULL) {
        char** res = malloc(sizeof(char*));
        res[0] = NULL;
        return res;
   }
   int c = 0;
   int size = tree_size(tree);
   char** res = malloc((size+1) * sizeof(char*));
   if (res == NULL) return NULL;
   get_keys_in_order(tree, res, &c);
   res[size] = NULL;
   return res;
} 

void **tree_get_values(struct tree_t *tree) {
    if (tree == NULL) return NULL;
    if(tree->entry == NULL) {
        void** res = malloc(sizeof(void*));
        res[0] = NULL;
        return res;
    }
    int c = 0;
    int size = tree_size(tree);
    void** res = malloc((size+1) * sizeof(void*)); 
    if (res == NULL) return NULL;
    get_values_in_order(tree, res, &c);
    res[size] = NULL;
    return res;
}

void tree_free_keys(char **keys) {
    if(keys == NULL) return;
    for (int i = 0; keys[i] != NULL; i++) {
        free(keys[i]);
    }
    free(keys);
}

void tree_free_values(void **values) {
    if(values == NULL) return;
    for (int i = 0; values[i] != NULL; i++) {
        free(values[i]);
    }
    free(values);
}

struct tree_t* tree_put_aux(struct tree_t* tree, struct entry_t* entry) {  
    if (tree == NULL) { // found the place to place the entry
        tree = tree_create();
        if(tree == NULL) {
            entry_destroy(entry);
            return NULL;
        }    
        tree->entry = entry;
    }    
    else if (tree->entry == NULL ) { //empty tree
        tree->entry = entry;
    }
    else if (entry_compare(entry,tree->entry) < 0 ) { //is smaller
        tree->left = tree_put_aux(tree->left, entry);
    }
    else if (entry_compare(entry,tree->entry) > 0 ) { //is greater
        tree->right = tree_put_aux(tree->right, entry);
    }
    else {
        entry_replace(tree->entry, entry->key, entry->value);
        free(entry);
    }
    return tree;
}

void node_destroy(struct tree_t* tree) {
    entry_destroy(tree->entry);
    free(tree);
}

struct entry_t* find_and_remove_largest_child(struct tree_t* tree) {
    if (tree->right->right == NULL) {
        struct entry_t* entry = tree->right->entry;
        struct tree_t* temp = tree->right;
        tree->right = tree->right->left;
        free(temp);
        return entry;
    }
    else {
        return find_and_remove_largest_child(tree->right);
    }
}

void get_keys_in_order(struct tree_t *tree, char** res, int* pos) {
    if (tree->left != NULL) {
        get_keys_in_order(tree->left, res, pos);
    }
    char* copy = malloc(strlen(tree->entry->key)+1);
    strcpy(copy, tree->entry->key);
    res[(*pos)++] = copy;
    if (tree->right != NULL) {
        get_keys_in_order(tree->right, res, pos);
    }
}

void get_values_in_order(struct tree_t *tree, void** res, int* pos) {
    if (tree->left != NULL) {
        get_values_in_order(tree->left, res, pos);
    }
    void* value = malloc(sizeof(struct data_t)); 
    memcpy(value, tree->entry->value, sizeof(struct data_t));
    res[(*pos)++] = value;
    if (tree->right != NULL) {
        get_values_in_order(tree->right, res, pos);
    }
}
