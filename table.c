/*
	table.c	-- symbol/type table handler

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/table.c $
 * revision 2.1 cond 89/10/04 23:34:41
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.21 cond 89/10/04 23:24:08
 * modify a little... to shut up MS-C compiler.
 * 
 * revision 1.2 cond 89/09/17 20:35:26
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/

#include <string.h>
#include <stdio.h>


#include "cmdef.h"


static char rcsID[] = "$Header: RCS/table.c 2.1 89/10/04 23:34:41 cond Exp $";

static	char	gloIdent[GLOBAL_IDENT_SIZE];
static	char	*gloIdPtr;
static	char	locIdent[LOCAL_IDENT_SIZE];
static	char    *locIdPtr;


static	SYMTBL	gloSymtbl[GLOBAL_SYMTBL_SIZE];
static	SYMTBL	*gloSymPtr;
static	SYMTBL	locSymtbl[LOCAL_SYMTBL_SIZE];
static	SYMTBL	*locSymPtr;


static	TYPE	gloType[GLOBAL_TYPE_SIZE];
static	TYPE	*gloTypePtr;
static	TYPE	locType[LOCAL_TYPE_SIZE];
static	TYPE	*locTypePtr;

TYPE	*ptrToInt, *ptrToChar;


/*  initTable -- initializes both global and local tables */
public	void	initTable(void)
{
	gloIdPtr   = gloIdent;
	gloSymPtr  = gloSymtbl;
	gloTypePtr = gloType;
	ptrToInt   = regGloType(tc_POINTER, TYPE_INTEGER);
	ptrToChar  = regGloType(tc_POINTER, TYPE_CHAR);
	initLocTbl();
}

/*  initLocTbl -- initializes local tables */
public	void	initLocTbl(void)
{
	locIdPtr   = locIdent;
	locSymPtr  = locSymtbl;
	locTypePtr = locType;
}



/*  saveGloId -- saves global string to string area */
public	char	*saveGloId(char *s)
{
	int	len;
	char	*p;

	len = strlen(s)+1;
	p = gloIdPtr;
	if ((gloIdPtr += len) >= gloIdent + GLOBAL_IDENT_SIZE)
		fatalError("Global Identifier overflow\n");
	return	strcpy(p, s);
}

/*  saveLocId -- saves local string to string area */
public	char	*saveLocId(char *s)
{
	int	len;
	char	*p;

	len = strlen(s)+1;
	p = locIdPtr;
	if ((locIdPtr += len) >= locIdent + LOCAL_IDENT_SIZE)
		fatalError("local Identifier overflow\n");
	return	strcpy(p, s);
}



/*  regGloId -- registers identifier into global symbol table */
public	SYMTBL	*regGloId(char *s)
{
	if (gloSymPtr >= gloSymtbl + GLOBAL_SYMTBL_SIZE)
		fatalError("Global Symbol Table overflow\n");
	gloSymPtr->name = s;
	return	gloSymPtr++;
}

/*  regLocId -- registers identifier into local symbol table */
public	SYMTBL	*regLocId(char *s)
{
	if (locSymPtr >= locSymtbl + LOCAL_SYMTBL_SIZE)
		fatalError("Local Symbol Table overflow\n");
	locSymPtr->name = s;
	return	locSymPtr++;
}


/*  searchGlo -- search global symbol table */
public	SYMTBL	*searchGlo(char *s)
{
	SYMTBL	*p;

	for (p = gloSymtbl; p < gloSymPtr; p++)
		if (strcmp(s, p->name) == 0)
			return	p;
	return	NULL;
}

/*  searchLoc -- search local symbol table */
public	SYMTBL	*searchLoc(char *s)
{
	SYMTBL	*p;

	for (p = locSymtbl; p < locSymPtr; p++)
		if (strcmp(s, p->name) == 0)
			return	p;
	return	NULL;
}


