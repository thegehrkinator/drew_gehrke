/*
 * In this file, you will write the structures and functions needed to
 * implement a priority queue.  Feel free to implement any helper functions
 * you need in this file to implement a priority queue.  Make sure to add your
 * name and @oregonstate.edu email address below:
 *
 * Name: Drew Gehrke
 * Email: gehrkea@oregonstate.edu
 */

#include <stdlib.h>
#include <stdio.h>

#include "pq.h"
#include "dynarray.h"

/*
 * This is the structure that represents a priority queue.  You must define
 * this struct to contain the data needed to implement a priority queue.
 */
struct pq {
	struct dynarray* da;
	struct pq_node* head;
};

struct pq_node {
	void* value;
	int priority;
};

/*void print(struct pq* pq) {
	for (int i = 0; i < dynarray_size(pq->da); i++) {
		struct pq_node* temp = dynarray_get(pq->da, i);

		printf("node_prio: %d\n", temp->priority);
	}
}*/
/*
 * This function will compare the newly inserted node to its parent and 
 * will adjust as needed to keep the heap property in tact.
 *
 * Params:
 *  pq - the priority queue being worked on
 *  idx - the index of the node being checked
 */
void pq_percolateUp(struct pq* pq, int idx) {
	int temp_p;
	struct pq_node* n = dynarray_get(pq->da, idx);
	struct pq_node* m, *temp;

	if (idx != 0) {
		temp_p = ((idx - 1) / 2);
		m = dynarray_get(pq->da, temp_p);
		if (n->priority < m->priority) {
			temp = m;
			dynarray_set(pq->da, temp_p, n);
			dynarray_set(pq->da, idx, temp);
			pq_percolateUp(pq, temp_p);
		}
	}
}

/*
 * This function will compare the root node of a subtree to its children and
 * will keep the heap property in tact.
 *
 * Params:
 *  pq - the priority queue being worked on
 *  idx - the index of the node being checked
 */
void pq_percolateDown(struct pq* pq, int idx) {
	int left_idx, right_idx, min_idx;
	left_idx = (2 * idx) + 1;
	right_idx = (2 * idx) + 2;
	struct pq_node* min, *left, *right, *curr, *temp;
	
	curr = dynarray_get(pq->da, idx);

	if (right_idx > (dynarray_size(pq->da) - 1)) {
		if (left_idx >= (dynarray_size(pq->da) - 1)) {
			return;
		}
		else {
			min = dynarray_get(pq->da, left_idx);
			min_idx = left_idx;
		}
	}
	else {
		left = dynarray_get(pq->da, left_idx);
		right = dynarray_get(pq->da, right_idx);
		if (left->priority <= right->priority) {
			min = dynarray_get(pq->da, left_idx);
			min_idx = left_idx;
		}
		else {
			min = dynarray_get(pq->da, right_idx);
			min_idx = right_idx;
		}
	}
	//printf("index: %d\n", idx);
	/*printf("current prio = %d\n", curr->priority);
	printf("min prio = %d\n", min->priority);
	printf("left prio = %d\n", left->priority);
	printf("right prio = %d\n", right->priority);*/
	if (curr->priority > min->priority) {
		temp = min;
		dynarray_set(pq->da, min_idx, curr);
		dynarray_set(pq->da, idx, temp);
		pq_percolateDown(pq, min_idx);
	}
}

/*
 * This function should allocate and initialize an empty priority queue and
 * return a pointer to it.
 */
struct pq* pq_create() {
	struct pq* pq = malloc(sizeof(struct pq));
	pq->da = dynarray_create();
	return pq;
}


/*
 * This function should free the memory allocated to a given priority queue.
 * Note that this function SHOULD NOT free the individual elements stored in
 * the priority queue.  That is the responsibility of the caller.
 *
 * Params:
 *   pq - the priority queue to be destroyed.  May not be NULL.
 */
void pq_free(struct pq* pq) {
	dynarray_free(pq->da);
	free(pq);
	return;
}


