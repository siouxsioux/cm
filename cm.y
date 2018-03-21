/*
	cm.y	syntax definition of C-minor language

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

	Note)	When YACC processes this file,
		two shift/reduce conflicts are reported,
		but DON'T PANIC, just ignore it....

$Log: RCS/cm.y $
# revision 2.1 cond 89/10/04 23:35:34
# Official version distributed with C-magazine, Dec 1989
# 
# revision 1.21 cond 89/10/01 12:16:37
# eliminate one shift/reduce conflict (in 'declarator2')
# 
# revision 1.2 cond 89/09/17 20:35:39
# Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
# 
*/

%{

#include <stdio.h>
#include "cmdef.h"

static char rcsID[] = "$Header: RCS/cm.y 2.1 89/10/04 23:35:34 cond Exp $";

/**#define	YYDEBUG**/
#define	YYDISPLAY

int	breakLabel;			/** label for 'break' **/
int	contLabel;			/** label for 'continue' **/
extern	int	exitLabel;		/** label for 'return' **/
extern	SYMTBL	*currentFunction;	/** function in process now **/

%}


%union	{
	CONST	_const;
	TREE	*tree;
	TYPE	*type;
	char	*symbol;
	EXPR	*expr;
	char	op;
	int	label;
}


/*
 *	terminal symbols
 */

%token	<symbol>	IDENTIFIER
%token	<_const>	CONSTANT
%token			IF ELSE WHILE FOR DO BREAK CONTINUE SWITCH CASE
%token			DEFAULT RETURN GOTO SIZEOF INT CHAR EXTERN

%token			';' '(' ')' '[' ']' '{' '}' ','
%token			'?' ':' '|' '^' '&' '*' '.'

%token	<op>		addop shiftop mulop compop eqop assignop
%token	<op>		incdecop unop
%token			'=' logor logand


/*
 *	nonterminal symbols
 */

/**	declaration	**/

%type	<type>		type_spec type_name

%type	<tree>		declaration
%type	<tree>		declarator_list declarator declarator2
%type	<tree>		param_list param_decl
%type	<tree>		abst_declarator abst_declarator2
%type	<tree>		expr_list

%type	<expr>		primary nc_expr expr expr_opt

%type	<label>		do_head
%type	<label>		if_head
%type	<label>		while_head
%type	<label>		for_head
%type	<label>		switch_head


/*
 *	procedence table
 */

%left		','		/* comma	,		*/
%right		assignop '='	/* assignment	=, +=, -= etc.	*/
%right		'?' ':'		/* conditional	? :		*/
%left		logor		/* logical or	||		*/
%left		logand		/* logical and	&&		*/
%left		'|'		/* bitwise or	|		*/
%left		'^'		/* bitwise xor	^		*/
%left		'&'		/* bitwise and	&		*/
%left		eqop		/* equality	==, !=		*/
%left		compop		/* comparison	<, >, <=, >=	*/
%left		shiftop		/* shift	>>, <<		*/
%left		addop		/* add, sub	+, -		*/
%left		mulop '*'	/* mul, div	*, /, %		*/
%right		unop		/* unary	!, ~, +, -, *, &*/
%nonassoc	incdecop	/* inc, dec	++, --		*/


%%


file
	: extern_def
	| file extern_def
	  { yyerrok; }
	| error
	| error func_body
	| file error
	| file error func_body
	;


extern_def
	: func_def
	| EXTERN func_def
	| declaration
		{ globalDataDecl($1, 0); }
	| EXTERN declaration
		{ globalDataDecl($2, 1); }
	| sc
	;

func_def
	: type_spec declarator
		{ funcDef($1, $2);
		  initFunc();
		  contLabel = breakLabel = 0;
		  initStr(); }
	  func_body
		{ funcend();
		  endFunc();
		  flushStr(); }
	| declarator
		{ funcDef(TYPE_INTEGER, $1);
		  initFunc();
		  contLabel = breakLabel = 0;
		  initStr(); }
	  func_body
		{ funcend();
		  endFunc();
		  flushStr(); }
	;

func_body
	: '{' declaration_list
		{ funchead(); }
	  stmt_list '}'
	;

