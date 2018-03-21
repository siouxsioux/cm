/*
	exp.c  --  expression

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/exp.c $
 * revision 2.1 cond 89/10/04 23:28:20
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:34:41
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/


#include <stdio.h>

#include "cmdef.h"

static char rcsID[] = "$Header: RCS/exp.c 2.1 89/10/04 23:28:20 cond Exp $";

extern	TYPE	*ptrToInt, *ptrToChar;



/*	expConst --  Constant */
public	EXPR	*expConst(CONST c)
{
	EXPR	*p;

	switch (c.type) {
	Case CONST_INT or CONST_CHAR:
		p = makeNode1(op_CONST, 'K', TYPE_INTEGER, NULL);
		p->e_value = c.value;
		return	p;
	Case CONST_STRING:
		p = makeNode1(op_STR, 'R', ptrToChar, NULL);
		p->e_value = c.value;
		return	p;
	Default:
		bug("expConst");
	}
}

/*	expIdent -- Identifier */
public	EXPR	*expIdent(char *sym)
{
	SYMTBL	*s;
	TYPE	*type;
	EXPR	*e;

	if ((s = searchLoc(sym)) == NULL) {
		if ((s = searchGlo(sym)) == NULL) {
			error2("undefined identifier '%s'", sym);
			s = regLocId(sym);
			s->type = TYPE_INTEGER;
		}
	}

	type = s->type;
	if (isArray(type)) {
		e = makeNode1(op_ADR, 'R', type, NULL);
		e->e_var = s;
		e->e_offset = 0;
		return	e;
	} else if (isFunction(type)) {
		e = makeNode1(op_ADR, 'R', makePointer(type), NULL);
		e->e_var = s;
		e->e_offset = 0;
		return	e;
	} else {
		e = makeNode1(op_ADR, 'R', makePointer(type), NULL);
		e->e_var = s;
		e->e_offset = 0;
		return	makeNode1(op_INDIR, toOptype(type), type, e);
	}
}

/*	expAddrof -- address-of operator (unary '&') */
public	EXPR	*expAddrof(EXPR *p)
{
	if (p == NULL)			return	NULL;
	if (p->opcode != op_INDIR) {
		error("&: l-value required");
		return	NULL;
	}
	return	p->e_left;
}

/*	expIndirect -- indirect operator (unary '*') */
public	EXPR	*expIndirect(EXPR *p)
{
	TYPE	*type;

	if (p == NULL)			return	NULL;
	type = p->type;
	if (isArray(type))
		p->type = type = makePointer(type->father);
	if (!isPointer(type)) {
		error("pointer required");
		return NULL;
	}
	type = type->father;
	if (isFunction(type))
		return p;
	if (isArray(type)) {
		p->type = type;
		return p;
	}
	return	makeNode1(op_INDIR, toOptype(type), type, p);
}

/*	enBool --  convert numeric type ('I', 'C') to boolean */
public	EXPR	*enBool(EXPR *p)
{
	EXPR	*zero;

	if (p == NULL)
		return NULL;
	if (p->optype == 'B')
		return	p;
	zero = makeNode1(op_CONST, 'K', TYPE_INTEGER, NULL);
	zero->e_value = 0;
	p = makeNode2(op_BOOL, 'B', TYPE_INTEGER, p, zero);
	p->misc = Bool_NEQ;
	return	p;
}

/*	deBool -- covert boolean ('B') type to numeric type ('I', 'C')  */
public	EXPR	*deBool(EXPR *p, char to)
{
	EXPR	*zero, *one;

	if (p == NULL)
		return NULL;
	if (p->optype == 'B') {
 		zero = makeNode1(op_CONST, 'K', TYPE_INTEGER, NULL);
		zero->e_value = 0;
		one  = makeNode1(op_CONST, 'K', TYPE_INTEGER, NULL);
		one->e_value = 1;
		p = makeNode3(op_COND, to, toType(to), p, one, zero);
	}
	return	p;
}

/*	logNot --  perform logical NOT operator  */
EXPR	*logNot(EXPR *p)
{
	switch (p->opcode) {
	Case op_BOOL:	p->misc ^= Bool_NOT;
	Case op_LAND:	logNot(p->e_left);	/* !(L && R) => !L || !R  */
			logNot(p->e_right);	/*	[de Morgan]	  */
			p->opcode = op_LOR;
	Case op_LOR:	logNot(p->e_left);	/* !(L || R) => !L && !R  */
			logNot(p->e_right);	/*	[de Morgan]	  */
			p->opcode = op_LAND;
	}
	return	p;
}