/*
 * This function should return 1 if the specified priority queue is empty and
 * 0 otherwise.
 *
 * Params:
 *   pq - the priority queue whose emptiness is to be checked.  May not be
 *     NULL.
 *
 * Return:
 *   Should return 1 if pq is empty and 0 otherwise.
 */
int pq_isempty(struct pq* pq) {
	if(dynarray_size(pq->da) == 0) {
		return 1;
	}
	return 0;
}


/*
 * This function should insert a given element into a priority queue with a
 * specified priority value.  Note that in this implementation, LOWER priority
 * values are assigned to elements with HIGHER priority.  In other words, the
 * element in the priority queue with the LOWEST priority value should be the
 * FIRST one returned.
 *
 * Params:
 *   pq - the priority queue into which to insert an element.  May not be
 *     NULL.
 *   value - the value to be inserted into pq.
 *   priority - the priority value to be assigned to the newly-inserted
 *     element.  Note that in this implementation, LOWER priority values
 *     should correspond to elements with HIGHER priority.  In other words,
 *     the element in the priority queue with the LOWEST priority value should
 *     be the FIRST one returned.
 */
void pq_insert(struct pq* pq, void* value, int priority) {
	int temp_idx = dynarray_size(pq->da);
	int temp_p, temp, size;
	struct pq_node* pq_node = malloc(sizeof(struct pq_node));
	pq_node->value = value;
	pq_node->priority = priority;
	
	dynarray_insert(pq->da, pq_node);
	size = dynarray_size(pq->da) - 1;
	pq_percolateUp(pq, size);
	
	/*while (temp_idx > 0) {
		temp_p = ((temp_idx - 1) / 2);
		if (priority < dynarray_get(pq->da, temp_p)) {
			temp = dynarray_get(pq->da, temp_p);
			dynarray_set(pq->da, temp_p, value);
			dynarray_set(pq->da, temp_idx, temp);
			temp_idx = temp_p;
		}
	}*/
	return;
}


/*
 * This function should return the value of the first item in a priority
 * queue, i.e. the item with LOWEST priority value.
 *
 * Params:
 *   pq - the priority queue from which to fetch a value.  May not be NULL or
 *     empty.
 *
 * Return:
 *   Should return the value of the first item in pq, i.e. the item with
 *   LOWEST priority value.
 */
void* pq_first(struct pq* pq) {
	struct pq_node* p = dynarray_get(pq->da, 0);

	return p->value;
}


/*
 * This function should return the priority value of the first item in a
 * priority queue, i.e. the item with LOWEST priority value.
 *
 * Params:
 *   pq - the priority queue from which to fetch a priority value.  May not be
 *     NULL or empty.
 *
 * Return:
 *   Should return the priority value of the first item in pq, i.e. the item
 *   with LOWEST priority value.
 */
int pq_first_priority(struct pq* pq) {
	struct pq_node* p = dynarray_get(pq->da, 0);
	
	return p->priority;
}


/*
 * This function should return the value of the first item in a priority
 * queue, i.e. the item with LOWEST priority value, and then remove that item
 * from the queue.
 *
 * Params:
 *   pq - the priority queue from which to remove a value.  May not be NULL or
 *     empty.
 *
 * Return:
 *   Should return the value of the first item in pq, i.e. the item with
 *   LOWEST priority value.
 */
void* pq_remove_first(struct pq* pq) {
	//print(pq);

	struct pq_node* temp_remove = dynarray_get(pq->da, 0);
	int size;
	void* temp = temp_remove->value;

	struct pq_node* p = dynarray_get(pq->da, dynarray_size(pq->da) - 1);
	
	free(dynarray_get(pq->da, 0));

	if (pq_isempty(pq) != 1) {
		size = dynarray_size(pq->da) - 1;
		dynarray_set(pq->da, 0,p);
		dynarray_remove(pq->da, size);
		size--;
		if(size >= 0)
			pq_percolateDown(pq, 0);
	}

	return temp;
}