declaration_list
	: /* empty */
	| declaration_list declaration
		{ yyerrok;
		  localDataDecl($2); }
	| declaration_list error
	;


/***************************/
/****	declaration	****/
/***************************/

declaration
	: type_spec declarator_list sc
		{ $$ = list3(dcl_DUMMY, $1, $2); }
	| type_spec sc
		{ $$ = list3(dcl_DUMMY, $1, (void *)NULL); }
	;

type_spec
	: INT
		{ $$ = TYPE_INTEGER; }
	| CHAR
		{ $$ = TYPE_CHAR; }
	;

declarator_list
	: declarator
		{ $$ = list3(dcl_DUMMY, $1, NULL); }
	| declarator_list ',' declarator
		{ $$ = append($1, list3(dcl_DUMMY, $3, NULL)); }
	;


declarator
	: '*' declarator
		{ $$ = list2(dcl_PTR, $2); }
	| declarator2
		{ $$ = $1; }
	;

declarator2
	: IDENTIFIER
		{ $$ = list2(dcl_ID, (void *)saveIdent($1)); }
	| '(' declarator rp
		{ $$ = $2; }
	| declarator2 '[' expr ']'
		{ if ($3->optype != 'K')
		  	error("array size must be constant");
		  $$ = list3(dcl_ARRAY, $1,(void *)($3->e_value)); }
	| declarator2 '(' rp
		{ $$ = list3(dcl_FUNC, $1, NULL); }
	| declarator2 '(' param_list rp
		{ $$ = list3(dcl_FUNC, $1, $3); }
	| declarator2 '(' error rp
		{ $$ = list3(dcl_FUNC, $1, ERROR); }
	;


param_list
	: param_decl
		{ $$ = list3(dcl_DUMMY, $1, NULL); }
	| param_list ',' param_decl
		{ yyerrok;
		  $$ = append($1, list3(dcl_DUMMY, $3, NULL)); }
	| param_list error
		{ $$ = ERROR; }
	| param_list error param_decl
		{ yyerrok;
		  $$ = ERROR; }
	| param_list ',' error
		{ $$ = ERROR; }
	;

param_decl
	: type_spec declarator
		{ $$ = list3(dcl_DUMMY, $1, $2); }
	;


type_name
	: type_spec abst_declarator
		{ $$ = abstdecl($1, $2); }
	| type_spec
		{ $$ = $1; }
	;

abst_declarator
	: '*'
		{ $$ = list2(dcl_PTR, list1(dcl_NONE)); }
	| '*' abst_declarator
		{ $$ = list2(dcl_PTR, $2); }
	| abst_declarator2
		{ $$ = $1; }
	;

abst_declarator2
	: '(' abst_declarator rp
		{ $$ = $2; }
	| '[' expr ']'
		{ if ($2->optype != 'K')
		  	error("array size must be constant");
		  $$ = list3(dcl_ARRAY, list1(dcl_NONE),
					(void *)($2->e_value)); }
	| abst_declarator2 '[' expr ']'
		{ if ($3->optype != 'K')
		  	error("array size must be constant");
		  $$ = list3(dcl_ARRAY, $1, (void *)($3->e_value)); }
	| abst_declarator2 '(' rp
		{ $$ = list3(dcl_FUNC, $1, NULL); }
	| abst_declarator2 '(' param_list rp
		{ $$ = list3(dcl_FUNC, $1, $3); }
	| '(' rp
		{ $$ = list3(dcl_FUNC, list1(dcl_NONE), NULL); }
	| '(' param_list rp
		{ $$ = list3(dcl_FUNC, list1(dcl_NONE), $2); }
	;


/***************************/
/****	statement	****/
/***************************/

stmt_list
	: /* empty */
	| stmt_list stmt
		{ yyerrok;
		  resetHeap(); }
	| stmt_list error
		{ resetHeap(); }
	;

