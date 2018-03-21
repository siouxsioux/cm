/*
	genexp.c -- traverse expression tree and generate code

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/genexp.c $
 * revision 2.1 cond 89/10/04 23:32:02
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:34:54
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/

#include <stdio.h>

#include "cmdef.h"


static char rcsID[] = "$Header: RCS/genexp.c 2.1 89/10/04 23:32:02 cond Exp $";

extern	int	errCount;

/*	isVariable  --  check whether argument is Variable  */
public	bool	isVariable(EXPR *a)
{
	return	a->opcode == op_INDIR && a->e_left->opcode == op_ADR;
}

/*	isConst -- check whether argument is constant  */
#define	isConst(a)	((a)->opcode == op_CONST)

/*	genlabel -- generate label  */
public	void	genlabel(int label)
{
	gencode("@%d:\n", label);
}

/*	genjump -- generate jump */
public	void	genjump(int label)
{
	gencode("\tjmp\t@%d\n", label);
}

/*	expstmt -- generate code for expression statement */
public	void	expstmt(EXPR *p)
{
	if (errCount == 0) {
		genexptop(p);
		gencode(";\n");
	}
}

/*	genassign -- generate code for assignment */
void	genassign(EXPR *p, bool needValue)
{
	EXPR	*left = p->e_left, *right = p->e_right;

	if (isVariable(left)) {
		if (isConst(right) && ! needValue)
			gencode("\tmov\t%v,%d\n", left, right->e_value);
		else {
			genexp(right);
			gencode("\tmov\t%v,%r\n", left, p->optype);
		}
	} else {
		genexp(left->e_left);
		if (isConst(right) && ! needValue)
			gencode("\tmov\t%o[ebx],%d\n", p->optype, right->e_value);
		else {
			pushbx();
			genexp(right);
			popdi();
			gencode("\tmov\t[edi],%r\n", p->optype);
		}
	}
}