/*	expUnary --  some unary operators (+, -, ~, !)  */
public	EXPR	*expUnary(OPCODE op, EXPR *p)
{
	if (p == NULL)			return	NULL;
	if (!isNumeric(p->type)) {
		error2("%s: type mismatch", opname(op));
		return	NULL;
	}
	if (p->optype == 'K') {
		p->e_value = fold1(op, p->e_value);
		return	p;
	}
	if (op == op_PLUS)		return	p;
	else if (op == op_LNOT)		return	logNot(enBool(p));
	else {  /** if (op == op_BNOT || op == op_MINUS)  **/
		p = deBool(p, 'I');
		return	makeNode1(op, p->optype, toType(p->optype), p);
	}
}

public	EXPR	*expIncdec(OPCODE op, EXPR *p)
{
	int	scale;
	TYPE	*type;

	if (p == NULL)			return	NULL;
	type = p->type;
	if (isNumeric(type))
		scale = 1;
	else if (isPointer(type))
		scale = computeSize(type->father);
	else {
		error2("%s: type mismatch", opname(op));
		return	NULL;
	}

	if (p->opcode != op_INDIR) {
		error2("%s: l-value required", opname(op));
		return	NULL;
	}
	p = makeNode1(op, p->optype, type, p);
	p->misc = scale;
	return	p;
}

/**	expBinary -- normal binary operators (%, /, *, &, ^, |)  **/
public	EXPR	*expBinary(OPCODE op, EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;
	char	optype;

	if (a == NULL || b == NULL)		return	NULL;
	ltype = a->type;	rtype = b->type;
	if (!isNumeric(ltype) || ! isNumeric(rtype)) {
		error2("%s: type mismatch", opname(op));
		return	NULL;
	}
	optype = adjust(a->optype, b->optype);
	if (optype == 'K') {
		a->e_value = fold2(op, a->e_value, b->e_value);
		return	a;
	}
	return	makeNode2(op, optype, toType(optype),
				coerce(a, optype), coerce(b, optype));
}

/**	expScaled -- scaled addition / subtraction  **/
EXPR	*expScaled(OPCODE op, EXPR *ptr, EXPR *num)
{
	int	scale = computeSize(ptr->type->father);

	if (isArray(ptr->type))
		ptr->type = makePointer(ptr->type->father);
	if (ptr->opcode == op_ADR && num->optype == 'K') {
		if (op == op_ADD)	ptr->e_offset += num->e_value * scale;
		else			ptr->e_offset -= num->e_value * scale;
		return	ptr;
	}
	return	makeNode2(op == op_ADD ? op_SCALEADD : op_SCALESUB,
				'R', ptr->type, ptr, coerce(num, 'I'));
}

/**	expDescaled -- descaled subtraction  **/
EXPR	*expDescaled(EXPR *a, EXPR *b)
{
	if (! eqType(a->type, b->type)) {
		error("-: pointer type mismatch");
		return	NULL;
	}
	return	makeNode2(op_DESCALESUB, 'I', TYPE_INTEGER, a, b);
}

/**	expAddsub -- binary addition and subtraction  */
public	EXPR	*expAddsub(OPCODE op, EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;
	char	optype;

	if (a == NULL || b == NULL)		return	NULL;
	ltype = a->type;	rtype = b->type;
	if (isArray(ltype))
		a->type = ltype = makePointer(ltype->father);
	if (isArray(rtype))
		b->type = rtype = makePointer(rtype->father);
	if (isNumeric(ltype) && isNumeric(rtype)) {
		optype = adjust(a->optype, b->optype);
		if (optype == 'K') {
			a->e_value = fold2(op, a->e_value, b->e_value);
			return	a;
		}
		return	makeNode2(op, optype, toType(optype),
				coerce(a, optype), coerce(b, optype));
	} else if (isPointer(ltype) && isNumeric(rtype))
		return	expScaled(op, a, b);
	else if (isNumeric(ltype) && isPointer(rtype) && op == op_ADD)
		return	expScaled(op, b, a);
	else if (isPointer(ltype) && isPointer(rtype) && op == op_SUB)
		return	expDescaled(a, b);
	else {
		error2("%s:  type mismatch", opname(op));
		return	NULL;
	}
}

/*	expShiftop --  shift operator (<<, >>)  */
public	EXPR	*expShiftop(OPCODE op, EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;

	if (a == NULL || b == NULL)		return	NULL;
	ltype = a->type;	rtype = b->type;
	if (! isNumeric(ltype) || ! isNumeric(rtype)) {
		error2("%s: type mismatch", opname(op));
		return	NULL;
	}
	a = deBool(a, 'I');		b = deBool(b, 'I');
	if (a->optype == 'K' && b->optype == 'K') {
		a->e_value = fold2(op, a->e_value, b->e_value);
		return	a;
	}
	return	makeNode2(op, a->optype, ltype, a, b);
}

