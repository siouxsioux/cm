/*
	cmdef.h		-- common definition for Cm language

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/cmdef.h $
 * revision 2.1 cond 89/10/04 23:35:00
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.2 cond 89/09/17 20:35:52
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/

#include <stdlib.h>

#define	YYLEX		int

#define	YES		1
#define	NO		0

#define	ERROR	((void *)(-1))

#define	Case		break; case
#define	Default		break;	default
#define	or		: case

#define	NOT		!

#define	public
#define	private		static

typedef	int	bool;

/*	exchange  -- exchange two pointers  */
#define	exchange(a, b)	{ void *x; x = a; a = b; b = x; }


/********************************************************/
/*                                                      */
/*  IMPORTANT CONSTANTS                                 */
/*                                                      */
/********************************************************/

#define	IDENTLENGTH		32	/* maximum length of identifier */
#define	HEAPSIZE		2000	/* tree area (bytes)		*/

#define	GLOBAL_IDENT_SIZE	4000	/* global ident. area (bytes)	*/
#define	LOCAL_IDENT_SIZE	2000	/* local ident. area  (bytes)	*/

#define	GLOBAL_SYMTBL_SIZE	500	/* global symbol table(entries)	*/
#define	LOCAL_SYMTBL_SIZE	300	/* local symbol table  (entries)*/

#define	GLOBAL_TYPE_SIZE	500	/* global type table (entries)	*/
#define	LOCAL_TYPE_SIZE		300	/* local type table  (entries)	*/

#define	LABEL_STACK_SIZE	200	/* size of label stack (entries) */
#define	LABEL_TABLE_SIZE	100	/* size of label table (entries) */
#define	SW_LABEL_TABLE_SIZE	400	/* size of switch label table    */
					/*                     (entries) */
#define	SW_STACK_SIZE		10	/* size of switch stack (levels) */

/********************************************************/
/*                                                      */
/*  CONSTANT                                            */
/*                                                      */
/********************************************************/

typedef	enum	{	CONST_CHAR = 1, CONST_INT, CONST_STRING}	CONST_TYPE;

typedef	struct	_const {
    CONST_TYPE	type;
    long	value;
}   CONST;


/********************************************************/
/*                                                      */
/*  TYPE                                                */
/*                                                      */
/********************************************************/

typedef	enum	tc {	tc_POINTER, tc_ARRAY, tc_FUNCTION	}
		TCLASS;

typedef	struct	type	{
	TCLASS	tc;
	struct	type	*father;
	int	size;
}	TYPE;

typedef	struct	pointer	{		/** when  tc_POINTER **/
	TCLASS	tc;
	struct	type	*father;
};

typedef	struct	array	{		/** when  tc_ARRAY **/
	TCLASS	tc;
	struct	type	*father;
	int	size;
};

typedef	struct	func	{		/** when  tc_FUNCTION **/
	TCLASS	tc;
	struct	type	*father;
};


#define	TYPE_CHAR		(TYPE*)1
#define	TYPE_INTEGER	(TYPE*)2


/********************************************************/
/*                                                      */
/*  SYMBOL TABLE                                        */
/*                                                      */
/********************************************************/

typedef	enum {	SC_GLOBAL,	/* global variable/function		*/
				/*	(defined in this file)		*/
		SC_EXTERN,      /* global variable/function		*/
				/*  (defined elsewhere)			*/
		SC_LOCAL,	/* local variable			*/
		SC_PARAM	/* parameter				*/
}	SCLASS;

typedef	struct	symtbl	{
	char	*name;
	SCLASS	class;
	TYPE	*type;
	int	offset;		/* offset in stack frame		*/
				/*  (for SC_LOCAL, SC_PARM only)	*/
}	SYMTBL;


/********************************************************/
/*                                                      */
/*  TREE NODE                                           */
/*                                                      */
/********************************************************/

typedef	enum	{	dcl_DUMMY,		/* dummy */
			dcl_ID,			/* identifier */
			dcl_NONE,		/* empty */
			dcl_PTR,		/* pointer */
			dcl_FUNC,		/* function */
			dcl_ARRAY		/* array */
}	DCLNODE;

typedef	struct	tree {
	DCLNODE			node;
	struct	tree	*first;
	struct	tree	*second;
	struct	tree	*third;
}	TREE;

struct	tree1	{
	int			node;
};

struct	tree2	{
	int			node;
	struct	tree	*first;
};

struct	tree3	{
	int			node;
	struct	tree	*first;
	struct	tree	*second;
};


/********************************************************/
/*                                                      */
/*  EXPRESSION TREE                                     */
/*                                                      */
/********************************************************/

typedef	enum	{
	op_ADR, op_INDIR, op_CONST, op_STR,
	op_ASSIGN, op_ASSIGN_OP, op_ASSIGN_OP_R,
	op_PLUS, op_MINUS, op_BNOT, op_LNOT,
	op_PREINC, op_PREDEC, op_POSTINC, op_POSTDEC,
	op_ADD, op_SUB,
	op_SCALEADD, op_SCALESUB, op_DESCALESUB,
	op_MOD, op_DIV, op_MUL,
	op_SHR, op_SHL,
	op_GT, op_GE, op_LT, op_LE,
	op_EQ, op_NEQ,
	op_BOOL,
	op_AND, op_XOR, op_OR,
	op_LAND,
	op_LOR,
	op_COMMA,
	op_COND,
	op_FUNC,
	op_ItoC, op_CtoI, op_RtoI, op_RtoC, op_CtoR, op_ItoR
}	OPCODE;

typedef	struct	expr	{
	OPCODE	opcode;
	char	optype;
	int	misc;
	struct	type	*type;
	union	{
		int	value;			/*  for constant  */
		struct	{			/*  for variable  */
			SYMTBL	*var;
			int	offset;
		}	v;
		struct	{
			struct	expr	*left;
			struct	expr	*right;
			struct	expr	*third;
		}	s;
	}	u;
}	EXPR;

#define	Bool_EQ		0
#define	Bool_GT		2
#define	Bool_NOT	1
#define	Bool_NEQ	(Bool_EQ | Bool_NOT)
#define	Bool_LE		(Bool_GT | Bool_NOT)

#define	e_value		u.value		/*  for op_CONST  */
#define	e_var		u.v.var		/*  for op_ADR    */
#define	e_offset	u.v.offset	/*  for op_ADR    */
#define	e_left		u.s.left
#define	e_right		u.s.right
#define	e_third		u.s.third

/*
 	optype
 		'C'	byte
 		'I'	int
		'B'	bool
		'R'	pointer
		'K'	constant
*/

/************************************************************************/
/*	Notorious Goto-Label 						*/
/************************************************************************/

typedef	struct	{
	int	num;
	char	*name;
	bool	defp;
}	LABEL;

/************************************************************************/
/*	switch and case label						*/
/************************************************************************/

typedef	struct	{
	int	value;
	int	label;
}	SWLABEL;

typedef	struct	{
	SWLABEL	*swtop;
	SWLABEL	*swbottom;
	int	defLabel;
	int	swid;
}	SWITCH_;

/*
	segments
*/

typedef	enum	{
	seg_NO,	seg_TEXT, seg_DATA, seg_BSS
}	SEG_T;

#include "function.h"
