/*
	misc.c	-- miscellaneous routines

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/misc.c $
 * revision 2.1 cond 89/10/04 23:33:25
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:35:18
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/


#include <stdio.h>
#include <string.h>


#include "cmdef.h"


static char rcsID[] = "$Header: RCS/misc.c 2.1 89/10/04 23:33:25 cond Exp $";

extern	TYPE	*ptrToInt, *ptrToChar;
extern	int	errCount, LineNo;
extern	char	FileName[];


public	void	error_line(void)
{
	fprintf(stderr, "%s %d ", FileName, LineNo);
}

public	void	error(char *str)
{
	error_line();
	fprintf(stderr, "%s\n", str);
	errCount++;
}

public	void	error2(char *str, char *param)
{
	error_line();
	fprintf(stderr, str, param);
	fprintf(stderr, "\n");
	errCount++;
}

public	void	error3(char *str, char *a, char *b)
{
	error_line();
	fprintf(stderr, str, a, b);
	fprintf(stderr, "\n");
	errCount++;
}

public	void	fatalError(char *str)
{
	error_line();
	fprintf(stderr, "%s", str);
	exit(1);
}

public	int	yyerror(char *s)
{
//	fprintf(stderr, s);
	fprintf(stderr, "%s", s);
	exit(1);
}

public	void	bug(char *func)
{
	error_line();
	fprintf(stderr, "This cannot happen in <%s>\n", func);
	exit(1);
}

public	char	*opname(OPCODE op)
{
	int	i;
	static	struct	{
		OPCODE	op;
		char	*name;
	}	table[] = {
		op_MOD, "%",		op_DIV, "/",
		op_MUL, "*",		op_AND, "&",
		op_XOR, "^",		op_OR, "|",
		op_EQ, "==",		op_NEQ, "!=",
		op_GT, ">",		op_GE, ">=",
		op_LT, "<",		op_LE, "<=",
		op_SHL, "<<",		op_SHR, ">>",
		op_ADD, "+",		op_SUB, "-",
		op_PREINC, "++",	op_PREDEC, "--",
		op_POSTINC, "++",	op_POSTDEC, "--"
	};

	for (i = 0; i < sizeof(table)/sizeof(table[1]); i++)
		if (table[i].op == op)
			return	table[i].name;
	return	"???";
}


static	int	seed = 1;			/* seed of gensym */

public	int		gensym(void)
{
	return	seed++;
}



public	bool	isPrimeType(TYPE *t)
{
	return	t == TYPE_CHAR || t == TYPE_INTEGER;
}

public	bool	eqType(TYPE *a, TYPE *b)
{
	for ( ; !isPrimeType(a) && ! isPrimeType(b); a=a->father,b=b->father)
		if (a->tc != b->tc)
			return	NO;
	return	a == b;
}

/*	makePointer -- makes pointer to parameter in local table */
public	TYPE	*makePointer(TYPE *t)
{
	if (t == TYPE_INTEGER)		return	ptrToInt;
	else if (t == TYPE_CHAR)	return	ptrToChar;
	else				return	regLocType(tc_POINTER, t);
}

/*	toOptype -- convert type to OPTYPE */
public	char	toOptype(TYPE *t)
{
	if (t == TYPE_INTEGER)		return	'I';
	else if (t == TYPE_CHAR)	return  'C';
	else if (t->tc == tc_POINTER)	return	'R';
	else				bug("toOptype");
}

/*	toType -- convert OPTYPE to type */
public	TYPE	*toType(char optype)
{
	switch(optype) {
	Case 'C':		return	TYPE_CHAR;
	Case 'I' or 'K':	return	TYPE_INTEGER;
	Default:		bug("toType");
	}
}

/*	isPointer -- check whether parameter is pointer type */
public	bool	isPointer(TYPE *t)
{
	return	(t != TYPE_INTEGER) && (t != TYPE_CHAR) &&
		(t->tc == tc_POINTER);
}

/*	isArray -- check whether parameter is array type */
public	bool	isArray(TYPE *t)
{
	return	(t != TYPE_INTEGER) && (t != TYPE_CHAR) && (t->tc == tc_ARRAY);
}

/*	isFunction -- check whether parameter is function type */
public	bool	isFunction(TYPE *t)
{
	return	(t != TYPE_INTEGER) && (t != TYPE_CHAR) &&
		(t->tc == tc_FUNCTION);
}

