/*
	declare.c	-- process declaration

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/declare.c $
 * revision 2.1 cond 89/10/04 23:33:02
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:35:01
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/


#include <stdio.h>

#include "cmdef.h"

static char rcsID[] = "$Header: RCS/declare.c 2.1 89/10/04 23:33:02 cond Exp $";

static	int	externp;

SYMTBL	*currentFunction;	/* function in process now */


int	locVarSize;		/* size of local variable area (in bytes) */
int	exitLabel;		/* label of epilogue code  */


/* glodecl -- process global declaration */
private	SYMTBL	*glodecl(TYPE *type, TREE *tree)
{
	TYPE	*tp;
	SYMTBL	*s;
	char	*name;

	for (;;) {
		switch (tree->node) {
		Case dcl_ID:
			name = saveGloId((char *)tree->first);
			if ((s = searchGlo(name)) == NULL) {
				s = regGloId(name);
				s->class = isFunction(type)   ? SC_EXTERN :
					   (externp)          ? SC_EXTERN :
								SC_GLOBAL;
				s->type = type;
			} else {
				if (!eqType(type, s->type)) {
				    error2(
					"type mismatch in redeclaration of %s",
					name);
				    return	NULL;
				}
				if (isFunction(type) && !externp)
					s->class = SC_GLOBAL;
			}
			return	s;
		Case dcl_PTR:
			type = regGloType(tc_POINTER, type);
			tree = tree->first;
		Case dcl_FUNC:
			if (isArray(type)) {
				error("function returns array is not allowed");
				return	NULL;
			}
			type = regGloType(tc_FUNCTION, type);
			tree = tree->first;
		Case dcl_ARRAY:
			if (isFunction(type)) {
				error("array of function is not allowed");
				return	NULL;
			}
			tp = regGloType(tc_ARRAY, type);
			((struct array *)tp)->size = (int)tree->second;
			type = tp;
			tree = tree->first;
		Default:
			bug("glodecl");
		}
	}
}

/*	globalDataDecl -- global data declaration/definition */
public	void	globalDataDecl(TREE *tree, int extp)
{
	TYPE	*type, *t;
	TREE	*p;
	SYMTBL	*s;
	int	n;

	externp = extp;
	type = (TYPE *)tree->first;
	for (p = tree->second; p != NULL; p = p->second) {
		s = glodecl(type, p->first);
		if (s->class == SC_GLOBAL) {
			openseg(seg_BSS);
			gencode("\tpublic\t_%s\n", s->name);
			gencode("_%s\t", s->name);
			if (s->type == TYPE_CHAR)
				gencode("db\t?\n");
			else if (s->type == TYPE_INTEGER ||
				 s->type->tc == tc_POINTER)
				gencode("dd\t?\n");
			else /* if (s->type->tc == tc_ARRAY) */ {
				n = 1;
				for (t = s->type; isArray(t); t = t->father)
					n *= t->size;
				gencode("%s\t%d dup(?)\n",
					t == TYPE_CHAR ? "db" : "dd", n);
			}
		}
	}
}


/* paramdecl -- process parameter declaration */
private	SYMTBL	*paramdecl(TYPE *type, TREE *tree)
{
	TYPE	*tp;
	SYMTBL	*s;
	char	*name;

	for (;;) {
		switch (tree->node) {
		Case dcl_ID:
			name = saveLocId((char *)tree->first);
			if ((s = searchLoc(name)) != NULL) {
				error2("parameter '%s' is declared twice.",
					name);
				return	NULL;
			}
			if (isFunction(type)) {
				error("parameter cannot be function");
				return	NULL;
			} else if (isArray(type))
				type->tc = tc_POINTER;
			s = regLocId(name);
			s->class = SC_PARAM;
			s->type = type;
			return	s;
		Case dcl_PTR:
			type = regLocType(tc_POINTER, type);
			tree = tree->first;
		Case dcl_FUNC:
			if (isArray(type)) {
				error("function returns array is not allowed");
				return	NULL;
			}
			type = regLocType(tc_FUNCTION, type);
			tree = tree->first;
		Case dcl_ARRAY:
			if (isFunction(type)) {
				error("array of function is not allowed");
				return	NULL;
			}
			tp = regLocType(tc_ARRAY, type);
			((struct array *)tp)->size = (int)tree->second;
			type = tp;
			tree = tree->first;
		Default:
			bug("paramdecl");
		}
	}
}