stmt
	: compound_stmt
	| expr sc
		{ expstmt($1);
		  resetHeap(); }
	| if_head stmt
		{ genlabel($1); }
	| if_head stmt ELSE
		{ genjump($1 + 1);
		  genlabel($1); }
	  stmt
		{ genlabel($1 + 1); }
	| while_head stmt
		{ genjump($1);
		  genlabel($1 + 1);
		  popLabels(); }
	| do_head stmt WHILE '(' expr rp sc
		{ genbool(enBool($5), $1, 0);
		  genlabel($1 + 1);
		  popLabels(); }
	| for_head stmt
		{ genjump($1);
		  genlabel($1 + 2);
		  popLabels(); }
	| switch_head stmt
		{ doSwitchend(); }
	| CASE expr
		{ doCase($2); }
	  ':' stmt
	| DEFAULT
		{ doDefault(); }
	  ':' stmt
	| BREAK sc
		{ if (breakLabel == 0) 
			error("break outside loop/switch");
		  genjump(breakLabel); }
	| CONTINUE sc
		{ if (contLabel == 0)
			error("continue outside loop");
		  genjump(contLabel); }
	| RETURN sc
		{ genjump(exitLabel); }
	| RETURN expr sc
		{ if ($2->optype != 'K' &&
		      !eqType(currentFunction->type->father, $2->type)) {
			error("return type mismatch");
			return;
		  }
		  if ($2->optype == 'R') {
			genexp($2);
			gencode("\tmov\tax,bx\n");
		  } else
		  	genexp(coerce($2, 'I'));
		  genjump(exitLabel); }
	| GOTO IDENTIFIER sc
		{ LABEL	*label;
		  if ((label = searchLabel($2)) == NULL)
			label = regLabel($2, NO);
		  genjump(label->num); }
	| IDENTIFIER
		{ LABEL *l;
		  if ((l = searchLabel($1)) == NULL)
		  	l = regLabel($1, YES);
		  else if (l->defp == 0)
		  	l->defp = YES;
		  else {
		  	error2("label '%s' is defined twice", $1);
			return;
		  }
		  genlabel(l->num); }
	  ':' stmt
	| sc
	;

if_head
	: IF '(' expr rp
		{ int	label;
		  $$ = label = gensym();	gensym();
		  genbool(enBool($3), 0, label);
		  resetHeap(); }
	| IF error
		{ $$ = -1;
		  resetHeap(); }
	;

while_head
	: WHILE '(' expr rp
		{ int	label;
		  $$ = label = gensym();	gensym();
		  pushLabels();
		  breakLabel = label + 1;	contLabel = label;
		  genlabel(label);
		  genbool(enBool($3), 0, label + 1);
		  resetHeap(); }
	| WHILE error
		{ $$ = -1;
		  resetHeap(); }
	;

do_head
	: DO

		{ int	label;
		  $$ = label = gensym();		gensym();
		  pushLabels();
		  breakLabel = label + 1;		contLabel = label;
		  genlabel(label); }
	;


for_head
	: FOR '(' expr_opt sc expr_opt sc expr_opt rp
		{ int	label;
		  $$ = label = gensym();   gensym();   gensym();
		  pushLabels();
		  breakLabel = label + 2;	contLabel = label;
		  if ($3 != NULL)  	genexptop($3);
		  if ($7 != NULL)  	genjump(label + 1);
		  genlabel(label);
		  if ($7 != NULL)     {	genexptop($7);	genlabel(label + 1); }
		  if ($5 != NULL)	genbool(enBool($5), 0, label + 2);
		  resetHeap(); }
	| FOR error
		{ $$ = -1;
		  resetHeap(); }
	;

expr_opt
	: /* empty */
		{ $$ = NULL; }
	| expr
		{ $$ = $1; }
	;

switch_head
	: SWITCH '(' expr rp
		{ doSwitchhead($3); }
	| SWITCH error
		{ doSwitchhead(ERROR); }
	;

compound_stmt
	: '{'
		{ yyerrok; }
	  stmt_list rb
		{ }
	;

