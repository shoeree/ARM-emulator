/* 
Header File: Linked Lists Functions
------------------------------------------------------------------------
A header file containing functions and a node structure for working
with linked lists.

Creator:    Sterling Hoeree
University: Zhejiang University, Hangzhou, China
Student ID: 3090300043

Version	Date		Comments
1.0		10/01/25	* created generic linked list functions & structures
2.0		11/04/19	* changed names of structures (llist -> llist, llnode -> llnode)
 					and names of functions (prefixed with 'll' for 'linked list')
 					* added llfind function
-----------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>

#ifndef LNKLISTS_H
#define LNKLISTS_H

/* The llnode structure */
typedef struct llnode_s {
	void *data;
	struct llnode_s *next;
	struct llnode_s *prev;
} llnode;

/* The llist structure */
typedef struct {
	llnode *head;
	llnode *tail;
	int size;
} llist;

/* ------------------------------------------------------ */

/* Generic functions */
llist* createLList();
llnode* createLLNode(void*);

llnode* getIndex(llist*, int);

typedef int (*LLCMP_FUNC)(void*, void*);
llnode* llfind(llist*, void*, LLCMP_FUNC);

void lladdHead(llist*, llnode*);
void lladdTail(llist*, llnode*);
void llinsIndex(llist*, int, llnode*);

void lldelHead(llist*);
void lldelTail(llist*);

void lldelAll(llist*);

#endif // LNKLISTS_H