/*	funcDef  -- function definition */
public	void	funcDef(TYPE *type, TREE *tree)
{
	SYMTBL	*s;
	TREE	*p;

	s = glodecl(type, tree);
	if (s == NULL)
		return;
 	if ( ! isFunction(s->type) ) {
		error2("'%s' is not declared as function", s->name);
		return;
	}
	s->class = SC_GLOBAL;

	currentFunction = s;

/**	register parameters into local symbol table **/

	initLocTbl();
	for (p = tree; ; p = p->first)
		if (p->node == dcl_FUNC && p->first->node == dcl_ID)
			break;

	for (p = p->second; p != NULL; p = p->second)
		paramdecl((TYPE *)p->first->first, (TREE *)p->first->second);
}

/* funchead -- generate function entry code */
public	void	funchead(void)
{
/**	First, compute offsets of parameters and local variables **/

	locVarSize = computeOffset();

/**	generate prologue code  **/

	openseg(seg_TEXT);
	gencode("\tpublic\t_%s\n", currentFunction->name);
	gencode("_%s\tproc\tnear\n", currentFunction->name);
	gencode("\tpush\tebp\n");
	gencode("\tmov\tebp,esp\n");
	if (locVarSize != 0) {
		locVarSize = (locVarSize + 1) & ~1;	/* align stack */
		gencode("\tsub\tesp,%d\n", locVarSize);
	}
	exitLabel = gensym();
}

/* funcend -- generate function exit code */
public	void	funcend(void)
{
	genlabel(exitLabel);

	if (locVarSize != 0)
		gencode("\tadd\tesp,%d\n", locVarSize);
	gencode("\tpop\tebp\n");
	gencode("\tret\n");
	gencode("_%s\tendp\n", currentFunction->name);
}


/* locdecl -- process local variable declaration */
private	SYMTBL	*locdecl(TYPE *type, TREE *tree)
{
	TYPE	*tp;
	SYMTBL	*s;
	char	*name;

	for (;;) {
		switch (tree->node) {
		Case dcl_ID:
			name = saveLocId((char *)tree->first);
			if ((s = searchLoc(name)) != NULL) {
				error2("local variable '%s' is declared twice.",
					name);
				return	NULL;
			}
			if (isFunction(type)) {
				error2("'%s' function cannot declared locally",
					name);
				return	NULL;
			}
			s = regLocId(name);
			s->class = SC_LOCAL;
			s->type = type;
			return	s;
		Case dcl_PTR:
			type = regLocType(tc_POINTER, type);
			tree = tree->first;
		Case dcl_FUNC:
			if (isArray(type)) {
				error("function returns array is not allowed");
				return	NULL;
			}
			type = regLocType(tc_FUNCTION, type);
			tree = tree->first;
		Case dcl_ARRAY:
			if (isFunction(type)) {
				error("array of function is not allowed");
				return	NULL;
			}
			tp = regLocType(tc_ARRAY, type);
			((struct array *)tp)->size = (int)tree->second;
			type = tp;
			tree = tree->first;
		Default:
			bug("locdecl");
		}
	}
}

/*	localDataDecl -- local data definition */
public	void	localDataDecl(TREE *tree)
{
	TYPE	*type;
	TREE	*p;

	type = (TYPE *)tree->first;
	for (p = tree->second; p != NULL; p = p->second)
		locdecl(type, p->first);
}

/* abstdecl -- process abstract declaration */
public	TYPE	*abstdecl(TYPE *type, TREE *tree)
{
	TYPE	*tp;

	for (;;) {
		switch (tree->node) {
		Case dcl_NONE:
			return	type;
		Case dcl_PTR:
			type = regLocType(tc_POINTER, type);
			tree = tree->first;
		Case dcl_FUNC:
			if (isArray(type)) {
				error("function returns array is not allowed");
				return	NULL;
			}
			type = regLocType(tc_FUNCTION, type);
			tree = tree->first;
		Case dcl_ARRAY:
			if (isFunction(type)) {
				error("array of function is not allowed");
				return	NULL;
			}
			tp = regLocType(tc_ARRAY, type);
			((struct array *)tp)->size = (int)tree->second;
			type = tp;
			tree = tree->first;
		Default:
			bug("abstdecl");
		}
	}
}