nc_expr
	: primary
		{ $$ = $1; }
	| '*' nc_expr  %prec unop
		{ $$ = expIndirect($2); }
	| '&' nc_expr  %prec unop
		{ $$ = expAddrof($2); }
	| addop nc_expr  %prec unop			/**  unary +, -  **/
		{ $$ = expUnary($1 == '+' ? op_PLUS : op_MINUS, $2); }
	| unop nc_expr					/**  unary !, ~  **/
		{ $$ = expUnary($1 == '!' ? op_LNOT : op_BNOT, $2); }
	| incdecop nc_expr				/**  pre ++, --  **/
		{ $$ = expIncdec($1 == '+' ? op_PREINC : op_PREDEC, $2); }
	| nc_expr incdecop				/**  post ++, -- **/
		{ $$ = expIncdec($2 == '+' ? op_POSTINC : op_POSTDEC, $1); }
	| nc_expr mulop nc_expr				/**  %, /        **/
		{ $$ = expBinary($2 == '%' ? op_MOD : op_DIV, $1, $3); }
	| nc_expr '*' nc_expr
		{ $$ = expBinary(op_MUL, $1, $3); }
	| nc_expr addop nc_expr				/**  +, -        **/
		{ $$ = expAddsub($2 == '+' ? op_ADD : op_SUB, $1, $3); }
	| nc_expr shiftop nc_expr			/**  <<, >>      **/
		{ $$ = expShiftop($2 == '>' ? op_SHR : op_SHL, $1, $3); }
	| nc_expr compop nc_expr			/**  <, <=, >, >= **/
		{ OPCODE op;
		  switch ($2) {
		  Case '>':	op = op_GT;
		  Case ')':	op = op_GE;
		  Case '<':	op = op_LT;
		  Case '(':	op = op_LE;
		  }
		  $$ = expCompare(op, $1, $3);
		}
	| nc_expr eqop nc_expr				/**  ==, !=      **/
		{ $$ = expCompare($2 == '=' ? op_EQ : op_NEQ, $1, $3); }
	| nc_expr '&' nc_expr
		{ $$ = expBinary(op_AND, $1, $3); }
	| nc_expr '^' nc_expr
		{ $$ = expBinary(op_XOR, $1, $3); }
	| nc_expr '|' nc_expr
		{ $$ = expBinary(op_OR, $1, $3); }
	| nc_expr logand nc_expr			/**  &&          **/
		{ $$ = expLogop(op_LAND, $1, $3); }
	| nc_expr logor nc_expr				/**  ||          **/
		{ $$ = expLogop(op_LOR, $1, $3); }
	| nc_expr '?' nc_expr ':' nc_expr
		{ $$ = expConditional($1, $3, $5); }
	| nc_expr assignop nc_expr			/**  +=, -= etc. **/
		{ $$ = expAssignop($2, $1, $3); }
	| nc_expr '=' nc_expr
		{ $$ = expAssign($1, $3); }
	| SIZEOF nc_expr  %prec unop
		{ $$ = expSizeofExp($2); }
	| SIZEOF '(' type_name rp  %prec unop
		{ $$ = expSizeofType($3); }
	| '(' type_name rp nc_expr  %prec unop
		{ $$ = expCast($2, $4); }
	;

expr
	: nc_expr
		{ $$ = $1; }
	| expr ',' nc_expr
		{ yyerrok;
		  $$ = expComma($1, $3); }
	| expr error
		{ $$ = ERROR; }
	| expr error nc_expr
		{ $$ = ERROR;
		  yyerrok; }
	| expr ',' error
		{ $$ = ERROR; }
	;

primary
	: IDENTIFIER
		{ $$ = expIdent($1); }
	| CONSTANT
		{ $$ = expConst($1); }
	| '(' expr rp
		{ $$ = $2; }
	| primary '(' expr_list rp
		{ $$ = expFuncall($1, $3); }
	| primary '(' rp
		{ $$ = expFuncall($1, NULL); }
	| primary '[' expr ']'
		{ $$ = expArray($1, $3); }
	| '(' error rp
		{ $$ = ERROR; }	
	;

expr_list
	: nc_expr
		{ $$ = list3(dcl_DUMMY, NULL, $1); }
	| expr_list ',' nc_expr
		{ yyerrok;
		  $$ = list3(dcl_DUMMY, $1, $3);
		}
	| error
		{ $$ = ERROR; }
	| expr_list error
		{ $$ = ERROR; }
	| expr_list error nc_expr
		{ yyerrok;
		  $$ = ERROR; }
	| expr_list ',' error
		{ $$ = ERROR; }
	;


rb	: '}'	{ yyerrok; }
	;
rp	: ')'	{ yyerrok; }
	;
sc	: ';'	{ yyerrok; }
	;

%%