/*	genassignop  -- generate code for assignment operator  */
void	genassignop(EXPR *p, bool needValue)
{
	EXPR	*l	= p->e_left,
		*r	= p->e_right;
	char	opt	= p->optype,
		*op;

	switch (p->misc) {
	Case '&':	op = "and";	goto l1;
	Case '+':	op = "add";	goto l1;
	Case '^':	op = "xor";	goto l1;
	Case '|':	op = "or";
l1:		if (isVariable(l) && isConst(r) && ! needValue)
			gencode("\t%s\t%v,%d\n", op, l, r->e_value);
		else if (isVariable(l)) {
			genexp(r);
			if (needValue) {
				gencode("\t%s\t%r,%v\n", op, opt, l);
				gencode("\tmov\t%v,%r\n", l, opt);
			} else
				gencode("\t%s\t%v,%r\n", op, l, opt);
		} else {
			genexp(l);
			if (isConst(l))
				gencode("\tmov\t%r,%d\n", opt, l->e_value);
			else {
				pushbx();	genexp(r);	popbx();
			}
			if (needValue) {
				gencode("\t%s\t%r,[ebx]\n", op, opt);
				gencode("\tmov\t[ebx],%r\n", opt);
			} else
				gencode("\t%s\t[ebx],%r\n", op, opt);
		}
	Case '>':	op = "shr";	goto l2;
	Case '<':	op = "shl";
l2:		if (isVariable(l)) {
			genexp(r);
			gencode("\tmov\tcl,al\n");
			if (needValue) {
				gencode("\tmov\t%r,%v\n", opt, l);
				gencode("\t%s\t%r,cl\n", op, opt);
				gencode("\tmov\t%v,%r\n", l, opt);
			} else
				gencode("\t%s\t%v,cl\n", op, l);
		} else {
			genexp(l);
			if (isConst(r))
				gencode("\tmov\tcl,%d\n", r->e_value);
			else {
				pushbx();	genexp(r);	popbx();
				gencode("\tmov\tcl,al\n");
			}
			if (needValue) {
				gencode("\tmov\t%r,[ebx]\n", opt);
				gencode("\t%s\t%r,cl\n", op, opt);
				gencode("\tmov\t[ebx],%r\n", opt);
			} else
				gencode("\t%s\t[ebx],cl\n", op);
		}
	Case '*':
		if (isVariable(l)) {
			genexp(r);
			gencode("\timul\t%v\n", l);
			gencode("\tmov\t%v,%r\n", l, opt);
		} else {
			genexp(l);
			if (isConst(r))
				gencode("%tmov\t%r,%d\n", opt, r->e_value);
			else {
				pushbx();	genexp(r);	popbx();
			}
			gencode("\timul\t%o[ebx]\n", opt);
			gencode("\tmov\t[ebx],%r\n", opt);
		}
	Case '/' or '%':
		if (isVariable(l)) {
			if (isConst(r))
				gencode("\tmov\t%y,%d\n", opt, r->e_value);
			else {
				genexp(r);
				gencode("\tmov\t%y,%r\n", opt, opt);
			}
			gencode("\tmov\t%r,%v\n", opt, l);
			if (opt == 'C')		gencode("\tcbw\n");
			else			gencode("\tcwd\n");
			gencode("\tidiv\t%y\n", opt);
			if (p->misc == '/')
				gencode("\tmov\t%v,%r\n", l, opt);
			else
				gencode("\tmov\t%v,%a\n", l, opt);
		} else {
			genexp(l);
			if (isConst(r))
				gencode("\tmov\t%y,%d\n", opt, r->e_value);
			else {
				pushbx();	genexp(r);	popbx();
				gencode("\tmov\t%y,%r\n", opt, opt);
			}
			gencode("\tmov\t%r,[ebx]\n", opt);
			if (opt == 'C')		gencode("\tcbw\n");
			else			gencode("\tcwd\n");
			gencode("\tidiv\t%y\n", opt);
			if (p->misc == '/')
				gencode("\tmov\t[ebx],%r\n", opt);
			else
				gencode("\tmov\t[ebx],%a\n", opt);
		}
	Case '-':
		if (isVariable(l)) {
			genexp(r);
			if (needValue) {
				gencode("\tmov\t%a,%r\n", opt, opt);
				gencode("\tmov\t%r,%v\n", opt, l);
				gencode("\tsub\t%r,%a\n", opt, opt);
				gencode("\tmov\t%v,%r\n", l, opt);
			} else
				gencode("\tsub\t%v,%r\n", l, opt);
		} else {
			genexp(l);
			if (isConst(r))
				gencode("\tmov\t%a,%d\n", opt, r->e_value);
			else {
				pushbx();	genexp(r);	popbx();
				gencode("\tmov\t%a,%r\n", opt, opt);
			}
			if (needValue) {
				gencode("\tmov\t%r,[ebx]\n", opt);
				gencode("\tsub\t%r,%a\n", opt, opt);
				gencode("\tmov\t[ebx],%r\n", opt);
			} else
				gencode("\tsub\t[ebx],%a\n", opt);
		}
	Default:	bug("genassignop");
	}
}

/*	scaleAX -- scale AX register */
void	scaleAX(int scale)
{
	switch (scale) {
	Case 1:	return;
	case 8:	gencode("\tshl\teax,1\n");
	case 4:	gencode("\tshl\teax,1\n");
	case 2:	gencode("\tshl\teax,1\n");
		return;
	Default:gencode("\tmov\tedx,%d\n", scale);
		gencode("\timul\tedx\n");
	}
}

/*	genassignopref  -- generate code for assignment operator  */
/*				(pointer += num, pointer -= num)  */
void	genassignopRef(EXPR *p, bool needValue)
{
	EXPR	*l	= p->e_left,
		*r	= p->e_right;
	char	*op	= p->misc == '+' ? "add" : "sub";
	int	size;


	size = computeSize(p->type->father);
	if (isVariable(l) && isConst(r)) {
		if (needValue) {
			gencode("\tmov\tebx,%v\n", l);
			gencode("\t%s\tebx,%d\n", op, r->e_value * size);
			gencode("\tmov\t%v,ebx\n", l);
		} else
			gencode("\t%s\t%v,%d\n", op, l, r->e_value * size);
	} else if (isVariable(l)) {
		genexp(r);
		scaleAX(size);
		if (needValue) {
			gencode("\tmov\tebx,%v\n", l);
			gencode("\t%s\tebx,eax\n", op);
			gencode("\tmov\t%v,ebx\n", l);
		} else
			gencode("\t%s\t%v,eax\n", op, l);
	} else if (isConst(r)) {
		genexp(l);
		if (needValue) {
			gencode("\tmov\tedi,ebx\n");
			gencode("\tmov\tebx,[edi]\n");
			gencode("\t%s\tebx,%d\n", r->e_value * size);
			gencode("\tmov\t[edi],ebx\n");
		} else
			gencode("\t%s\t[ebx],%d\n", op, r->e_value * size);
	} else {
		genexp(l);	pushbx();	genexp(r);
		scaleAX(size);	popdi();
		if (needValue) {
			gencode("\tmov\tebx,[edi]\n");
			gencode("\tadd\tebx,eax\n");
			gencode("\tmov\t[edi],ebx\n");
		} else
			gencode("\t%s\t[edi],eax\n", op);
	}
}

