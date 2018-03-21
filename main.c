/*
	main.c --	Cm Compiler main routine

	Copyright (C) 1989 by Yoshiyuki Kondo.
		All Rights Reserved.

$Log: RCS/main.c $
 * revision 2.1 cond 89/10/04 23:33:18
 * Official version distributed with C-magazine, Dec 1989
 * 
 * revision 1.21 cond 89/10/01 12:19:09
 * alter usage message to indicate "yes, It's OK to release to public!"
 * 
 * revision 1.2 cond 89/09/17 20:35:13
 * Rewrite some functions to distinguish enum and int(char).  LSI C compiler no longer complains..
 * 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
/* #include <jctype.h> */ /* by E.Kako */

#include "cmdef.h"
#include "cm.h"


static char rcsID[] = "$Header: RCS/main.c 2.1 89/10/04 23:33:18 cond Exp $";

#define	FILENAME	100


char	Title[] = "\
C minor compiler Ver 1.00 Copyright (C) 1989 by Yoshiyuki Kondo\n\
";
char	Usage[] =
"usage: cm [-o outfile] infile\n\
";

char	FileName[FILENAME];

int	yydebug, debugSym, debugDecl, errCount, warnCount;

FILE	*inf, *outf;

extern	char	yyident[];
extern	YYSTYPE	yylval;


private	FILE	*efopen(char *file, char *mode)
{
	FILE	*fp;

	if ((fp = fopen(file, mode)) == NULL) {
		fprintf(stderr, "can't open: %s\n", file);
		exit(1);
	}
	return (fp);
}

char	*nameof(char *s)
{
	char	*t, c;

	t = s;
	while ((c = *s++) != '\0') {
		if (iskanji(c) && iskanji2(*s))
			s++;
		else if (c == ':' || c == '\\' || c == '/')
			t = s;
	}
	return (t);
}

char	*typeof(char *s)
{
	s = nameof(s);
	while (*s && *s != '.')
		s++;
	return (s);
}

private	void	usage(void)
{
	fprintf(stderr, Title);
	fprintf(stderr, Usage);
	exit(1);
}

static	char	outfile[FILENAME];

void	main(int argc, char *argv[])
{
	char	*p, *ofn;
	char	c;

	ofn = NULL;
	while (++argv, --argc != 0 && argv[0][0] == '-')
		for (p = &argv[0][1]; c = *p++;)
			switch (c) {
			Case 'o':
				if (*p == '\0') {
					if (argv[1] != NULL) {
						ofn = argv[1];
						argv++;
						argc--;
					} else
						usage();
				} else {
					ofn = p;
					p = "";
				}
			Case 'p':
				yydebug = 1;
			Case 's':
				debugSym = 1;
			Case 'd':
				debugDecl = 1;
			Default:
				usage();
			}
	if (argc != 1)
		usage();
	if (*(p = typeof(strcpy(FileName, argv[0]))) != '.')
		strcpy(p, ".c");
	if (ofn == NULL) {
		strcpy(typeof(strcpy(outfile, argv[0])), ".asm");
		ofn = outfile;
	}
	inf = efopen(FileName, "r");
	outf = efopen(ofn, "w");

	errCount = warnCount = 0;

	resetHeap();
	initStr();
	initTable();

	gencode("\t.386p\n\n");
	gencode("\t.model flat,syscall\n\n");
	openseg(seg_TEXT);
	openseg(seg_DATA);
	openseg(seg_BSS);

	yyparse();

	genGloref();

	gencode("\tend\n");

/**	dumpSymTbl();	**/

	fclose(inf);
	fclose(outf);

	exit(errCount != 0 ? 1 : warnCount != 0 ? 2 : 0);

#if	0
{	int	sym;
    while ((sym = yylex()) != YYEOF) {
        switch (sym) {
		Case IDENTIFIER:
			printf("ID :\"%s\"\n", yyident);
		Case CONSTANT:
			switch (yylval._const.type) {
			Case CONST_INT:
				printf("CONST_INT:  val=%d\n", yylval._const.value);
			Case CONST_CHAR:
				printf("CONST_CHAR: val=%c [%02x]\n", yylval._const.value,
													  yylval._const.value);
			Case CONST_STRING:
				printf("CONST_STRING: val=%d\n", yylval._const.value);
			Default:
				printf("CONST: but type is invalid=%d\n", yylval._const.type);
			}
		Default:
			printf("other: sym=%d op=%c(%d)\n", sym,
								isprint(yylval.op) ? yylval.op : '-',
								yylval.op);
		}
	}

	fclose(inf);
/*	fclose(Outf);					**/
}
#endif
}