/*	expCompare --  compare operators (==, !=, <, <=, >, >=)  */
public	EXPR	*expCompare(OPCODE op, EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;
	char	optype, x;
	EXPR	*p;

	if (a == NULL || b == NULL)		return	NULL;
	ltype = a->type;	rtype = b->type;
	if (isArray(ltype))
		a->type = ltype = makePointer(ltype->father);
	if (isArray(rtype))
		b->type = rtype = makePointer(rtype->father);
	if (isNumeric(ltype) && isNumeric(rtype)) {
		optype = adjust(a->optype, b->optype);
		if (optype == 'K') {
			a->e_value = fold2(op, a->e_value, b->e_value);
			return	a;
		}
		a = coerce(a, optype);	b = coerce(b, optype);
	} else if (isPointer(ltype) && b->optype == 'K') {
		b = expCast(ltype, b);
	} else if (a->optype == 'K' && isPointer(rtype)) {
		a = expCast(rtype, a);
	} else if (isPointer(ltype) && isPointer(rtype)) {
		if (!eqType(ltype, rtype)) {
			error2("%s: pointer mismatch", opname(op));
			return	NULL;
		}
	} else {
		error2("%s: type mismatch", opname(op));
		return	NULL;
	}
	switch (op) {			/* normalize compare operation */
	Case op_EQ:	x = Bool_EQ;
	Case op_NEQ:	x = Bool_NEQ;
	Case op_GT:	x = Bool_GT;
	Case op_GE:	exchange(a, b);	x = Bool_LE;
	Case op_LT:	exchange(a, b); x = Bool_GT;
	Case op_LE:	x = Bool_LE;
	}
	p = makeNode2(op_BOOL, 'B', TYPE_INTEGER, a, b);
	p->misc = x;
	return	p;
}

public	EXPR	*expLogop(OPCODE op, EXPR *a, EXPR *b)
{
	if (a == NULL || b == NULL)		return	NULL;
	if (! isNumeric(a->type) || ! isNumeric(b->type)) {
		error2("%s: type mismatch", opname(op));
		return	NULL;
	}
	return	makeNode2(op, 'B', TYPE_INTEGER, enBool(a), enBool(b));
}

public	EXPR	*expConditional(EXPR *cond, EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;
	char	optype;

	if (cond == NULL || a == NULL || b == NULL)		return	NULL;
	ltype = a->type;	rtype = b->type;
	if (! isNumeric(cond->type)) {
		error("?: condition must be numeric type");
		return	NULL;
	}
	cond = enBool(cond);
	if (isArray(ltype))
		a->type = ltype = makePointer(ltype->father);
	if (isArray(rtype))
		b->type = rtype = makePointer(rtype->father);
	if (isNumeric(ltype) && isNumeric(rtype)) {
		optype = adjust(a->optype, b->optype);
		a = coerce(a, optype);	b = coerce(b, optype);
		return	makeNode3(op_COND, optype, toType(optype), cond, a, b);
	} else if (isPointer(ltype) && b->optype == 'K') {
		return makeNode3(op_COND, 'R', ltype, cond, a, expCast(ltype, b));
	} else if (a->optype == 'K' && isPointer(rtype)) {
		return makeNode3(op_COND, 'R', rtype, cond, expCast(rtype, a), b);
	} else if (isPointer(ltype) && isPointer(rtype)) {
		if (!eqType(ltype, rtype)) {
			error("?: pointer mismatch");
			return	NULL;
		}
		return makeNode3(op_COND, 'R', ltype, cond, a, b);
	}
	error("?: type mismatch");
	return	NULL;
}

public	EXPR	*expAssignop(char op, EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;
	EXPR	*e;

	if (a == NULL || b == NULL)		return	NULL;
	if (a->opcode != op_INDIR) {
		error("Assign Op: l-value required");
		return	NULL;
	}
	ltype = a->type;	rtype = b->type;
	if (isPointer(ltype) && isNumeric(rtype)) {
		if (op == '+' || op == '-') {
			e = makeNode2(op_ASSIGN_OP_R, 'R', ltype,
				a, coerce(b, 'I'));
			e->misc = op;
			return	e;
		}
	} else if (isNumeric(ltype) && isNumeric(rtype)) {
		e = makeNode2(op_ASSIGN_OP, toOptype(ltype), ltype,
				a, coerce(b, toOptype(ltype)));
		e->misc = op;
		return	e;
	} else {
		error("Assign Op: type mismatch");
		return	NULL;
	}
}

