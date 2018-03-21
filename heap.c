/*
	heap.c	-- heap and tree

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/heap.c $
 * revision 2.1 cond 89/10/04 23:33:42
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:35:34
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 

*/


#include <stdio.h>
#include <string.h>


#include "cmdef.h"


static char rcsID[] = "$Header: RCS/heap.c 2.1 89/10/04 23:33:42 cond Exp $";

static	char	heap[HEAPSIZE];
static	char	*heapptr;

public	void	resetHeap(void)
{
	heapptr = heap;
}

public	TREE	*list1(DCLNODE a)
{
	TREE	*p;

	p = (TREE *)heapptr;
	if ((heapptr += sizeof(struct tree1)) >= heap + HEAPSIZE)
		fatalError("heap overflow\n");
	p->node = a;
	return	p;
}

public	TREE	*list2(DCLNODE a, void *b)
{
	TREE	*p;

	p = (TREE *)heapptr;
	if ((heapptr += sizeof(struct tree2)) >= heap + HEAPSIZE)
		fatalError("heap overflow\n");
	p->node = a;
	p->first = b;
	return	p;
}

public	TREE	*list3(DCLNODE a, void *b, void *c)
{
	TREE	*p;

	p = (TREE *)heapptr;
	if ((heapptr += sizeof(struct tree3)) >= heap + HEAPSIZE)
		fatalError("heap overflow\n");
	p->node = a;
	p->first = b;
	p->second = c;
	return	p;
}

public	TREE	*append(TREE *a, TREE *b)
{
	TREE	*p;

	p = a;
	while (a->second != NULL)
		a = a->second;
	a->second = b;
	return	p;
}


/*	saveIdent -- save string into heap area */
public	char	*saveIdent(char *s)
{
	int		len;
	char	*p;

	len = strlen(s)+ 1;
	p = (char *)heapptr;
	if ((heapptr += len) >= heap + HEAPSIZE)
		fatalError("heap overflow\n");
	return	strcpy(p, s);
}

/*	makeNode1 -- make unary EXPR node */
public	EXPR	*makeNode1(OPCODE a, char optype, TYPE *type, EXPR *left)
{
	EXPR	*p;

	p = (EXPR *)heapptr;
	if ((heapptr += sizeof(EXPR)) >= heap + HEAPSIZE)
		fatalError("heap overflow\n");
	p->opcode = a;
	p->optype = optype;
	p->type   = type;
	p->e_left = left;
	return	p;
}

/*	makeNode2 -- make unary EXPR node */
public	EXPR	*makeNode2(OPCODE a, char optype, TYPE *type, EXPR *left, EXPR *right)
{
	EXPR	*p;

	p = (EXPR *)heapptr;
	if ((heapptr += sizeof(EXPR)) >= heap + HEAPSIZE)
		fatalError("heap overflow\n");
	p->opcode  = a;
	p->optype  = optype;
	p->type    = type;
	p->e_left  = left;
	p->e_right = right;
	return	p;
}

/*	makeNode3 -- make unary EXPR node */
public	EXPR	*makeNode3(OPCODE a, char optype, TYPE *type, EXPR *left, EXPR *right, EXPR *third)
{
	EXPR	*p;

	p = (EXPR *)heapptr;
	if ((heapptr += sizeof(EXPR)) >= heap + HEAPSIZE)
		fatalError("heap overflow\n");
	p->opcode  = a;
	p->optype  = optype;
	p->type    = type;
	p->e_left  = left;
	p->e_right = right;
	p->e_third = third;
	return	p;
}
