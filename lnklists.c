/* 
Source File: Linked Lists Functions
------------------------------------------------------------------------
A source file containing the definition of functions and a Node structure 
for working with linked lists.

Creator:    Sterling Hoeree
University: Zhejiang University, Hangzhou, China
Student ID: 3090300043

Version	Date		Comments
1.0		10/01/25	* created generic linked list functions & structures
2.0		11/04/19	* changed names of structures (List -> llist, Node -> llnode)
 					and names of functions (prefixed with 'll' for 'linked list')
 					* added llfind function
-----------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>

#include "lnklists.h"

/* Generic functions */

llist* createLList() {
	llist *ls = (llist*)malloc(sizeof(llist));
	ls->head = NULL;
	ls->tail = NULL;
	ls->size = 0;

	return ls;
}

llnode* createLLNode(void *d) {
	llnode *n = (llnode*)malloc(sizeof(llnode));
	n->data = d;
	n->next = NULL;
	n->prev = NULL;
	
	return n;
}

/* Get a llnode at index 'in' */
/* Returns NULL if the index doesn't exist */
llnode* llgetIndex(llist *ls, int in) {
	llnode *nd = NULL;	
	if (in > ls->size) {	
		if (in <= ls->size) {	
			nd = ls->head;
			for (int i=0; i<in; i++, nd=nd->next);
		}
		else {
			nd = ls->tail;
			for (int i=ls->size; i>in; i--, nd=nd->prev);
		}
	}

	return nd;
}

/* Return a llnode that matches the given search condition */
/* Returns NULL if the element is not found */
/* NOTE: A *different* source file must supply the FUNCTION POINTER
 * which is used as the compare function, e.g. a cmp_int which compares
 * 2 integers. */
llnode* llfind(llist *ls, void *toFind, LLCMP_FUNC cmpf) {
	llnode *nd = NULL;
	for (llnode *p=ls->head; p!=NULL && nd==NULL; p=p->next) {
		if (cmpf(toFind, nd->data))
			nd = p;
	}
	return nd;
}

void lladdHead(llist *ls, llnode *nd) {
/* Adds a linked list node to the start of the linked list. */
	if (ls->head) {
		ls->head->prev = nd;
		nd->next = ls->head;
		nd->prev = NULL;
		ls->head = nd;
	}
	else {
		ls->head = nd;
		ls->tail = nd;
		nd->next = NULL;
		nd->prev = NULL;
	}
	ls->size++;
}

void lladdTail(llist *ls, llnode *nd) {
/* Adds a linked list node to the end of the linked list. */
	if (ls->tail) {
		ls->tail->next = nd;
		nd->prev = ls->tail;
		ls->tail = nd;
	}
	else {
		ls->head = nd;
		ls->tail = nd;
		nd->prev = NULL;
	}
	nd->next = NULL;
	ls->size++;
}

void llinsIndex(llist *ls, int index, llnode *nd) {
/* Adds a linked list node to the specified node of the linked list,Node *p = ls->head;
   inserting it at the index parameter supplied. */
	if (index <= 1) {
		lladdHead(ls, nd);
	}
	else if (index >= ls->size) {
		lladdTail(ls, nd);
	}
	else {
		if (index <= ls->size/2) {
			llnode *p = ls->head;			
			for (int i=0; i<index; i++, p=p->next);
			p->next->prev = nd;
			nd->next = p->next;
			nd->prev = p;
			p->next = nd;
		}
		else {
			llnode *p = ls->tail;			
			for (int i=ls->size; i>index; i--, p=p->prev);			
			p->prev->next = nd;
			nd->prev = p->prev;
			nd->next = p;
			p->prev = nd;		
		}		
		ls->size++;	
	}		
}	

void lldelHead(llist *ls) {
/* Removes the first node in the linked list. */

	if (ls->size > 0) {
		llnode* tmp = ls->head->next;
		if (tmp) {
			free(ls->head);
			ls->head = tmp;
		}
		else {
			free(ls->head);
			ls->head = 0;
			ls->tail = 0;
		}
		ls->size--;
	}
}

void lldelTail(llist *ls) {
/* Removes the last node in the linked list. */

	if (ls->size > 0) {
		llnode* p = ls->head;
		if ((ls->tail != ls->head) && ls->tail) {
			llnode *tmp = ls->tail->prev;		
			ls->tail->prev->next = NULL;		
			free(ls->tail);
			ls->tail = tmp;
			tmp->next = NULL;
		}
		else {
			free(ls->tail);
			ls->head = NULL;
			ls->tail = NULL;
		}
		ls->size--;	
	}
}

void lldelAll(llist *ls) {
/* Deletes and frees the memory of all nodes in the linked list. */

	int max = ls->size;
	for (int i=0; i<max; i++) {
		lldelTail(ls);
	}
}