/*  regGloType -- registers type into global type table */
public	TYPE	*regGloType(TCLASS tc, TYPE *father)
{
	if (gloTypePtr >= gloType + GLOBAL_TYPE_SIZE)
		fatalError("Global Type Table overflow\n");
	gloTypePtr->tc = tc;
	gloTypePtr->father = father;
	return	gloTypePtr++;
}

/*  regLocType -- registers type into local type table */
public	TYPE	*regLocType(TCLASS tc, TYPE *father)
{
	if (locTypePtr >= locType + LOCAL_TYPE_SIZE)
		fatalError("Local Type Table overflow\n");
	locTypePtr->tc = tc;
	locTypePtr->father = father;
	return	locTypePtr++;
}


/*  genGloref -- generate EXTRN/PUBLIC directive for global symbols */
public	void	genGloref(void)
{
	SYMTBL	*p;
	TYPE	*t;

	for (p = gloSymtbl; p < gloSymPtr; p++) {
#if	0
		if (p->class == SC_GLOBAL) {
			if (! isFunction(p->type))
				gencode("_%s\tdb\t%d dup (?)\n",
					p->name, computeSize(p->type));
			gencode("\tpublic\t_%s\n", p->name);
		} else 
#endif
		if (p->class == SC_EXTERN) {
			t = p->type;
			gencode("\textrn\t_%s:", p->name);
			if (isFunction(t)) {
				gencode("near\n");
				continue;
			}
			if (isArray(t))
				for (; isArray(t); t = t->father)
					;
			if (t == TYPE_CHAR)
				gencode("byte\n");
			else
				gencode("word\n");
		}
	}
}

/*  computeOffset -- compute offsets of parameters and global variables */
/*                   returns size of local variables                    */
public	int	computeOffset(void)
{
/**  compute offsets of parameters  **/
#if	0
	SYMTBL	*p, *q;
	int	offset;

	for (p = locSymtbl; p->class == SC_PARAM; p++)
		;
	offset = 4;
	for (q = p - 1; q >= locSymtbl; q--) {
		q->offset = offset;
		offset += 2;
	}
#else
	SYMTBL	*p;
	int	offset;

	offset = 8;
	for (p = locSymtbl; p->class == SC_PARAM; p++) {
		p->offset = offset;
		offset += 4;
	}
#endif
/**  compute offsets of local variables **/

	offset = 0;
	for (; p < locSymPtr; p++) {
		offset += computeSize(p->type);
		p->offset = -offset;
	}
	return	offset;	
 }



/*  dumpSymTbl -- dumps contents of global and local symbol table (debugging)*/
public	void	dumpSymTbl(void)
{
	SYMTBL	*p;
	TYPE	*t;

/*	dump global symbol table */
	for (p = gloSymtbl; p < gloSymPtr; p++) {
		printf("Glo [%04x] \"%-10s\" class=%d type=%04x\n",
				p, p->name, p->class, p->type);
		for (t = p->type; ; t = t->father) {
			if (isPrimeType(t)) {
				if (t == TYPE_CHAR)
					printf("    [char]\n");
				else if (t == TYPE_INTEGER)
					printf("    [int]\n");
				break;
			}
			printf("    [%04x] tc=%d father=%04x size=%d\n",
					t, t->tc, t->father, t->size);
		}
	}

/*	dump local symbol table */
	for (p = locSymtbl; p < locSymPtr; p++) {
		printf("Loc [%04x] \"%-10s\" class=%d type=%04x offset=%d\n",
				p, p->name, p->class, p->type, p->offset);
		for (t = p->type; ; t = t->father) {
			if (isPrimeType(t)) {
				if (t == TYPE_CHAR)
					printf("    [char]\n");
				else if (t == TYPE_INTEGER)
					printf("    [int]\n");
				break;
			}
			printf("    [%04x] tc=%d father=%04x size=%d\n",
					t, t->tc, t->father, t->size);
		}
	}
}
