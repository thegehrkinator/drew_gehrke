/*
 * This file is where you should implement your binary search tree.  It already
 * contains skeletons of the functions you need to implement (along with
 * documentation for each function).  Feel free to implement any additional
 * functions you might need.  Also, don't forget to include your name and
 * @oregonstate.edu email address below.
 *
 * Name: Drew Gehrke
 * Email: gehrkea@oregonstate.edu
 */

#include <stdlib.h>
#include <stdio.h>

#include "bst.h"

/*
 * This structure represents a single node in a BST.  In addition to containing
 * pointers to its two child nodes (i.e. `left` and `right`), it contains two
 * fields representing the data stored at this node.  The `key` field is an
 * integer value that should be used as an identifier for the data in this
 * node.  Nodes in the BST should be ordered based on this `key` field.  The
 * `value` field stores data associated with the key.
 *
 * You should not modify this structure.
 */
struct bst_node {
  int key;
  void* value;
  struct bst_node* left;
  struct bst_node* right;
};


/*
 * This structure represents an entire BST.  It specifically contains a
 * reference to the root node of the tree.
 *
 * You should not modify this structure.
 */
struct bst {
  struct bst_node* root;
};

/*
 * This function should allocate and initialize a new, empty, BST and return
 * a pointer to it.
 */
struct bst* bst_create() {
	struct bst* new_bst = malloc(sizeof(struct bst));
	new_bst->root = NULL;
	return new_bst;
}

void bst_freeNode(struct bst_node* temp) {
	if (temp->right != NULL) {
		bst_freeNode(temp->right);
	}
	if (temp->left != NULL) {
		bst_freeNode(temp->left);
	}
	free(temp);
}

/*
 * This function should free the memory associated with a BST.  While this
 * function should up all memory used in the BST itself, it should not free
 * any memory allocated to the pointer values stored in the BST.  This is the
 * responsibility of the caller.
 *
 * Params:
 *   bst - the BST to be destroyed.  May not be NULL.
 */
void bst_free(struct bst* bst) {
	bst_freeNode(bst->root);
	free(bst);
	return;
}

/*
 * This function will traverse the array and return the total number of elements it finds.
 *
 * Params:
 *   bst_node: The node that the function is currently looking over.
 */
int bst_traversal(struct bst_node* N, int* count) {
	if (N != NULL) {
		(*count)++;
		bst_traversal(N->left, count);
		bst_traversal(N->right, count);
	}
}

/*
 * This function should return the total number of elements stored in a given
 * BST.
 *
 * Params:
 *   bst - the BST whose elements are to be counted.  May not be NULL.
 */
int bst_size(struct bst* bst) {
	int count = 0;
	int *count_ptr = &count;

	bst_traversal(bst->root, count_ptr);
	return count;
}

/*
 * This function should insert a new key/value pair into the BST.  The key
 * should be used to order the key/value pair with respect to the other data
 * stored in the BST.  The value should be stored along with the key, once the
 * right location in the tree is found.
 *
 * Params:
 *   bst - the BST into which a new key/value pair is to be inserted.  May not
 *     be NULL.
 *   key - an integer value that should be used to order the key/value pair
 *     being inserted with respect to the other data in the BST.
 *   value - the value being inserted into the BST.  This should be stored in
 *     the BST alongside the key.  Note that this parameter has type void*,
 *     which means that a pointer of any type can be passed.
 */
void bst_insert(struct bst* bst, int key, void* value) {
	struct bst_node* new_node = malloc(sizeof(struct bst_node));
	struct bst_node* temp_prev = NULL;
	struct bst_node* temp_next = bst->root;	

	new_node->key = key;
	new_node->value = value;
	new_node->left = NULL;
	new_node->right = NULL;
	

	if (bst->root == NULL) {
		bst->root = new_node;
	}
	else {
		while (temp_next != NULL) {
			temp_prev = temp_next;
			if (key < temp_next->key) {
				temp_next = temp_next->left;
			}
			else {
				temp_next = temp_next->right;
			}
		}
		if (key < temp_prev->key) {
			temp_prev->left = new_node;
		}
		else {
			temp_prev->right = new_node;
		}
	}
	return;
}

/*
 * This function finds out what to do after the node is removed in order to keep
 * the BST properties. Will rearrange the tree if needed upon removing the node.
 *
 * Params:
 *  bst - the bst which is being used.
 *  temp - the temp node from the remove function; the node being removed.
 *  temp_p - the temp parent of the node being removed from the tree.
 */
void bst_removal(struct bst* bst, struct bst_node* temp, struct bst_node* temp_p, int key) {
	if (temp->right == NULL && temp->left == NULL) {
		if (temp_p->right->key == key) {
			temp_p->right = NULL;
		}
		else if (temp_p->left->key == key) {
			temp_p->left = NULL;
		}
	}
	else if (temp->left == NULL || temp->right == NULL) {
		if (temp_p->left->key == key) {
			if (temp->left == NULL) {
				temp_p->left = temp->right;
			}
			else {
				temp_p->left = temp->left;
			}
		}
		else if (temp_p->right->key == key) {
			if (temp->left == NULL) {
				temp_p->right = temp->right;
			}
			else {
				temp_p->right = temp->left;
			}
		}
	}
	else if (temp->left != NULL && temp->right != NULL) {
		struct bst_node* temp_ps;
		struct bst_node* ios = temp->right;

		while (ios->left != NULL) {
			temp_ps = ios;
			ios = ios->left;
		}
		ios->left = temp->left;
		if (ios != temp->right) {
			temp_ps->left = ios->right;
			ios->right = temp->right;
		}

		if (temp == bst->root) {
			bst->root = ios;
		}
		else if (temp_p->right->key == key) {
			temp_p->right = ios;
		}
		else {
			temp_p->left = ios;
		}
	}
}

/*
 * This function should remove a key/value pair with a specified key from a
 * given BST.  If multiple values with the same key exist in the tree, this
 * function should remove the first one it encounters (i.e. the one closest to
 * the root of the tree).
 *
 * Params:
 *   bst - the BST from which a key/value pair is to be removed.  May not
 *     be NULL.
 *   key - the key of the key/value pair to be removed from the BST.
 */
void bst_remove(struct bst* bst, int key) {
	struct bst_node* temp = bst->root;
	struct bst_node* temp_p = NULL;

	while (temp != NULL) {

		if (temp->key == key) {
			bst_removal(bst, temp, temp_p, key);
			free(temp);
			temp = NULL;
		}
		else if (key < temp->key) {
			temp_p = temp;
			temp = temp->left;			
		}
		else {
			temp_p = temp;
			temp = temp->right;
		}
	}
}

/*
 * This function should return the value associated with a specified key in a
 * given BST.  If multiple values with the same key exist in the tree, this
 * function should return the first one it encounters (i.e. the one closest to
 * the root of the tree).  If the BST does not contain the specified key, this
 * function should return NULL.
 *
 * Params:
 *   bst - the BST from which a key/value pair is to be removed.  May not
 *     be NULL.
 *   key - the key of the key/value pair whose value is to be returned.
 *
 * Return:
 *   Should return the value associated with the key `key` in `bst` or NULL,
 *   if the key `key` was not found in `bst`.
 */
void* bst_get(struct bst* bst, int key) {
	struct bst_node* temp = bst->root;

	while (temp != NULL) {
		if (temp->key == key) {
			return temp->value;
		}
		else if (key < temp->key) {
			temp = temp->left;
		}
		else {
			temp = temp->right;
		}
	}
	return NULL;
}