/*	genincdectop -- generate code for ++, --                */
/*			(When all the required is side effect)  */
void	genincdectop(EXPR *p)
{
	char	*incdec, *addsub, optype = p->optype;
	OPCODE	op = p->opcode;
	EXPR	*exp;

	if (op == op_PREINC || op == op_POSTINC)
		incdec = "inc",	addsub = "add";
	else
		incdec = "dec",	addsub = "sub";
	exp = p->e_left;
	if (isVariable(exp)) {
		if (p->misc == 1)
			gencode("\t%s\t%v\n", incdec, exp);
		else
			gencode("\t%s\t%v,%d\n", addsub, exp, p->misc);
	} else {
		genexp(exp->e_left);
		if (p->misc == 1)
			gencode("\t%s\t%o[ebx]\n", incdec, optype);
		else
			gencode("\t%s\t%o[ebx],%n\n", addsub, optype, p->misc);
	}
}

/*	genexptop -- generate code for expression		*/
/*			(with some optimization for top level)	*/
public	void	genexptop(EXPR *p)
{
	if (p == NULL)		return;
	switch (p->opcode) {
	Case op_ASSIGN:
		genassign(p, NO);	/* we only need side effect */
	Case op_ASSIGN_OP:
		genassignop(p, NO);	/* we only need side effect */
	Case op_ASSIGN_OP_R:
		genassignopRef(p, NO);	/* we only need side effect */
	Case op_PREINC or op_POSTINC or op_PREDEC or op_POSTDEC:
		genincdectop(p);	/* we only need side effect */
	Default:
		genexp(p);
	}
}


/*	gendiv -- generate code for division  */
void	gendiv(EXPR *right, EXPR *left, char optype, int optimize)
{
	char	*conv = optype == 'C' ? "\tcbw\n" : "\tcwd\n";

	if (isVariable(right)) {
		genexp(left);
		gencode(conv);
		gencode("\tidiv\t%v\n", right);
	} else if (isConst(right)) {
		genexp(left);
		gencode(conv);
		gencode("\tmov\t%y,%d\n", optype, right->e_value);
		gencode("\tidiv\t%y\n", optype);
	} else if (isVariable(left) || isConst(left)) {
		genexp(right);
		gencode("\tmov\t%y,%r\n", optype, optype);
		if (isVariable(left))
			gencode("\tmov\t%r,%v\n", optype, left);
		else
			gencode("\tmov\t%r,%d\n", optype, left->e_value);
		gencode(conv);
		gencode("\tidiv\t%y\n", optype);
	} else {
		genexp(right);
		pushax();
		genexp(left);
		gencode(conv);
		gencode("\tpop\t%y\n", optype);
		gencode("\tidiv\t%y\n", optype);
	}
}

/*	genbitop  -- generate code for bit operation (&, |, ^)  */
void	genbitop(OPCODE op, EXPR *left, EXPR *right, char optype)
{
	char  *code =   op == op_AND ? "and"
	              : op == op_OR  ? "or"
	              : /* op_XOR */   "xor";

	if (isConst(left) || isVariable(left))
		exchange(left, right);
	if (isConst(right)) {
		genexp(left);
		gencode("\t%s\t%r,%d\n", code, optype, right->e_value);
	} else if (isVariable(right)) {
		genexp(left);
		gencode("\t%s\t%r,%v\n", code, optype, right);
	} else {
		genexp(left);
		pushax();
		genexp(right);
		popdx();
		gencode("\t%s\t%r,%a\n", code, optype, optype);
	}
}