/*	isNumeric -- check whether paramter is numeric type (int or char) */
public	bool	isNumeric(TYPE *t)
{
	return	(t == TYPE_INTEGER) || (t == TYPE_CHAR);
}

/*  coerce -- coerce tree to 'type'  */
/*            This routine works on only arithmetic type ('C', 'I', 'K') */
public	EXPR	*coerce(EXPR *a, char to)
{
	char	from;
	
	from = a->optype;
	switch (to) {
	Case 'C':
		switch (from) {
		Case 'K' or 'C':	return	a;
		Case 'I':	return	makeNode1(op_ItoC, 'C', TYPE_CHAR, a);
		Case 'B':	return	deBool(a, 'C');
		Default:	bug("coerce1");
		}
	Case 'I':
		switch (from) {
		Case 'K' or 'I':
			return	a;
		Case 'B':
			return	deBool(a, 'I');
		Case 'C':
			switch (a->opcode) {
			Case op_ADR or op_INDIR or op_CONST or op_STR or
			op_ASSIGN or op_LNOT or
			op_PREINC or op_PREDEC or op_POSTINC or op_POSTDEC or
			op_ItoC or op_CtoI:
				return makeNode1(op_CtoI,'I', TYPE_INTEGER, a);
			Case op_ADD or op_SUB or op_MOD or op_DIV or op_MUL or
			op_SHR or op_SHL or
			op_GT or op_GE or op_LT or op_LE or
			op_EQ or op_NEQ or
			op_AND or op_XOR or op_OR or
			op_LAND or op_LOR:
				a->e_left  = coerce(a->e_left, 'I');
				a->e_right = coerce(a->e_right, 'I');
				a->optype = 'I';
				a->type = TYPE_INTEGER;
				return	a;
			Case op_PLUS or op_MINUS or op_BNOT:
				a->e_left = coerce(a->e_left, 'I');
				a->optype = 'I';
				a->type = TYPE_INTEGER;
				return	a;
			Case op_COND:
				a->e_right = coerce(a->e_right, 'I');
				a->e_third = coerce(a->e_third, 'I');
				a->optype = 'I';
				a->type = TYPE_INTEGER;
				return	a;
			Default:
				bug("coerce2");
			}
		Default:
			printf("op=%d\n", from);
			bug("coerce3");
		}
	Case 'K':
		if (from != 'K')
			bug("coerce2");
		return (a);
	Default:
		bug("coerce4");
	}
}

/*	adjust -- adjust two types */
/*            This routine works on only arithmetic type ('C', 'I', 'K') */
public	char	adjust(char left, char right)
{
	if (left == 'I' || right == 'I')		return	'I';
	if (left == 'C' || right == 'C')		return	'C';
	if (left == 'B' || right == 'B')		return	'I';
	return	'K';
}

/*	computeSize -- compute size of given type in bytes */
public	int	computeSize(TYPE *type)
{
	if (type == NULL)		return	0;
	if (type == TYPE_CHAR)		return	1;
	if (type == TYPE_INTEGER)	return	4;
	switch (type->tc) {
	Case tc_POINTER:		return	4;
	Case tc_ARRAY:			return	((struct array *)type)->size
						    * computeSize(type->father);
	Case tc_FUNCTION:		return	4;
	Default:			bug("computeSize");
	}
}

/*	fold1 --  fold constant-valued unary expression  */
public	int	fold1(OPCODE op, int a)
{
	switch (op) {
	Case op_PLUS:	return	a;
	Case op_MINUS:	return	-a;
	Case op_BNOT:	return	~a;
	Case op_LNOT:	return	!a;
	Default:	bug("fold1");
	}
}

/*	fold2 --  fold constant-valued unary expression  */
public	int	fold2(OPCODE op, int x, int y)
{
	switch (op) {
	Case op_MOD:	return	x % y;
	Case op_DIV:	return	x / y;
	Case op_MUL:	return	x * y;
	Case op_ADD:	return	x + y;
	Case op_SUB:	return	x - y;
	Case op_AND:	return	x & y;
	Case op_XOR:	return	x ^ y;
	Case op_OR:	return	x | y;
	Case op_EQ:	return	x == y;
	Case op_NEQ:	return	x != y;
	Case op_GT:	return	x > y;
	Case op_GE:	return	x >= y;
	Case op_LT:	return	x < y;
	Case op_LE:	return	x <= y;
	Case op_SHL:	return	x << y;
	Case op_SHR:	return	x >> y;
	Default:	bug("fold2");
	}
}