/*	expAssign --  assignment (= only)  */
public	EXPR	*expAssign(EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;

	if (a == NULL || b == NULL)		return	NULL;
	if (a->opcode != op_INDIR) {
		error("=: l-value required");
		return	NULL;
	}
	ltype = a->type;	rtype = b->type;
	if (isArray(rtype))
		b->type = rtype = makePointer(rtype->father);
	if (isNumeric(ltype) && isNumeric(rtype)) {
		return	makeNode2(op_ASSIGN, toOptype(ltype), ltype,
				a, coerce(b, toOptype(ltype)));
	} else if (isPointer(ltype) && b->optype == 'K') {
		return	makeNode2(op_ASSIGN, 'R', ltype, a, expCast(ltype, b));
	} else if (isPointer(ltype) && isPointer(rtype)) {
		if (!eqType(ltype, rtype)) {
			error("=: pointer mismatch");
			return	NULL;
		}
		return	makeNode2(op_ASSIGN, 'R', ltype, a, b);
	}
	error("=: type mismatch");
	return	NULL;
}

/*	expSizeofExp --  sizeof expression  */
public	EXPR	*expSizeofExp(EXPR *a)
{
	EXPR	*p;

	if (a == NULL)			return	NULL;
	p = makeNode1(op_CONST, 'K', TYPE_INTEGER, NULL);
	p->e_value = computeSize(a->type);
	return	p;
}

/*	expSizeofType --  sizeof type  */
public	EXPR	*expSizeofType(TYPE *a)
{
	EXPR	*p;

	if (a == NULL)			return	NULL;
	p = makeNode1(op_CONST, 'K', TYPE_INTEGER, NULL);
	p->e_value = computeSize(a);
	return	p;
}

public	EXPR	*expCast(TYPE *to, EXPR *a)
{
	TYPE	*from = a->type;

	if (to == NULL || a == NULL)		return	NULL;
	if (isArray(from))
		a->type = from = makePointer(from->father);
	if (isNumeric(to)) {
		if (isNumeric(from))
			return	coerce(a, to == TYPE_INTEGER ? 'I' : 'C');
		else /** isPointer(from) **/ {
			if (to == TYPE_INTEGER)
				return	makeNode1(op_RtoI, 'I', to, a);
			else /** if (to == TYPE_CHAR) **/
				return	makeNode1(op_RtoC, 'C', to, a);
		}
	} else if (isPointer(to)) {
		if (isNumeric(from)) {
			if (from == TYPE_INTEGER)
				return	makeNode1(op_ItoR, 'R', to,
							deBool(a, 'I'));
			else /** if (from ==TYPE_CHAR) **/
				return	makeNode1(op_CtoR, 'R', to, a);
		} else /** isPointer(from) **/ {
			a->type = to;
			return	a;
		}
	} else {
		error("cast: cannot cast to this type");
		return	NULL;
	}
}

/*	expComma --  Comma operator  */
public	EXPR	*expComma(EXPR *a, EXPR *b)
{
	if (a == NULL || b == NULL)		return	NULL;
	return	makeNode2(op_COMMA, b->optype, b->type, a, b);
}

/**	expArray -- array reference  **/
public	EXPR	*expArray(EXPR *a, EXPR *b)
{
	TYPE	*ltype, *rtype;

	if (a == NULL || b == NULL)		return	NULL;
	ltype = a->type;	rtype = b->type;
	if (isArray(ltype))
		a->type = ltype = makePointer(ltype->father);
	if (isArray(rtype))
		b->type = rtype = makePointer(rtype->father);
	if (isPointer(ltype) && isNumeric(rtype))
		;
	else if (isNumeric(ltype) && isPointer(rtype)) {
		exchange(a, b);
	} else {
		error("[]: type mismatch");
		return	NULL;
	}
	return	expIndirect(expScaled(op_ADD, a, b));
}

public	EXPR	*expFuncall(EXPR *fun, TREE *paramlist)
{
	TREE	*p;
	TYPE	*type;
	EXPR	*param, *x;

	if (fun == NULL)		return	NULL;
	type = fun->type;
	if (isFunction(type))
		type = type->father;
	else if (isPointer(type) && isFunction(type->father)) {
		type = type->father->father;
	} else {
		error("function or pointer to function required");
		return	NULL;
	}
	for (p = paramlist; p != NULL && p != ERROR; p = p->first) {
		param = (EXPR *)p->second;
		if (param == NULL)
			return	NULL;
		if (isNumeric(param->type))
			p->second = (TREE *)coerce(param, 'I');
	}
	x = makeNode2(op_FUNC, 'I', TYPE_INTEGER, fun, (EXPR *)paramlist);
	if (type == TYPE_CHAR)
		return	coerce(x, 'C');
	else if (isPointer(type))
		return	makeNode1(op_ItoR, 'R', type, x);
	else
		return	x;
}