/*	genshiftop -- generate code for shift operator (>>, <<)  */
void	genshiftop(OPCODE op, EXPR *left, EXPR *right, char optype)
{
	char  *code =   op == op_SHR ? "shr"
	              : /* op_SHL */   "shl";
	int	i;

	if (isConst(right)) {
		genexp(left);
		for (i = right->e_value; i > 0; i--)
			gencode("\t%s\t%r,1\n", code, optype);
	} else if (isVariable(right)) {
		genexp(left);
		gencode("\tmov\tcl,%v\n", right);
		gencode("\t%s\t%r,cl\n", code, optype);
	} else if (isConst(left)) {
		genexp(right);
		gencode("\tmov\tcl,al\n");
		gencode("\tmov\teax,%d\n", left->e_value);
		gencode("\t%s\teax,cl\n", code);
	} else if (isVariable(left)) {
		genexp(right);
		gencode("\tmov\tcl,al\n");
		gencode("\tmov\t%r,%v\n", optype, left);
		gencode("\t%s\t%r,cl\n", code, optype);
	} else {
		genexp(right);
		pushax();
		genexp(left);
		popcx();
		gencode("\t%s\t%r,cl\n", code, optype);
	}
}

/*	gencondjump -- generate conditional branch */
void	gencondjump(char optype, char cond, int true, int false)
{
static	char	*inst[][4] = {
	{ "je", "jne", "jg", "jle" },
	{ "je", "jne", "ja", "jbe" }
};
	char	**instp;
	int	target, exit, temp;

	instp = inst[optype == 'R'];
	if (true != 0) {
		target = true;
		exit = false;
	} else {
		cond ^= Bool_NOT;
		target = false;
		exit = 0;
	}
	gencode("\t%s\t@%d\n", instp[cond ^ Bool_NOT], temp = gensym());
	gencode("\tjmp\t@%d\n", target);
	genlabel(temp);
	if (exit != 0)
		genjump(exit);
}

/*	gencompare -- generate compare  */
void	gencompare(EXPR *p, int true, int false)
{
	EXPR	*left, *right;
	char	optype;

	left = p->e_left;	right = p->e_right;
	optype = left->optype;
	if (isVariable(right)) {
		genexp(left);
		gencode("\tcmp\t%r,%v\n", optype, right);
	} else if (isConst(right)) {
		genexp(left);
		gencode("\tcmp\t%r,%d\n", optype, right->e_value);
	} else if (isConst(left)) {
		genexp(right);
		gencode("\tmov\t%a,%d\n", optype, left->e_value);
		gencode("\tcmp%a,%r\n", optype, optype);
	} else if (isVariable(left)) {
		genexp(right);
		gencode("\tcmp\t%v,%r\n", left, optype);
	} else {
		genexp(left);
		pushax();
		genexp(right);
		popdx();
		gencode("\tcmp\t%a,%r\n", optype, optype);
	}
	gencondjump(optype, p->misc, true, false);
}

/*	genbool -- generate code for boolean operation */
public	void	genbool(EXPR *p, int true, int false)
{
	int	x;

	if (p == NULL)		return;
	switch (p->opcode) {
	Case op_BOOL:	gencompare(p, true, false);
	Case op_LAND:	if (false == 0) {
				genbool(p->e_left, 0, x = gensym());
				genbool(p->e_right, true, 0);
				genlabel(x);
			} else {
				genbool(p->e_left, 0, false);
				genbool(p->e_right, true, false);
			}
	Case op_LOR:	if (true == 0) {
				genbool(p->e_left, x = gensym(), 0);
				genbool(p->e_right, 0, false);
				genlabel(x);
			} else {
				genbool(p->e_left, true, 0);
				genbool(p->e_right, true, false);
			}
	Default:	bug("genbool");
	}
}

/*	genscaled -- generate code for scaled addtion/subtraction  */
void	genscaled(OPCODE op, EXPR *left, EXPR *right)
{
	char 	*addsub = op == op_SCALEADD ? "add" : "sub";
	int	scale = computeSize(left->type->father);

	if (isConst(right)) {
		genexp(left);
		gencode("\t%s\tebx,%d\n", addsub, right->e_value * scale);
	} else if (isVariable(right)) {
		genexp(left);
		if (scale == 1)
			gencode("\t%s\tebx,%v\n", addsub, right);
		else {
			gencode("\tmov\teax,%v\n", right);
			scaleAX(scale);
			gencode("\t%s\tebx,eax\n", addsub);
		}
	} else if (right->opcode == op_ADR) {
		genexp(right);
		scaleAX(scale);
		genlea(right);
		gencode("\t%s\tebx,eax\n", addsub);
	} else if (isVariable(right)) {
		genexp(right);
		scaleAX(scale);
		gencode("\tmov\tebx,%v\n", right);
		gencode("\t%s\tebx,eax\n");
	} else {
		genexp(left);
		pushbx();
		genexp(right);
		scaleAX(scale);
		popbx();
		gencode("\t%s\tebx,eax\n", addsub);
	}
}

