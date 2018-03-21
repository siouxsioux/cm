/*
	stmt.c	-- statements

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/stmt.c $
 * revision 2.1 cond 89/10/04 23:33:31
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:35:22
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/

#include <stdio.h>
#include <string.h>

#include "cmdef.h"


static char rcsID[] = "$Header: RCS/stmt.c 2.1 89/10/04 23:33:31 cond Exp $";

static	int	stack[LABEL_STACK_SIZE], *sp;
extern	int	breakLabel, contLabel;
extern	SYMTBL	*currentFunction;

/**  pushLabels -- push break label and continue label **/
public	void	pushLabels(void)
{
	if (sp >= &stack[LABEL_STACK_SIZE])
		fatalError("label stack overflow\n");
	*sp++ = breakLabel;
	if (sp >= &stack[LABEL_STACK_SIZE])
		fatalError("label stack overflow\n");
	*sp++ = contLabel;
}

/**  popLabels -- pop break label and continue label **/
public	void	popLabels(void)
{
	contLabel = *--sp;
	breakLabel = *--sp;
}



/************************************************************************/
/*									*/
/*	Notorious Goto-Label handling routine				*/
/*									*/
/************************************************************************/

static	LABEL	labelTable[LABEL_TABLE_SIZE], *lp;


/**  searchLabel  -- search label table  */
public	LABEL	*searchLabel(char *s)
{
	LABEL	*p;

	for (p = labelTable; p < lp; p++)
		if (strcmp(s, p->name) == 0)
			return	p;
	return	NULL;
}

/**  regLabel -- register (goto) label into label table  */
public	LABEL	*regLabel(char *s, bool defp)
{
	if (lp >= &labelTable[LABEL_TABLE_SIZE])
		fatalError("label table overflow\n");
	lp->name = saveLocId(s);
	lp->defp = defp;
	lp->num  = gensym();
	return	lp++;
}

/********************************************************/
/*							*/
/*	switch / case handling				*/
/*							*/
/********************************************************/

static	SWITCH_	swstack[SW_STACK_SIZE], *swsp;

static	int	swid, defLabel;
static	SWLABEL	swlabel[SW_LABEL_TABLE_SIZE], *swtop, *swbottom;


public	void	doSwitchhead(EXPR *exp)
{
	int	l0, l1;

	if (swsp >= &swstack[SW_STACK_SIZE]) {
		fatalError("switch too nested");
	}
	swsp->swtop	= swtop;
	swsp->swbottom	= swbottom;
	swsp->swid	= swid;
	swsp->defLabel	= defLabel;
	swsp++;
	swtop = swbottom = swtop;
	swid  = gensym();	gensym();	gensym();
	defLabel = 0;
	l0 = gensym();	l1 = gensym();

	pushLabels();
	breakLabel = swid + 2;

	if (exp->optype == 'R') {
		genexp(exp);
		gencode("\tmov\teax,ebx\n");
	} else
		genexp(coerce(exp, 'I'));
	gencode("\tmov\tecx,@%d\n", swid);
	gencode("\tmov\tebx,offset @%d\n", swid+1);
	genlabel(l0);
	gencode("\tcmp\teax,[ebx]\n");
	gencode("\tjne\t@%d\n", l1);
	gencode("\tjmp\tdword ptr [ebx+4]\n");
	genlabel(l1);
	gencode("\tadd\tebx,8\n");
	gencode("\tloop\t@%d\n", l0);
	gencode("\tjmp\tdword ptr [ebx]\n");
}

public	void	doSwitchend(void)
{
	SWLABEL	*p;

	genjump(swid + 2);
	gencode("@%d\tdd\t%d\n", swid, swtop - swbottom);
	genlabel(swid + 1);
	for (p = swbottom; p < swtop; p++)
		gencode("\tdd\t%d,@%d\n", p->value, p->label);
	gencode("\tdd\t@%d\n", defLabel);
	genlabel(swid + 2);

	swsp--;
	swtop	= swsp->swtop;
	swbottom= swsp->swbottom;
	swid	= swsp->swid;
	defLabel= swsp->defLabel;

	popLabels();
}

public	void	doCase(EXPR *exp)
{
	int	val;
	SWLABEL	*p;

	if (exp->optype != 'K') {
		error("case label must be constant");
		return;
	}
	val = exp->e_value;
	for (p = swbottom; p < swtop; p++) {
		if (p->value == val) {
			error("duplicate switch label");
			return;
		}
	}
	if (swtop >= &swlabel[SW_LABEL_TABLE_SIZE])
		fatalError("too many switch labels...\n");
	swtop->value = val;
	genlabel(swtop->label = gensym());
	swtop++;
}
 
public	void	doDefault(void)
{
	if (defLabel != 0) {
		error("'default' appers more than twice in this switch");
		return;
	}
	genlabel(defLabel = gensym());
}

/********************************************************/
/*							*/
/*	house keeping on function entry/exit		*/
/*							*/
/********************************************************/

/**  initFunc -- initialize environment before entering function **/
public	void	initFunc(void)
{
	sp = stack;
	lp = labelTable;
	swtop = swlabel;
	swsp  = swstack;
}

/**  endFunc -- wind-up routine on exiting function **/
public	void	endFunc(void)
{
	LABEL	*p;

	for (p = labelTable; p < lp; p++)
		if (p->defp == NO)
			error3("label '%s' is not defined in function '%s'",
				p->name, currentFunction->name);
}
