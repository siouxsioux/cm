/****   function.h  -- function prototypes   ****/ 
/**/ 
/*** declare.c ***/ 
	void	globalDataDecl(TREE *tree, int extp);
	void	funcDef(TYPE *type, TREE *tree);
	void	funchead(void);
	void	funcend(void);
	void	localDataDecl(TREE *tree);
	TYPE	*abstdecl(TYPE *type, TREE *tree);
/*** exp.c ***/ 
	EXPR	*expConst(CONST c);
	EXPR	*expIdent(char *sym);
	EXPR	*expAddrof(EXPR *p);
	EXPR	*expIndirect(EXPR *p);
	EXPR	*enBool(EXPR *p);
	EXPR	*deBool(EXPR *p, char to);
	EXPR	*expUnary(OPCODE op, EXPR *p);
	EXPR	*expIncdec(OPCODE op, EXPR *p);
	EXPR	*expBinary(OPCODE op, EXPR *a, EXPR *b);
	EXPR	*expAddsub(OPCODE op, EXPR *a, EXPR *b);
	EXPR	*expShiftop(OPCODE op, EXPR *a, EXPR *b);
	EXPR	*expCompare(OPCODE op, EXPR *a, EXPR *b);
	EXPR	*expLogop(OPCODE op, EXPR *a, EXPR *b);
	EXPR	*expConditional(EXPR *cond, EXPR *a, EXPR *b);
	EXPR	*expAssignop(char op, EXPR *a, EXPR *b);
	EXPR	*expAssign(EXPR *a, EXPR *b);
	EXPR	*expSizeofExp(EXPR *a);
	EXPR	*expSizeofType(TYPE *a);
	EXPR	*expCast(TYPE *to, EXPR *a);
	EXPR	*expComma(EXPR *a, EXPR *b);
	EXPR	*expArray(EXPR *a, EXPR *b);
	EXPR	*expFuncall(EXPR *fun, TREE *paramlist);
/*** gencode.c ***/ 
	void	gencode(char *fmt, ...);
	void	genlea(EXPR *p);
	void	pushax(void);
	void	pushbx(void);
	void	popax(void);
	void	popbx(void);
	void	popcx(void);
	void	popdx(void);
	void	popdi(void);
	void	closeseg(void);
	void	openseg(SEG_T seg);
/*** genexp.c ***/ 
	bool	isVariable(EXPR *a);
	void	genlabel(int label);
	void	genjump(int label);
	void	expstmt(EXPR *p);
	void	genexptop(EXPR *p);
	void	genbool(EXPR *p, int true, int false);
	void	genexp(EXPR *p);
/*** heap.c ***/ 
	void	resetHeap(void);
	TREE	*list1(DCLNODE a);
	TREE	*list2(DCLNODE a, void *b);
	TREE	*list3(DCLNODE a, void *b, void *c);
	TREE	*append(TREE *a, TREE *b);
	char	*saveIdent(char *s);
	EXPR	*makeNode1(OPCODE a, char optype, TYPE *type, EXPR *left);
	EXPR	*makeNode2(OPCODE a, char optype, TYPE *type, EXPR *left, EXPR *right);
	EXPR	*makeNode3(OPCODE a, char optype, TYPE *type, EXPR *left, EXPR *right, EXPR *third);
/*** lex.c ***/ 
	char	readch(void);
	void	backch(char  c);
	YYLEX	yylex(void);
/*** lexstr.c ***/ 
	void	initStr(void);
	void	flushStr(void);
	int	tonumber(char  c, int  radix);
	char	chaval(char  ch);
	char	*readString(void);
	YYLEX	lexString(void);
/*** main.c ***/ 
/*** misc.c ***/ 
	void	error_line(void);
	void	error(char *str);
	void	error2(char *str, char *param);
	void	error3(char *str, char *a, char *b);
	void	fatalError(char *str);
/*	int	yyerror(char *s); */
	void	bug(char *func);
	char	*opname(OPCODE op);
	int		gensym(void);
	bool	isPrimeType(TYPE *t);
	bool	eqType(TYPE *a, TYPE *b);
	TYPE	*makePointer(TYPE *t);
	char	toOptype(TYPE *t);
	TYPE	*toType(char optype);
	bool	isPointer(TYPE *t);
	bool	isArray(TYPE *t);
	bool	isFunction(TYPE *t);
	bool	isNumeric(TYPE *t);
	EXPR	*coerce(EXPR *a, char to);
	char	adjust(char left, char right);
	int	computeSize(TYPE *type);
	int	fold1(OPCODE op, int a);
	int	fold2(OPCODE op, int x, int y);
/*** stmt.c ***/ 
	void	pushLabels(void);
	void	popLabels(void);
	LABEL	*searchLabel(char *s);
	LABEL	*regLabel(char *s, bool defp);
	void	doSwitchhead(EXPR *exp);
	void	doSwitchend(void);
	void	doCase(EXPR *exp);
	void	doDefault(void);
	void	initFunc(void);
	void	endFunc(void);
/*** table.c ***/ 
	void	initTable(void);
	void	initLocTbl(void);
	char	*saveGloId(char *s);
	char	*saveLocId(char *s);
	SYMTBL	*regGloId(char *s);
	SYMTBL	*regLocId(char *s);
	SYMTBL	*searchGlo(char *s);
	SYMTBL	*searchLoc(char *s);
	TYPE	*regGloType(TCLASS tc, TYPE *father);
	TYPE	*regLocType(TCLASS tc, TYPE *father);
	void	genGloref(void);
	int	computeOffset(void);
	void	dumpSymTbl(void);