/*	gendescaled -- generate code for descaled subtraction */
void	gendescaled(EXPR *left, EXPR *right)
{
	int	scale = computeSize(left->type->father);

	genexp(left);
	pushbx();
	genexp(right);
	popax();
	gencode("\tsub\teax,ebx\n");
	switch (scale) {
	case 8:	gencode("\tshr\neax,1\n");
	case 4:	gencode("\tshr\neax,1\n");
	case 2:	gencode("\tshr\neax,1\n");
		break;
	case 1:	break;
	default:
		gencode("\tmov\tedx,0\n");
		gencode("\tmov\tecx,%d\n", scale);
		gencode("\tidiv\tecx\n");
	}
}

/*	genfuncall  -- generate code for function call */
void	genfuncall(EXPR *fun, TREE *paramlist)
{
	TREE	*p;
	EXPR	*param;
	int	count = 0;

	for (p = paramlist; p != NULL; p = p->first) {
		param = (EXPR *)p->second;
		genexp(param);
		if (isNumeric(param->type))
			pushax();
		else
			pushbx();
		count++;
	}
	if (fun->opcode == op_ADR)
		gencode("\tcall\t_%s\n", fun->e_var->name);
	else {
		genexp(fun);
		gencode("\tcall\t[ebx]\n");
	}
	if (count != 0)
		gencode("\tadd\tesp,%d\n", count * 4);
}

/*	genincdec -- generate code for ++/--  */
void	genincdec(OPCODE op, char optype, EXPR *exp, int scale)
{
	char	*addsub, *incdec;

	if (op == op_PREINC || op == op_POSTINC)
		addsub = "add", incdec = "inc";
	else
		addsub = "sub", incdec = "dec";
	if (op == op_PREINC || op == op_PREDEC) {
		if (isVariable(exp)) {
			if (scale == 1)
				gencode("\t%s\t%v\n", incdec, exp);
			else
				gencode("\t%s\t%v,%d\n", addsub, exp, scale);
			gencode("\tmov\t%r,%v\n", optype, exp);
		} else {
			genexp(exp);
			if (scale == 1)
				gencode("\t%s\t%o[ebx]\n", incdec, optype);
			else
				gencode("\t%s\t%o[ebx],%d\n",
						addsub, optype, scale);
			gencode("\tmov\t%r,[ebx]\n", optype);
		}
	} else /** if (op == op_POSTINC || op == op_POSTDEC) **/ {
		if (isVariable(exp)) {
			gencode("\tmov\t%r,%v\n", optype, exp);
			if (scale == 1)
				gencode("\t%s\t%v\n", incdec, exp);
			else
				gencode("\t%s\t%v,%d\n", addsub, exp, scale);
		} else if (optype == 'R') {
			genexp(exp);
			gencode("\tmov\tesi,ebx\n");
			gencode("\tmov\tebx,[ebx]\n");
			if (scale == 1)
				gencode("\t%s\t%o[esi]\n", incdec, optype);
			else
				gencode("\t%s\t%o[esi],%d\n",
						addsub, optype, scale);
		} else { /** if (optype == 'I' || optype == 'C') **/
			genexp(exp);
			gencode("\tmov\t%r,[ebx]\n", optype);
			if (scale == 1)
				gencode("\t%s\t%o[ebx]\n", incdec, optype);
			else
				gencode("\t%s\t%o[ebx],%d\n",
						addsub, optype, scale);
		}
	}
}

