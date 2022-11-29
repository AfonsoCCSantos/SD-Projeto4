/*
* Grupo nº: 15
* Afonso Santos - FC56368
* Alexandre Figueiredo - FC57099
* Raquel Domingos - FC56378
*/
#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"
#include "entry.h"

struct tree_t {
	struct entry_t* entry;
	struct tree_t* left;
	struct tree_t* right;
};

/*
* Funcao auxiliar de tree_del
*/
struct tree_t* tree_del_aux(struct tree_t *curr, char *key, int *flag);

/*
* Funcao auxiliar de tree_put
*/
struct tree_t* tree_put_aux(struct tree_t* tree, struct entry_t* entry);

/*
* Destroi a entry e o proprio no, mantendo os seus filhos.
*/
void node_destroy(struct tree_t* tree);

/*
* Devolve e remove o maior no da arvore passada
*/
struct entry_t* find_and_remove_largest_child(struct tree_t* tree);

/*
* Preenche um char** com as keys em ordem 
*/
void get_keys_in_order(struct tree_t *tree, char** res, int* pos);

/*
* Preenche um void** com as values em ordem 
*/
void get_values_in_order(struct tree_t *tree, void** res, int* pos);

#endif