/*	genexp -- traverse expression tree and generate code */
public	void	genexp(EXPR *p)
{
	EXPR	*left, *right;
	char	optype;
	int	label;

	if (p == NULL)
		return;
	left   = p->e_left;		right  = p->e_right;
	optype = p->optype;
	switch (p->opcode) {
	Case op_ADR:
		genlea(p);
	Case op_INDIR:
		if (left->opcode == op_ADR)		/* variable */
			gencode("\tmov\t%r,%v\n", optype, p);
		else {
			genexp(left);
			gencode("\tmov\t%r,[ebx]\n", optype);
		}
	Case op_CONST:
		gencode("\tmov\t%r,%d\n", optype, p->e_value);
	Case op_STR:
//		gencode("\tmov\tebx,offset DGROUP:@%d\n", p->e_value);
		gencode("\tmov\tebx,offset @%d\n", p->e_value);
	Case op_ASSIGN:
		genassign(p, YES);	/* YES, we need val of this exp */
	Case op_CtoI:
		genexp(left);
		gencode("\tcbw\n");
	Case op_CtoR:
		genexp(left);
		gencode("\tcbw\n");
		gencode("\tmov\tebx,eax\n");
	Case op_ItoC:
		genexp(left);
	Case op_ItoR:
		genexp(left);
		gencode("\tmov\tebx,eax\n");
	Case op_RtoC or op_RtoI:
		genexp(left);
		gencode("\tmov\teax,ebx\n");
	Case op_FUNC:
		genfuncall(left, (TREE *)right);
	Case op_ADD:
		if (isConst(left) || isVariable(left))
			exchange(left, right);
		if (isConst(right)) {
			genexp(left);
			gencode("\tadd\t%r,%d\n", optype, right->e_value);
		} else if (isVariable(right)) {
			genexp(left);
			gencode("\tadd\t%r,%v\n", optype, right);
		} else {
			genexp(left);
			pushax();
			genexp(right);
			popdx();
			gencode("\tadd\t%r,%a\n", optype, optype);
		}
	Case op_SUB:
		if (isVariable(right)) {
			genexp(left);
			gencode("\tsub\t%r,%v\n", optype, right);
		} else if (isConst(right)) {
			genexp(left);
			gencode("\tsub\t%r,%d\n", optype, right->e_value);
		} else if (isVariable(left)) {
			genexp(right);
			gencode("\tneg\t%r\n", optype);
			gencode("\tadd\t%r,%v\n", optype, left);
		} else if (isConst(left)) {
			genexp(right);
			gencode("\tneg\t%r\n", optype);
			gencode("\tadd\t%r,%d\n", optype, right->e_value);
		} else {
			genexp(right);
			pushax();
			genexp(left);
			popdx();
			gencode("\tsub\t%r,%a\n", optype, optype);
		}
	Case op_SCALEADD or op_SCALESUB:
		genscaled(p->opcode, left, right);
	Case op_DESCALESUB:
		gendescaled(left, right);
	Case op_MUL:
		if (isConst(left) || isVariable(left))
			exchange(left, right);
		if (isConst(right)) {
			genexp(left);
			gencode("\tmov\t%a,%d\n", optype, right->e_value);
			gencode("\timul\t%a\n", optype);
		} else if (isVariable(right)) {
			genexp(left);
			gencode("\timul\t%v\n", right);
		} else {
			genexp(left);
			pushax();
			genexp(right);
			popdx();
			gencode("\tadd\t%a\n", optype);
		}
	Case op_DIV:
		gendiv(right, left, optype, YES);
	Case op_MOD:
		gendiv(right, left, optype, NO);
		if (optype == 'I')
			gencode("\tmov\teax,edx\n");
		else
			gencode("\tmov\tal,ah\n");
	Case op_MINUS:
		genexp(left);
		gencode("\tneg\t%r\n", optype);
	Case op_BNOT:
		genexp(left);
		gencode("\tnot\t%r\n", optype);
	Case op_AND or op_OR or op_XOR:
		genbitop(p->opcode, left, right, optype);
	Case op_COMMA:
		genexp(left);	genexp(right);
	Case op_SHR or op_SHL:
		genshiftop(p->opcode, left, right, optype);
	Case op_BOOL:
		genexp(left);	genexp(right);
	Case op_COND:
		label = gensym();	gensym();
		genbool(left, 0, label);
		genexp(right);
		genjump(label + 1);
		genlabel(label);
		genexp(p->e_third);
		genlabel(label + 1);
	Case op_LAND:
		genbool(p, 0, label = gensym());
		genlabel(label);
	Case op_LOR:
		genbool(p, label = gensym(), 0);
		genlabel(label);
	Case op_PREINC or op_PREDEC or op_POSTINC or op_POSTDEC:
		genincdec(p->opcode, optype, left, p->misc);
	Case op_ASSIGN_OP:
		genassignop(p, YES);	/* YES, we need val of this op */
	Case op_ASSIGN_OP_R:
		genassignopRef(p, YES);	/* YES, we need val of this op */
	Default:
		printf("p->opcode = %d\n", p->opcode);
		bug("genexp");
	}
}
